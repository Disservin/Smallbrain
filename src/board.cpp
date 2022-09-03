#include <algorithm>
#include <locale>

#include "board.h"
#include "helper.h"
#include "sliders.hpp"

using namespace Chess_Lookup::Fancy;

void Board::initializeLookupTables()
{
    // initialize squares between table
    U64 sqs;
    for (int sq1 = 0; sq1 <= 63; ++sq1)
    {
        for (int sq2 = 0; sq2 <= 63; ++sq2)
        {
            sqs = (1ULL << sq1) | (1ULL << sq2);
            if (square_file(Square(sq1)) == square_file(Square(sq2)) ||
                square_rank(Square(sq1)) == square_rank(Square(sq2)))
                SQUARES_BETWEEN_BB[sq1][sq2] = RookAttacks(Square(sq1), sqs) & RookAttacks(Square(sq2), sqs);

            else if (diagonal_of(Square(sq1)) == diagonal_of(Square(sq2)) ||
                     anti_diagonal_of(Square(sq1)) == anti_diagonal_of(Square(sq2)))
                SQUARES_BETWEEN_BB[Square(sq1)][sq2] =
                    BishopAttacks(Square(sq1), sqs) & BishopAttacks(Square(sq2), sqs);
        }
    }
}

void Board::accumulate()
{
    for (int i = 0; i < HIDDEN_BIAS; i++)
        accumulator[i] = hiddenBias[i];

    for (int i = 0; i < 64; i++)
    {
        Piece p = board[i];
        bool input = p != None;
        if (!input)
            continue;
        int j = Square(i) + (p)*64;
        NNUE::activate(accumulator, j);
    }
}

Board::Board()
{
    initializeLookupTables();
    prevStates.list.reserve(MAX_PLY);
    hashHistory.reserve(512);
}

U64 Board::zobristHash()
{
    U64 hash = 0ULL;
    U64 wPieces = Us(White);
    U64 bPieces = Us(Black);
    // Piece hashes
    while (wPieces)
    {
        Square sq = poplsb(wPieces);
        hash ^= updateKeyPiece(pieceAtB(sq), sq);
    }
    while (bPieces)
    {
        Square sq = poplsb(bPieces);
        hash ^= updateKeyPiece(pieceAtB(sq), sq);
    }
    // Ep hash
    U64 ep_hash = 0ULL;
    if (enPassantSquare != NO_SQ)
    {
        ep_hash = updateKeyEnPassant(enPassantSquare);
    }
    // Turn hash
    U64 turn_hash = sideToMove == White ? RANDOM_ARRAY[780] : 0;
    // Castle hash
    U64 cast_hash = updateKeyCastling();

    return hash ^ cast_hash ^ turn_hash ^ ep_hash;
}

Piece Board::pieceAtBB(Square sq)
{
    if (Bitboards[WhitePawn] & (1ULL << sq))
        return WhitePawn;
    if (Bitboards[WhiteKnight] & (1ULL << sq))
        return WhiteKnight;
    if (Bitboards[WhiteBishop] & (1ULL << sq))
        return WhiteBishop;
    if (Bitboards[WhiteRook] & (1ULL << sq))
        return WhiteRook;
    if (Bitboards[WhiteQueen] & (1ULL << sq))
        return WhiteQueen;
    if (Bitboards[WhiteKing] & (1ULL << sq))
        return WhiteKing;
    if (Bitboards[BlackPawn] & (1ULL << sq))
        return BlackPawn;
    if (Bitboards[BlackKnight] & (1ULL << sq))
        return BlackKnight;
    if (Bitboards[BlackBishop] & (1ULL << sq))
        return BlackBishop;
    if (Bitboards[BlackRook] & (1ULL << sq))
        return BlackRook;
    if (Bitboards[BlackQueen] & (1ULL << sq))
        return BlackQueen;
    if (Bitboards[BlackKing] & (1ULL << sq))
        return BlackKing;
    return None;
}

Piece Board::pieceAtB(Square sq)
{
    return board[sq];
}

void Board::applyFen(std::string fen, bool updateAcc)
{
    for (int i = 0; i < 12; i++)
    {
        Bitboards[i] = 0ULL;
    }
    std::vector<std::string> params = splitInput(fen);
    std::string position = params[0];
    std::string move_right = params[1];
    std::string castling = params[2];
    std::string en_passant = params[3];
    // default
    std::string half_move_clock = "0";
    std::string full_move_counter = "1";
    if (params.size() > 4)
    {
        half_move_clock = params[4];
        full_move_counter = params[5];
    }
    sideToMove = (move_right == "w") ? White : Black;

    if (updateAcc)
    {
        for (int i = 0; i < HIDDEN_BIAS; i++)
            accumulator[i] = hiddenBias[i];
    }

    Square square = Square(56);
    for (int index = 0; index < (int)position.size(); index++)
    {
        char curr = position[index];
        if (charToPiece.find(curr) != charToPiece.end())
        {
            Piece piece = charToPiece[curr];
            if (updateAcc)
                placePiece<true>(piece, square);
            else
                placePiece<false>(piece, square);
            square = Square(square + 1);
        }
        else if (curr == '/')
            square = Square(square - 16);
        else if (isdigit(curr))
        {
            for (int i = 0; i < (int)curr - '0'; i++)
            {
                board[square + i] = None;
            }
            square = Square(square + (curr - '0'));
        }
    }

    castlingRights = 0;
    for (size_t i = 0; i < castling.size(); i++)
    {
        if (castling[i] == 'K')
        {
            castlingRights |= wk;
        }
        if (castling[i] == 'Q')
        {
            castlingRights |= wq;
        }
        if (castling[i] == 'k')
        {
            castlingRights |= bk;
        }
        if (castling[i] == 'q')
        {
            castlingRights |= bq;
        }
    }
    if (en_passant == "-")
    {
        enPassantSquare = NO_SQ;
    }
    else
    {
        char letter = en_passant[0];
        int file = letter - 96;
        int rank = en_passant[1] - 48;
        enPassantSquare = Square((rank - 1) * 8 + file - 1);
    }
    // half_move_clock
    halfMoveClock = std::stoi(half_move_clock);

    // full_move_counter actually half moves
    fullMoveNumber = std::stoi(full_move_counter) * 2;

    hashHistory.clear();
    hashHistory.push_back(zobristHash());

    prevStates.list.clear();
    hashKey = zobristHash();
    accumulatorStack.clear();
}

std::string Board::getFen()
{
    int sq;
    char letter;
    std::string fen = "";
    for (int rank = 7; rank >= 0; rank--)
    {
        int free_space = 0;
        for (int file = 0; file < 8; file++)
        {
            sq = rank * 8 + file;
            Piece piece = pieceAtB(Square(sq));
            if (piece != None)
            {
                if (free_space)
                {
                    fen += std::to_string(free_space);
                    free_space = 0;
                }
                letter = pieceToChar[piece];
                fen += letter;
            }
            else
            {
                free_space++;
            }
        }
        if (free_space != 0)
        {
            fen += std::to_string(free_space);
        }
        fen += rank > 0 ? "/" : "";
    }
    fen += sideToMove == White ? " w " : " b ";

    if (castlingRights & wk)
        fen += "K";
    if (castlingRights & wq)
        fen += "Q";
    if (castlingRights & bk)
        fen += "k";
    if (castlingRights & bq)
        fen += "q";
    if (castlingRights == 0)
        fen += "-";

    if (enPassantSquare == NO_SQ)
        fen += " - ";
    else
        fen += " " + squareToString[enPassantSquare] + " ";

    fen += std::to_string(halfMoveClock);
    fen += " " + std::to_string(fullMoveNumber / 2);
    return fen;
}

void Board::printBitboard(U64 bb)
{
    std::bitset<64> b(bb);
    std::string str_bitset = b.to_string();
    for (int i = 0; i < MAX_SQ; i += 8)
    {
        std::string x = str_bitset.substr(i, 8);
        reverse(x.begin(), x.end());
        std::cout << x << std::endl;
    }
    std::cout << '\n' << std::endl;
}

void Board::print()
{
    for (int i = 63; i >= 0; i -= 8)
    {
        std::cout << " " << pieceToChar[board[i - 7]] << " " << pieceToChar[board[i - 6]] << " "
                  << pieceToChar[board[i - 5]] << " " << pieceToChar[board[i - 4]] << " " << pieceToChar[board[i - 3]]
                  << " " << pieceToChar[board[i - 2]] << " " << pieceToChar[board[i - 1]] << " "
                  << pieceToChar[board[i]] << " " << std::endl;
    }
    std::cout << '\n' << std::endl;
    std::cout << "Fen: " << getFen() << std::endl;
    std::cout << "Side to move: " << (int)sideToMove << std::endl;
    std::cout << "Castling rights: " << (int)castlingRights << std::endl;
    std::cout << "Halfmoves: " << (int)halfMoveClock << std::endl;
    std::cout << "Fullmoves: " << (int)fullMoveNumber / 2 << std::endl;
    std::cout << "EP: " << (int)enPassantSquare << std::endl;
    std::cout << "Hash: " << hashKey << std::endl;
}

Piece Board::makePiece(PieceType type, Color c)
{
    if (type == NONETYPE)
        return None;
    return Piece(type + 6 * c);
}

bool Board::isRepetition(int draw)
{
    uint8_t c = 0;
    for (int i = static_cast<int>(hashHistory.size()) - 2;
         i >= 0 && i >= static_cast<int>(hashHistory.size()) - halfMoveClock; i -= 2)
    {
        if (hashHistory[i] == hashKey)
            c++;
        if (c == draw)
            return true;
    }
    return false;
}

bool Board::nonPawnMat(Color c)
{
    return Knights(c) | Bishops(c) | Rooks(c) | Queens(c);
}

U64 Board::PawnAttacks(Square sq, Color c)
{
    return PAWN_ATTACKS_TABLE[c][sq];
}

U64 Board::KnightAttacks(Square sq)
{
    return KNIGHT_ATTACKS_TABLE[sq];
}

U64 Board::BishopAttacks(Square sq, U64 occupied)
{
    return GetBishopAttacks(sq, occupied);
}

U64 Board::RookAttacks(Square sq, U64 occupied)
{
    return GetRookAttacks(sq, occupied);
}

U64 Board::QueenAttacks(Square sq, U64 occupied)
{
    return GetQueenAttacks(sq, occupied);
}

U64 Board::KingAttacks(Square sq)
{
    return KING_ATTACKS_TABLE[sq];
}

U64 Board::Pawns(Color c)
{
    return Bitboards[PAWN + c * 6];
}
U64 Board::Knights(Color c)
{
    return Bitboards[KNIGHT + c * 6];
}
U64 Board::Bishops(Color c)
{
    return Bitboards[BISHOP + c * 6];
}
U64 Board::Rooks(Color c)
{
    return Bitboards[ROOK + c * 6];
}
U64 Board::Queens(Color c)
{
    return Bitboards[QUEEN + c * 6];
}
U64 Board::Kings(Color c)
{
    return Bitboards[KING + c * 6];
}

U64 Board::updateKeyPiece(Piece piece, Square sq)
{
    return RANDOM_ARRAY[64 * hash_piece[piece] + sq];
}

U64 Board::updateKeyEnPassant(Square sq)
{
    return RANDOM_ARRAY[772 + square_file(sq)];
}

U64 Board::updateKeyCastling()
{
    return castlingKey[castlingRights];
}

U64 Board::updateKeySideToMove()
{
    return RANDOM_ARRAY[780];
}

Square Board::KingSQ(Color c)
{
    return Square(bsf(Kings(c)));
}

U64 Board::Us(Color c)
{
    return Bitboards[PAWN + c * 6] | Bitboards[KNIGHT + c * 6] | Bitboards[BISHOP + c * 6] | Bitboards[ROOK + c * 6] |
           Bitboards[QUEEN + c * 6] | Bitboards[KING + c * 6];
}

U64 Board::Enemy(Color c)
{
    return Us(~c);
}

U64 Board::All()
{
    return Us(White) | Us(Black);
}

U64 Board::Empty()
{
    return ~All();
}

U64 Board::EnemyEmpty(Color c)
{
    return ~Us(c);
}

U64 Board::DoCheckmask(Color c, Square sq)
{
    U64 Occ = occAll;
    U64 checks = 0ULL;
    U64 pawn_mask = Pawns(~c) & PawnAttacks(sq, c);
    U64 knight_mask = Knights(~c) & KnightAttacks(sq);
    U64 bishop_mask = (Bishops(~c) | Queens(~c)) & BishopAttacks(sq, Occ);
    U64 rook_mask = (Rooks(~c) | Queens(~c)) & RookAttacks(sq, Occ);
    doubleCheck = 0;
    if (pawn_mask)
    {
        checks |= pawn_mask;
        doubleCheck++;
    }
    if (knight_mask)
    {
        checks |= knight_mask;
        doubleCheck++;
    }
    if (bishop_mask)
    {
        if (popcount(bishop_mask) > 1)
            doubleCheck++;

        int8_t index = bsf(bishop_mask);
        checks |= SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        doubleCheck++;
    }
    if (rook_mask)
    {
        if (popcount(rook_mask) > 1)
            doubleCheck++;

        int8_t index = bsf(rook_mask);
        checks |= SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        doubleCheck++;
    }
    return checks;
}

void Board::DoPinMask(Color c, Square sq)
{
    U64 them = occEnemy;
    U64 rook_mask = (Rooks(~c) | Queens(~c)) & RookAttacks(sq, them);
    U64 bishop_mask = (Bishops(~c) | Queens(~c)) & BishopAttacks(sq, them);

    pinD = 0ULL;
    pinHV = 0ULL;
    while (rook_mask)
    {
        Square index = poplsb(rook_mask);
        U64 possible_pin = (SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & occUs) == 1)
            pinHV |= possible_pin;
    }
    while (bishop_mask)
    {
        Square index = poplsb(bishop_mask);
        U64 possible_pin = (SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & occUs) == 1)
            pinD |= possible_pin;
    }
}

void Board::seenSquares(Color c)
{
    U64 pawns = Pawns(c);
    U64 knights = Knights(c);
    U64 bishops = Bishops(c);
    U64 rooks = Rooks(c);
    U64 queens = Queens(c);
    U64 kings = Kings(c);

    seen = 0ULL;
    Square kSq = KingSQ(~c);
    occAll &= ~(1ULL << kSq);

    if (c == White)
    {
        seen |= pawns << 9 & ~MASK_FILE[0];
        seen |= pawns << 7 & ~MASK_FILE[7];
    }
    else
    {
        seen |= pawns >> 7 & ~MASK_FILE[0];
        seen |= pawns >> 9 & ~MASK_FILE[7];
    }
    while (knights)
    {
        Square index = poplsb(knights);
        seen |= KnightAttacks(index);
    }
    while (bishops)
    {
        Square index = poplsb(bishops);
        seen |= BishopAttacks(index, occAll);
    }
    while (rooks)
    {
        Square index = poplsb(rooks);
        seen |= RookAttacks(index, occAll);
    }
    while (queens)
    {
        Square index = poplsb(queens);
        seen |= QueenAttacks(index, occAll);
    }
    while (kings)
    {
        Square index = poplsb(kings);
        seen |= KingAttacks(index);
    }

    occAll |= (1ULL << kSq);
}

void Board::init(Color c, Square sq)
{
    occUs = Us(c);
    occEnemy = Us(~c);
    occAll = occUs | occEnemy;
    enemyEmptyBB = ~occUs;
    seenSquares(~c);
    U64 newMask = DoCheckmask(c, sq);
    checkMask = newMask ? newMask : DEFAULT_CHECKMASK;
    DoPinMask(c, sq);
}

bool Board::isSquareAttacked(Color c, Square sq)
{
    if (Pawns(c) & PawnAttacks(sq, ~c))
        return true;
    if (Knights(c) & KnightAttacks(sq))
        return true;
    if ((Bishops(c) | Queens(c)) & BishopAttacks(sq, All()))
        return true;
    if ((Rooks(c) | Queens(c)) & RookAttacks(sq, All()))
        return true;
    if (Kings(c) & KingAttacks(sq))
        return true;
    return false;
}

bool Board::isSquareAttackedStatic(Color c, Square sq)
{
    if (Pawns(c) & PawnAttacks(sq, ~c))
        return true;
    if (Knights(c) & KnightAttacks(sq))
        return true;
    if ((Bishops(c) | Queens(c)) & BishopAttacks(sq, occAll))
        return true;
    if ((Rooks(c) | Queens(c)) & RookAttacks(sq, occAll))
        return true;
    if (Kings(c) & KingAttacks(sq))
        return true;
    return false;
}

U64 Board::allAttackers(Square sq, U64 occupiedBB)
{
    return attackersForSide(White, sq, occupiedBB) | attackersForSide(Black, sq, occupiedBB);
}

U64 Board::attackersForSide(Color attackerColor, Square sq, U64 occupiedBB)
{
    U64 attackingBishops = Bishops(attackerColor);
    U64 attackingRooks = Rooks(attackerColor);
    U64 attackingQueens = Queens(attackerColor);
    U64 attackingKnights = Knights(attackerColor);
    U64 attackingKing = Kings(attackerColor);
    U64 attackingPawns = Pawns(attackerColor);

    U64 interCardinalRays = BishopAttacks(sq, occupiedBB);
    U64 cardinalRaysRays = RookAttacks(sq, occupiedBB);

    U64 attackers = interCardinalRays & (attackingBishops | attackingQueens);
    attackers |= cardinalRaysRays & (attackingRooks | attackingQueens);
    attackers |= KnightAttacks(sq) & attackingKnights;
    attackers |= KingAttacks(sq) & attackingKing;
    attackers |= PawnAttacks(sq, ~attackerColor) & attackingPawns;
    return attackers;
}

U64 Board::PawnPushSingle(Color c, Square sq)
{
    if (c == White)
    {
        return ((1ULL << sq) << 8) & ~occAll;
    }
    else
    {
        return ((1ULL << sq) >> 8) & ~occAll;
    }
}

U64 Board::PawnPushBoth(Color c, Square sq)
{
    U64 push = (1ULL << sq);
    if (c == White)
    {
        push = (push << 8) & ~occAll;
        return square_rank(sq) == 1 ? push | ((push << 8) & ~occAll) : push;
    }
    else
    {
        push = (push >> 8) & ~occAll;
        return square_rank(sq) == 6 ? push | ((push >> 8) & ~occAll) : push;
    }
}

int Board::pseudoLegalMovesNumber()
{
    int total = 0;
    U64 pawns_mask = Pawns(sideToMove);
    U64 knights_mask = Knights(sideToMove);
    U64 bishops_mask = Bishops(sideToMove);
    U64 rooks_mask = Rooks(sideToMove);
    U64 queens_mask = Queens(sideToMove);
    while (pawns_mask)
    {
        Square from = poplsb(pawns_mask);
        total += popcount(PawnPushBoth(sideToMove, from) | PawnAttacks(from, sideToMove));
    }
    while (knights_mask)
    {
        Square from = poplsb(knights_mask);
        total += popcount(KnightAttacks(from) & enemyEmptyBB);
    }
    while (bishops_mask)
    {
        Square from = poplsb(bishops_mask);
        total += popcount(BishopAttacks(from, occAll) & enemyEmptyBB);
    }
    while (rooks_mask)
    {
        Square from = poplsb(rooks_mask);
        total += popcount(RookAttacks(from, occAll) & enemyEmptyBB);
    }
    while (queens_mask)
    {
        Square from = poplsb(queens_mask);
        total += popcount(QueenAttacks(from, occAll) & enemyEmptyBB);
    }
    Square from = KingSQ(sideToMove);
    total += popcount(KingAttacks(from) & enemyEmptyBB);
    return total;
}

U64 Board::LegalPawnNoisy(Color c, Square sq, Square ep)
{
    U64 enemy = occEnemy;
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the checkmask
    if (pinD & (1ULL << sq))
        return PawnAttacks(sq, c) & pinD & checkMask & (enemy | (1ULL << ep));
    // If we are pinned horizontally/vertically we can do no captures
    if (pinHV & (1ULL << sq))
        return 0ULL;
    U64 attacks = PawnAttacks(sq, c);
    U64 pawnPromote = 0ULL;
    if ((square_rank(sq) == 1 && c == Black) || (square_rank(sq) == 6 && c == White))
    {
        pawnPromote = PawnPushSingle(c, sq) & ~occAll;
    }
    return ((attacks & enemy) | pawnPromote) & checkMask;
}

U64 Board::LegalKingCaptures(Square sq)
{
    return KingAttacks(sq) & occEnemy & ~seen;
}

U64 Board::LegalPawnMoves(Color c, Square sq)
{
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the checkmask
    if (pinD & (1ULL << sq))
        return PawnAttacks(sq, c) & pinD & checkMask & occEnemy;

    // If we are pinned horizontally we can do no moves but if we are pinned vertically we can only do pawn pushs
    if (pinHV & (1ULL << sq))
        return PawnPushBoth(c, sq) & pinHV & checkMask;
    return ((PawnAttacks(sq, c) & occEnemy) | PawnPushBoth(c, sq)) & checkMask;
}

U64 Board::LegalPawnMovesEP(Color c, Square sq, Square ep)
{
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the checkmask
    if (pinD & (1ULL << sq))
        return PawnAttacks(sq, c) & pinD & checkMask & (occEnemy | (1ULL << ep));

    // Calculate pawn pushs

    // If we are pinned horizontally we can do no moves but if we are pinned vertically we can only do pawn pushs
    if (pinHV & (1ULL << sq))
        return PawnPushBoth(c, sq) & pinHV & checkMask;
    U64 attacks = PawnAttacks(sq, c);
    if (checkMask != DEFAULT_CHECKMASK)
    {
        // If we are in check and the en passant square lies on our attackmask and the en passant piece gives check
        // return the ep mask as a move square
        if (attacks & (1ULL << ep) && checkMask & (1ULL << (ep - (c * -2 + 1) * 8)))
            return 1ULL << ep;
        // If we are in check we can do all moves that are on the checkmask
        return ((attacks & occEnemy) | PawnPushBoth(c, sq)) & checkMask;
    }

    U64 moves = ((attacks & occEnemy) | PawnPushBoth(c, sq)) & checkMask;
    // We need to make extra sure that ep moves dont leave the king in check
    // 7k/8/8/K1Pp3r/8/8/8/8 w - d6 0 1
    // Horizontal rook pins our pawn through another pawn, our pawn can push but not take enpassant
    // remove both the pawn that made the push and our pawn that could take in theory and check if that would give check
    if ((1ULL << ep) & attacks)
    {
        Square tP = c == White ? Square((int)ep - 8) : Square((int)ep + 8);
        Square kSQ = KingSQ(c);
        U64 enemyQueenRook = Rooks(~c) | Queens(~c);
        if ((enemyQueenRook)&MASK_RANK[square_rank(kSQ)])
        {
            Piece ourPawn = makePiece(PAWN, c);
            Piece theirPawn = makePiece(PAWN, ~c);
            removePiece<false>(ourPawn, sq);
            removePiece<false>(theirPawn, tP);
            placePiece<false>(ourPawn, ep);
            if (!((RookAttacks(kSQ, All()) & (enemyQueenRook))))
                moves |= (1ULL << ep);
            placePiece<false>(ourPawn, sq);
            placePiece<false>(theirPawn, tP);
            removePiece<false>(ourPawn, ep);
        }
        else
        {
            moves |= (1ULL << ep);
        }
    }
    return moves;
}

U64 Board::LegalKnightMoves(Square sq)
{
    if (pinD & (1ULL << sq) || pinHV & (1ULL << sq))
        return 0ULL;
    return KnightAttacks(sq) & enemyEmptyBB & checkMask;
}

U64 Board::LegalBishopMoves(Square sq)
{
    if (pinHV & (1ULL << sq))
        return 0ULL;
    if (pinD & (1ULL << sq))
        return BishopAttacks(sq, occAll) & enemyEmptyBB & pinD & checkMask;
    return BishopAttacks(sq, occAll) & enemyEmptyBB & checkMask;
}

U64 Board::LegalRookMoves(Square sq)
{
    if (pinD & (1ULL << sq))
        return 0ULL;
    if (pinHV & (1ULL << sq))
        return RookAttacks(sq, occAll) & enemyEmptyBB & pinHV & checkMask;
    return RookAttacks(sq, occAll) & enemyEmptyBB & checkMask;
}

U64 Board::LegalQueenMoves(Square sq)
{
    return LegalRookMoves(sq) | LegalBishopMoves(sq);
}

U64 Board::LegalKingMoves(Square sq)
{
    return KingAttacks(sq) & enemyEmptyBB & ~seen;
}

U64 Board::LegalKingMovesCastling(Color c, Square sq)
{
    U64 moves = KingAttacks(sq) & enemyEmptyBB & ~seen;

    if (c == White)
    {
        if (castlingRights & wk && !(WK_CASTLE_MASK & occAll) && moves & (1ULL << SQ_F1) && !(seen & (1ULL << SQ_G1)))
        {
            moves |= (1ULL << SQ_G1);
        }
        if (castlingRights & wq && !(WQ_CASTLE_MASK & occAll) && moves & (1ULL << SQ_D1) && !(seen & (1ULL << SQ_C1)))
        {
            moves |= (1ULL << SQ_C1);
        }
    }
    else
    {
        if (castlingRights & bk && !(BK_CASTLE_MASK & occAll) && moves & (1ULL << SQ_F8) && !(seen & (1ULL << SQ_G8)))
        {
            moves |= (1ULL << SQ_G8);
        }
        if (castlingRights & bq && !(BQ_CASTLE_MASK & occAll) && moves & (1ULL << SQ_D8) && !(seen & (1ULL << SQ_C8)))
        {
            moves |= (1ULL << SQ_C8);
        }
    }
    return moves;
}

Movelist Board::legalmoves()
{
    Movelist movelist{};
    movelist.size = 0;

    init(sideToMove, KingSQ(sideToMove));
    if (doubleCheck < 2)
    {
        U64 pawns_mask = Pawns(sideToMove);
        U64 knights_mask = Knights(sideToMove);
        U64 bishops_mask = Bishops(sideToMove);
        U64 rooks_mask = Rooks(sideToMove);
        U64 queens_mask = Queens(sideToMove);

        const bool noEP = enPassantSquare == NO_SQ;

        while (pawns_mask)
        {
            Square from = poplsb(pawns_mask);
            U64 moves = noEP ? LegalPawnMoves(sideToMove, from) : LegalPawnMovesEP(sideToMove, from, enPassantSquare);
            while (moves)
            {
                Square to = poplsb(moves);
                if (square_rank(to) == 7 || square_rank(to) == 0)
                {
                    movelist.Add(make(QUEEN, from, to, true));
                    movelist.Add(make(ROOK, from, to, true));
                    movelist.Add(make(KNIGHT, from, to, true));
                    movelist.Add(make(BISHOP, from, to, true));
                }
                else
                {
                    movelist.Add(make(PAWN, from, to, false));
                }
            }
        }
        while (knights_mask)
        {
            Square from = poplsb(knights_mask);
            U64 moves = LegalKnightMoves(from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(KNIGHT, from, to, false));
            }
        }
        while (bishops_mask)
        {
            Square from = poplsb(bishops_mask);
            U64 moves = LegalBishopMoves(from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(BISHOP, from, to, false));
            }
        }
        while (rooks_mask)
        {
            Square from = poplsb(rooks_mask);
            U64 moves = LegalRookMoves(from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(ROOK, from, to, false));
            }
        }
        while (queens_mask)
        {
            Square from = poplsb(queens_mask);
            U64 moves = LegalQueenMoves(from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(QUEEN, from, to, false));
            }
        }
    }

    Square from = KingSQ(sideToMove);
    U64 moves = !castlingRights || checkMask != DEFAULT_CHECKMASK ? LegalKingMoves(from)
                                                                  : LegalKingMovesCastling(sideToMove, from);
    while (moves)
    {
        Square to = poplsb(moves);
        movelist.Add(make(KING, from, to, false));
    }
    return movelist;
}

Movelist Board::capturemoves()
{
    Movelist movelist{};
    movelist.size = 0;

    init(sideToMove, KingSQ(sideToMove));
    if (doubleCheck < 2)
    {
        U64 pawns_mask = Pawns(sideToMove);
        U64 knights_mask = Knights(sideToMove);
        U64 bishops_mask = Bishops(sideToMove);
        U64 rooks_mask = Rooks(sideToMove);
        U64 queens_mask = Queens(sideToMove);
        U64 enemy = occEnemy;
        while (pawns_mask)
        {
            Square from = poplsb(pawns_mask);
            U64 moves = LegalPawnNoisy(sideToMove, from, enPassantSquare);
            while (moves)
            {
                Square to = poplsb(moves);
                if (square_rank(to) == 7 || square_rank(to) == 0)
                {
                    movelist.Add(make(QUEEN, from, to, true));
                    movelist.Add(make(ROOK, from, to, true));
                    movelist.Add(make(KNIGHT, from, to, true));
                    movelist.Add(make(BISHOP, from, to, true));
                }
                else
                {
                    movelist.Add(make(PAWN, from, to, false));
                }
            }
        }
        while (knights_mask)
        {
            Square from = poplsb(knights_mask);
            U64 moves = LegalKnightMoves(from) & enemy;
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(KNIGHT, from, to, false));
            }
        }
        while (bishops_mask)
        {
            Square from = poplsb(bishops_mask);
            U64 moves = LegalBishopMoves(from) & enemy;
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(BISHOP, from, to, false));
            }
        }
        while (rooks_mask)
        {
            Square from = poplsb(rooks_mask);
            U64 moves = LegalRookMoves(from) & enemy;
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(ROOK, from, to, false));
            }
        }
        while (queens_mask)
        {
            Square from = poplsb(queens_mask);
            U64 moves = LegalQueenMoves(from) & enemy;
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(QUEEN, from, to, false));
            }
        }
    }
    Square from = KingSQ(sideToMove);
    U64 moves = LegalKingCaptures(from);
    while (moves)
    {
        Square to = poplsb(moves);
        movelist.Add(make(KING, from, to, false));
    }
    return movelist;
}

void Board::makeNullMove()
{
    State store = State(enPassantSquare, castlingRights, halfMoveClock, None, hashKey);
    prevStates.Add(store);
    sideToMove = ~sideToMove;
    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);
    enPassantSquare = NO_SQ;
    fullMoveNumber++;
}

void Board::unmakeNullMove()
{
    State restore = prevStates.Get();
    enPassantSquare = restore.enPassant;
    castlingRights = restore.castling;
    halfMoveClock = restore.halfMove;
    hashKey = restore.h;

    fullMoveNumber--;
    sideToMove = ~sideToMove;
}
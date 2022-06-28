#include <algorithm>
#include <locale>

#include "board.h"
#include "helper.h"
#include "sliders.hpp"

using namespace Chess_Lookup::Fancy;

void Board::initializeLookupTables() {
    //initialize squares between table
    U64 sqs;
    for (int sq1 = 0; sq1 <= 63; ++sq1) {
        for (int sq2 = 0; sq2 <= 63; ++sq2) {
            sqs = (1ULL << sq1) | (1ULL << sq2);
            if (square_file(Square(sq1)) == square_file(Square(sq2))
                || square_rank(Square(sq1)) == square_rank(Square(sq2)))
                SQUARES_BETWEEN_BB[sq1][sq2] =
                RookAttacks(Square(sq1), sqs) & RookAttacks(Square(sq2), sqs);

            else if (diagonal_of(Square(sq1)) == diagonal_of(Square(sq2))
                || anti_diagonal_of(Square(sq1)) == anti_diagonal_of(Square(sq2)))
                SQUARES_BETWEEN_BB[Square(sq1)][sq2] =
                BishopAttacks(Square(sq1), sqs) & BishopAttacks(Square(sq2), sqs);
        }
    }
}

void Board::activate(int inputNum) {
    for (int i = 0; i < HIDDEN_BIAS; i++) {
        accumulator[i] += inputWeights[inputNum * HIDDEN_BIAS + i];
    }
}

void Board::deactivate(int inputNum) {
    for (int i = 0; i < HIDDEN_BIAS; i++) {
        accumulator[i] -= inputWeights[inputNum * HIDDEN_BIAS + i];
    }
}

Board::Board() {
    initializeLookupTables();
}

U64 Board::zobristHash() {
    U64 hash = 0ULL;
    U64 wPieces = Us(White);
    U64 bPieces = Us(Black);
    // Piece hashes
    while (wPieces) {
        Square sq = poplsb(wPieces);
        hash ^= updateKeyPiece(pieceAtB(sq), sq);
    }
    while (bPieces) {
        Square sq = poplsb(bPieces);
        hash ^= updateKeyPiece(pieceAtB(sq), sq);
    }
    // Ep hash
    U64 ep_hash = 0ULL;
    if (enPassantSquare != NO_SQ) {
        ep_hash = updateKeyEnPassant(enPassantSquare);
    }
    // Turn hash
    U64 turn_hash = sideToMove == White ? RANDOM_ARRAY[780] : 0;
    // Castle hash
    U64 cast_hash = updateKeyCastling();

    return hash ^ cast_hash ^ turn_hash ^ ep_hash;
}

Piece Board::pieceAtBB(Square sq) {
    if (Bitboards[WhitePawn] & (1ULL << sq)) return WhitePawn;
    if (Bitboards[WhiteKnight] & (1ULL << sq)) return WhiteKnight;
    if (Bitboards[WhiteBishop] & (1ULL << sq)) return WhiteBishop;
    if (Bitboards[WhiteRook] & (1ULL << sq)) return WhiteRook;
    if (Bitboards[WhiteQueen] & (1ULL << sq)) return WhiteQueen;
    if (Bitboards[WhiteKing] & (1ULL << sq)) return WhiteKing;
    if (Bitboards[BlackPawn] & (1ULL << sq)) return BlackPawn;
    if (Bitboards[BlackKnight] & (1ULL << sq)) return BlackKnight;
    if (Bitboards[BlackBishop] & (1ULL << sq)) return BlackBishop;
    if (Bitboards[BlackRook] & (1ULL << sq)) return BlackRook;
    if (Bitboards[BlackQueen] & (1ULL << sq)) return BlackQueen;
    if (Bitboards[BlackKing] & (1ULL << sq)) return BlackKing;
    return None;
}

Piece Board::pieceAtB(Square sq) {
    return board[sq];
}

void Board::applyFen(std::string fen) {
    for (int i = 0; i < 12; i++) {
        Bitboards[i] = 0ULL;
    }
    std::vector<std::string> params = split_input(fen);
    std::string position = params[0];
    std::string move_right = params[1];
    std::string castling = params[2];
    std::string en_passant = params[3];
    // default
    std::string half_move_clock = "0";
    std::string full_move_counter = "1";
    if (params.size() > 4) {
        half_move_clock = params[4];
        full_move_counter = params[5];
    }
    sideToMove = (move_right == "w") ? White : Black;

    std::map<char, Piece> piece_to_int =
    {
    { 'P', WhitePawn },
    { 'N', WhiteKnight },
    { 'B', WhiteBishop },
    { 'R', WhiteRook },
    { 'Q', WhiteQueen },
    { 'K', WhiteKing },
    { 'p', BlackPawn },
    { 'n', BlackKnight },
    { 'b', BlackBishop },
    { 'r', BlackRook },
    { 'q', BlackQueen },
    { 'k', BlackKing },
    };

    for (int i = 0; i < HIDDEN_BIAS; i++) {
        accumulator[i] = hiddenBias[i];
    }

    Square square = Square(56);
    for (int index = 0; index < (int)position.size(); index++) {
        char curr = position[index];
        if (piece_to_int.find(curr) != piece_to_int.end()) {
            Piece piece = piece_to_int[curr];
            placePiece(piece, square);
            square = Square(square + 1);
        }
        else if (curr == '/') square = Square(square - 16);
        else if (isdigit(curr)){
            for (int i = 0; i < (int)curr - '0'; i++) {
                board[square + i] = None;
            }
            square = Square(square + (curr - '0'));
        } 
    }

    castlingRights = 0;
    for (size_t i = 0; i < castling.size(); i++) {
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
    if (en_passant == "-") {
        enPassantSquare = NO_SQ;
    }
    else {
        char letter = en_passant[0];
        int file = letter - 96;
        int rank = en_passant[1] - 48;
        enPassantSquare = Square((rank - 1) * 8 + file - 1);
    }
    // half_move_clock
    halfMoveClock = std::stoi(half_move_clock);

    // full_move_counter actually half moves
    fullMoveNumber = std::stoi(full_move_counter) * 2;

    prevStates.size = 0;
    hashKey = zobristHash();
}

std::string Board::getFen() {
    int sq;
    char letter;
    std::string fen = "";
    for (int rank = 7; rank >= 0; rank--) {
        int free_space = 0;
        for (int file = 0; file < 8; file++) {
            sq = rank * 8 + file;
            Piece piece = pieceAtB(Square(sq));
            if (piece != None) {
                if (free_space) {
                    fen += std::to_string(free_space);
                    free_space = 0;
                }
                letter = pieceToChar[piece];
                fen += letter;
            }
            else {
                free_space++;
            }
        }
        if (free_space != 0) {
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
        fen += " - ";
    if (enPassantSquare == NO_SQ)
        fen += " - ";
    else
        fen += " " + squareToString[enPassantSquare] + " ";

    fen += std::to_string(halfMoveClock);
    fen += " " + std::to_string(fullMoveNumber / 2);
    return fen;
}

void Board::printBitboard(U64 bb) {
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

void Board::print() {
    for (int i = 63; i >= 0; i -= 8)
    {
        std::cout << " " << pieceToChar[board[i - 7]]
            << " " << pieceToChar[board[i - 6]]
            << " " << pieceToChar[board[i - 5]]
            << " " << pieceToChar[board[i - 4]]
            << " " << pieceToChar[board[i - 3]]
            << " " << pieceToChar[board[i - 2]]
            << " " << pieceToChar[board[i - 1]]
            << " " << pieceToChar[board[i]]
            << " " << std::endl;
    }
    std::cout << '\n' << std::endl;
    std::cout << "Side to move: " << (int)sideToMove << std::endl;
    std::cout << "Castling rights: " << (int)castlingRights << std::endl;
    std::cout << "Halfmoves: " << (int)halfMoveClock << std::endl;
    std::cout << "Fullmoves: " << (int)fullMoveNumber / 2 << std::endl;
    std::cout << "EP: " << (int)enPassantSquare << std::endl;
    std::cout << "Hash: " << hashKey << std::endl;
}

Piece Board::makePiece(PieceType type, Color c) {
    if (type == NONETYPE) return None;
    return Piece(type + 6 * c);
}

PieceType Board::piece_type(Piece piece) {
    if (piece == None) return NONETYPE;
    return PieceType(piece % 6);
}

std::string Board::printMove(Move& move) {
    std::string m = "";
    m += squareToString[move.from()];
    m += squareToString[move.to()];
    if (move.promoted()) m += PieceToPromPiece[Piece(move.piece())];
    return m;
}

bool Board::isRepetition(int draw) {
    for (int i = fullMoveNumber - 2; i >= 0 && i >= fullMoveNumber - halfMoveClock; i -= 2) {
        if (hashHistory[i] == hashKey) return true;
    }
    return false;
}

bool Board::nonPawnMat(Color c) {
    return Knights(c) | Bishops(c) | Rooks(c) | Queens(c);
}

U64 Board::PawnAttacks(Square sq, Color c) {
    return PAWN_ATTACKS_TABLE[c][sq];
}

U64 Board::KnightAttacks(Square sq) {
    return KNIGHT_ATTACKS_TABLE[sq];
}

U64 Board::BishopAttacks(Square sq, U64 occupied) {
    return GetBishopAttacks(sq, occupied);
}

U64 Board::RookAttacks(Square sq, U64 occupied) {
    return GetRookAttacks(sq, occupied);
}

U64 Board::QueenAttacks(Square sq, U64 occupied) {
    return GetQueenAttacks(sq, occupied);
}

U64 Board::KingAttacks(Square sq) {
    return KING_ATTACKS_TABLE[sq];
}

U64 Board::Pawns(Color c) {
    return Bitboards[PAWN + c * 6];
}
U64 Board::Knights(Color c) {
    return Bitboards[KNIGHT + c * 6];
}
U64 Board::Bishops(Color c) {
    return Bitboards[BISHOP + c * 6];
}
U64 Board::Rooks(Color c) {
    return Bitboards[ROOK + c * 6];
}
U64 Board::Queens(Color c) {
    return Bitboards[QUEEN + c * 6];
}
U64 Board::Kings(Color c) {
    return Bitboards[KING + c * 6];
}

void Board::removePieceSimple(Piece piece, Square sq) {
    Bitboards[piece] &= ~(1ULL << sq);
    board[sq] = None;
}

void Board::placePieceSimple(Piece piece, Square sq) {
    Bitboards[piece] |= (1ULL << sq);
    board[sq] = piece;
}


void Board::removePiece(Piece piece, Square sq) {
    Bitboards[piece] &= ~(1ULL << sq);
    board[sq] = None;
    deactivate(sq + piece * 64);
}

void Board::placePiece(Piece piece, Square sq) {
    Bitboards[piece] |= (1ULL << sq);
    board[sq] = piece;
    activate(sq + piece * 64);
}

U64 Board::updateKeyPiece(Piece piece, Square sq) {
    return RANDOM_ARRAY[64 * (piece_type(piece) * 2 + ((piece/6) ^ 1)) + sq];
}

U64 Board::updateKeyEnPassant(Square sq) {
    return RANDOM_ARRAY[772 + square_file(sq)];
}

U64 Board::updateKeyCastling() {
    return castlingKey[castlingRights];
}

U64 Board::updateKeySideToMove() {
    return RANDOM_ARRAY[780];
}

Square Board::KingSQ(Color c) {
    return Square(bsf(Kings(c)));
}

U64 Board::Us(Color c) {
    return Bitboards[PAWN + c * 6] | Bitboards[KNIGHT + c * 6] |
        Bitboards[BISHOP + c * 6] | Bitboards[ROOK + c * 6] |
        Bitboards[QUEEN + c * 6] | Bitboards[KING + c * 6];
}

U64 Board::Enemy(Color c) {
    return Us(~c);
}

U64 Board::All() {
    return Us(White) | Us(Black);
}

U64 Board::Empty() {
    return ~All();
}

U64 Board::EnemyEmpty(Color c) {
    return ~Us(c);
}

U64 Board::DoCheckmask(Color c, Square sq) {
    U64 Occ = occAll;
    U64 checks = 0ULL;
    U64 pawn_mask = Pawns(~c) & PawnAttacks(sq, c);
    U64 knight_mask = Knights(~c) & KnightAttacks(sq);
    U64 bishop_mask = (Bishops(~c) | Queens(~c)) & BishopAttacks(sq, Occ);
    U64 rook_mask = (Rooks(~c) | Queens(~c)) & RookAttacks(sq, Occ);
    doubleCheck = 0;
    if (pawn_mask) {
        checks |= pawn_mask;
        doubleCheck++;
    }
    if (knight_mask) {
        checks |= knight_mask;
        doubleCheck++;
    }
    if (bishop_mask) {
        if (popcount(bishop_mask) > 1)
            doubleCheck++;

        int8_t index = bsf(bishop_mask);
        checks |= SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        doubleCheck++;
    }
    if (rook_mask) {
        if (popcount(rook_mask) > 1)
            doubleCheck++;

        int8_t index = bsf(rook_mask);
        checks |= SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        doubleCheck++;
    }
    return checks;
}

void Board::DoPinMask(Color c, Square sq) {
    U64 them = Enemy(c);
    U64 rook_mask = (Rooks(~c) | Queens(~c)) & RookAttacks(sq, them);
    U64 bishop_mask = (Bishops(~c) | Queens(~c)) & BishopAttacks(sq, them);

    pinD = 0ULL;
    pinHV = 0ULL;
    while (rook_mask) {
        Square index = poplsb(rook_mask);
        U64 possible_pin = (SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & Us(c)) == 1)
            pinHV |= possible_pin;
    }
    while (bishop_mask) {
        Square index = poplsb(bishop_mask);
        U64 possible_pin = (SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & Us(c)) == 1)
            pinD |= possible_pin;
    }
}

void Board::init(Color c, Square sq) {
    occAll = All();
    occWhite = Us(White);
    occBlack = Us(Black);
    enemyEmptyBB = EnemyEmpty(c);
    U64 newMask = DoCheckmask(c, sq);
    checkMask = newMask ? newMask : 18446744073709551615ULL;
    DoPinMask(c, sq);

}

bool Board::isSquareAttacked(Color c, Square sq) {
    if (Pawns(c) & PawnAttacks(sq, ~c)) return true;
    if (Knights(c) & KnightAttacks(sq)) return true;
    if ((Bishops(c) | Queens(c)) & BishopAttacks(sq, All())) return true;
    if ((Rooks(c) | Queens(c)) & RookAttacks(sq, All())) return true;
    if (Kings(c) & KingAttacks(sq) ) return true;
    return false;
}

U64 Board::allAttackers(Square sq, U64 occupiedBB) {
    return attackersForSide(White, sq, occupiedBB) | 
                attackersForSide(Black, sq, occupiedBB);
}

U64 Board::attackersForSide(Color attackerColor, Square sq, U64 occupiedBB) {
    U64 attackingBishops = Bishops(attackerColor);
    U64 attackingRooks   = Rooks(attackerColor);
    U64 attackingQueens  = Queens(attackerColor);
    U64 attackingKnights = Knights(attackerColor);
    U64 attackingKing    = Kings(attackerColor);
    U64 attackingPawns   = Pawns(attackerColor);

    U64 interCardinalRays = BishopAttacks(sq, occupiedBB);
    U64 cardinalRaysRays  = RookAttacks(sq, occupiedBB);

    U64 attackers = interCardinalRays & (attackingBishops | attackingQueens);
    attackers |= cardinalRaysRays & (attackingRooks | attackingQueens);
	attackers |= KnightAttacks(sq) & attackingKnights;
	attackers |= KingAttacks(sq) & attackingKing;
	attackers |= PawnAttacks(sq, ~attackerColor) & attackingPawns;
	return attackers;
} 

U64 Board::PawnPush(Color c, Square sq) {
    return (1ULL << (sq + (c * -2 + 1) * 8));
}
U64 Board::PawnPush2(Color c, Square sq, U64 push) {
    if (c == White && square_rank(sq) == 1) return (push << 8) & ~occAll;
    if (c == Black && square_rank(sq) == 6) return (push >> 8) & ~occAll;
    return 0ULL;
}

U64 Board::LegalPawnNoisy(Color c, Square sq, Square ep) {
    U64 enemy = Enemy(c);
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the checkmask
    if (pinD & (1ULL << sq)) return PawnAttacks(sq, c) & pinD & checkMask & (enemy | (1ULL << ep));
    // If we are pinned horizontally/vertically we can do no captures
    if (pinHV & (1ULL << sq)) return 0ULL;
    U64 attacks = PawnAttacks(sq, c);
    U64 pawnPromote = 0ULL;
    if ((square_rank(sq) == 1 && c == Black) || (square_rank(sq) == 6 && c == White)) {
        pawnPromote = PawnPush(c, sq) & ~occAll;
        pawnPromote |= PawnPush2(c, sq, pawnPromote);
    }
    return ((attacks & enemy) | pawnPromote) & checkMask;
}

U64 Board::LegalKingCaptures(Color c, Square sq) {
    U64 moves = KingAttacks(sq) & Enemy(c);
    U64 final_moves = 0ULL;
    Piece k = makePiece(KING, c);

    removePieceSimple(k, sq);
    while (moves) {
        Square index = poplsb(moves);
        if (isSquareAttacked(~c, index)) continue;
        final_moves |= (1ULL << index);
    }
    placePieceSimple(k, sq);
    return final_moves;
}

U64 Board::LegalPawnMoves(Color c, Square sq, Square ep) {
    U64 enemy = Enemy(c);
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the checkmask
    if (pinD & (1ULL << sq)) return PawnAttacks(sq, c) & pinD & checkMask & (enemy | (1ULL << ep));
    // Calculate pawn pushs
    U64 push = PawnPush(c, sq) & ~occAll;
    push |= PawnPush2(c, sq, push);
    // If we are pinned horizontally we can do no moves but if we are pinned vertically we can only do pawn pushs
    if (pinHV & (1ULL << sq)) return push & pinHV & checkMask;
    U64 attacks = PawnAttacks(sq, c);
    // If we are in check and  the en passant square lies on our attackmask and the en passant piece gives check
    // return the ep mask as a move square
    if (checkMask != 18446744073709551615ULL && ep != NO_SQ && attacks & (1ULL << ep) && checkMask & (1ULL << (ep - (c * -2 + 1) * 8))) return (attacks & (1ULL << ep));
    // If we are in check we can do all moves that are on the checkmask
    if (checkMask != 18446744073709551615ULL) return ((attacks & enemy) | push) & checkMask;

    U64 moves = ((attacks & enemy) | push) & checkMask;
    // We need to make extra sure that ep moves dont leave the king in check
    // 7k/8/8/K1Pp3r/8/8/8/8 w - d6 0 1 
    // Horizontal rook pins our pawn through another pawn, our pawn can push but not take enpassant 
    // remove both the pawn that made the push and our pawn that could take in theory and check if that would give check
    if (ep != NO_SQ && square_distance(sq, ep) == 1 && (1ULL << ep) & attacks) {
        Piece ourPawn = makePiece(PAWN, c);
        Piece theirPawn = makePiece(PAWN, ~c);
        Square kSQ = KingSQ(c);
        removePieceSimple(ourPawn, sq);
        removePieceSimple(theirPawn, Square((int)ep - (c * -2 + 1) * 8));
        placePieceSimple(ourPawn, ep);
        if (!((RookAttacks(kSQ, All()) & (Rooks(~c) | Queens(~c))))) moves |= (1ULL << ep);
        placePieceSimple(ourPawn, sq);
        placePieceSimple(theirPawn, Square((int)ep - (c * -2 + 1) * 8));
        removePieceSimple(ourPawn, ep);
    }
    return moves;
}

U64 Board::LegalKnightMoves(Color c, Square sq) {
    if (pinD & (1ULL << sq) || pinHV & (1ULL << sq)) return 0ULL;
    return KnightAttacks(sq) & enemyEmptyBB & checkMask;
}

U64 Board::LegalBishopMoves(Color c, Square sq) {
    if (pinHV & (1ULL << sq)) return 0ULL;
    if (pinD & (1ULL << sq)) return BishopAttacks(sq, occAll) & enemyEmptyBB & pinD & checkMask;
    return BishopAttacks(sq, occAll) & enemyEmptyBB & checkMask;
}

U64 Board::LegalRookMoves(Color c, Square sq) {
    if (pinD & (1ULL << sq)) return 0ULL;
    if (pinHV & (1ULL << sq)) return RookAttacks(sq, occAll) & enemyEmptyBB & pinHV & checkMask;
    return RookAttacks(sq, occAll) & enemyEmptyBB & checkMask;
}

U64 Board::LegalQueenMoves(Color c, Square sq) {
    return LegalRookMoves(c, sq) | LegalBishopMoves(c, sq);
}

U64 Board::LegalKingMoves(Color c, Square sq) {
    U64 moves = KingAttacks(sq) & enemyEmptyBB;
    U64 final_moves = 0ULL;
    Piece k = makePiece(KING, c);

    removePieceSimple(k, sq);
    while (moves) {
        Square index = poplsb(moves);
        if (isSquareAttacked(~c, index)) continue;
        final_moves |= (1ULL << index);
    }
    placePieceSimple(k, sq);

    if (checkMask == 18446744073709551615ULL) {
        if (c == White) {
            if (castlingRights & wk
                && !(WK_CASTLE_MASK & occAll)
                && final_moves & (1ULL << SQ_F1) //!isSquareAttacked(Black, SQ_F1)
                && !isSquareAttacked(Black, SQ_G1)) {
                final_moves |= (1ULL << SQ_G1);
            }
            if (castlingRights & wq
                && !(WQ_CASTLE_MASK & occAll)
                && final_moves & (1ULL << SQ_D1) //!isSquareAttacked(Black, SQ_D1)
                && !isSquareAttacked(Black, SQ_C1)) {
                final_moves |= (1ULL << SQ_C1);
            }
        }
        else {
            if (castlingRights & bk
                && !(BK_CASTLE_MASK & occAll)
                && final_moves & (1ULL << SQ_F8) //!isSquareAttacked(White, SQ_F8)
                && !isSquareAttacked(White, SQ_G8)) {
                final_moves |= (1ULL << SQ_G8);
            }
            if (castlingRights & bq
                && !(BQ_CASTLE_MASK & occAll)
                && final_moves & (1ULL << SQ_D8)  //!isSquareAttacked(White, SQ_D8)
                && !isSquareAttacked(White, SQ_C8)) {
                final_moves |= (1ULL << SQ_C8);
            }
        }
    }
    return final_moves;
}

Movelist Board::legalmoves() {
    Move move{};
    Movelist movelist{};
    movelist.size = 0;

    init(sideToMove, KingSQ(sideToMove));
    if (doubleCheck < 2) {
        U64 pawns_mask = Pawns(sideToMove);
        U64 knights_mask = Knights(sideToMove);
        U64 bishops_mask = Bishops(sideToMove);
        U64 rooks_mask = Rooks(sideToMove);
        U64 queens_mask = Queens(sideToMove);
        while (pawns_mask) {
            Square from = poplsb(pawns_mask);
            U64 moves = LegalPawnMoves(sideToMove, from, enPassantSquare);
            while (moves) {
                Square to = poplsb(moves);
                if (square_rank(to) == 7 || square_rank(to) == 0) {
                    movelist.Add(Move(QUEEN, from, to, true));
                    movelist.Add(Move(ROOK, from, to, true));
                    movelist.Add(Move(KNIGHT, from, to, true));
                    movelist.Add(Move(BISHOP, from, to, true));
                }
                else {
                    movelist.Add(Move(PAWN, from, to, false));
                }
            }
        }
        while (knights_mask) {
            Square from = poplsb(knights_mask);
            U64 moves = LegalKnightMoves(sideToMove, from);
            while (moves) {
                Square to = poplsb(moves);
                movelist.Add(Move(KNIGHT, from, to, false));
            }
        }
        while (bishops_mask) {
            Square from = poplsb(bishops_mask);
            U64 moves = LegalBishopMoves(sideToMove, from);
            while (moves) {
                Square to = poplsb(moves);
                movelist.Add(Move(BISHOP, from, to, false));
            }
        }
        while (rooks_mask) {
            Square from = poplsb(rooks_mask);
            U64 moves = LegalRookMoves(sideToMove, from);
            while (moves) {
                Square to = poplsb(moves);
                movelist.Add(Move(ROOK, from, to, false));
            }
        }
        while (queens_mask) {
            Square from = poplsb(queens_mask);
            U64 moves = LegalQueenMoves(sideToMove, from);
            while (moves) {
                Square to = poplsb(moves);
                movelist.Add(Move(QUEEN, from, to, false));
            }
        }

    }
    Square from = KingSQ(sideToMove);
    U64 moves = LegalKingMoves(sideToMove, from);
    while (moves) {
        Square to = poplsb(moves);
        movelist.Add(Move(KING, from, to, false));
    }
    return movelist;
}

Movelist Board::capturemoves() {
    Move move{};
    Movelist movelist{};
    movelist.size = 0;

    init(sideToMove, KingSQ(sideToMove));
    if (doubleCheck < 2) {
        U64 pawns_mask = Pawns(sideToMove);
        U64 knights_mask = Knights(sideToMove);
        U64 bishops_mask = Bishops(sideToMove);
        U64 rooks_mask = Rooks(sideToMove);
        U64 queens_mask = Queens(sideToMove);
        U64 enemy = Enemy(sideToMove);
        while (pawns_mask) {
            Square from = poplsb(pawns_mask);
            // U64 moves = LegalPawnMoves(sideToMove, from, enPassantSquare) & (enemy | RANK_1 | RANK_8);
            U64 moves = LegalPawnNoisy(sideToMove, from, enPassantSquare);
            while (moves) {
                Square to = poplsb(moves);
                if (square_rank(to) == 7 || square_rank(to) == 0) {
                    movelist.Add(Move(QUEEN, from, to, true));
                    movelist.Add(Move(ROOK, from, to, true));
                    movelist.Add(Move(KNIGHT, from, to, true));
                    movelist.Add(Move(BISHOP, from, to, true));
                }
                else {
                    movelist.Add(Move(PAWN, from, to, false));
                }
            }
        }
        while (knights_mask) {
            Square from = poplsb(knights_mask);
            U64 moves = LegalKnightMoves(sideToMove, from) & enemy;
            while (moves) {
                Square to = poplsb(moves);
                movelist.Add(Move(KNIGHT, from, to, false));
            }
        }
        while (bishops_mask) {
            Square from = poplsb(bishops_mask);
            U64 moves = LegalBishopMoves(sideToMove, from) & enemy;
            while (moves) {
                Square to = poplsb(moves);
                movelist.Add(Move(BISHOP, from, to, false));
            }
        }
        while (rooks_mask) {
            Square from = poplsb(rooks_mask);
            U64 moves = LegalRookMoves(sideToMove, from) & enemy;
            while (moves) {
                Square to = poplsb(moves);
                movelist.Add(Move(ROOK, from, to, false));
            }
        }
        while (queens_mask) {
            Square from = poplsb(queens_mask);
            U64 moves = LegalQueenMoves(sideToMove, from) & enemy;
            while (moves) {
                Square to = poplsb(moves);
                movelist.Add(Move(QUEEN, from, to, false));
            }
        }

    }
    Square from = KingSQ(sideToMove);
    U64 moves = LegalKingCaptures(sideToMove, from);
    while (moves) {
        Square to = poplsb(moves);
        movelist.Add(Move(KING, from, to, false));
    }
    return movelist;
}


void Board::makeMove(Move& move) {
    Piece piece = makePiece(move.piece(), sideToMove);
    Square from = move.from();
    Square to = move.to();
    Piece capture = board[to];

    hashHistory[fullMoveNumber] = hashKey;
    State store = State(enPassantSquare, castlingRights, halfMoveClock, capture, hashKey);
    prevStates.Add(store);

    halfMoveClock++;
    fullMoveNumber++;

    bool ep = to == enPassantSquare;

    if (enPassantSquare != NO_SQ) hashKey ^= updateKeyEnPassant(enPassantSquare);
    enPassantSquare = NO_SQ;

    if (move.piece() == KING) {
        if (sideToMove == White && from == SQ_E1 && to == SQ_G1 && castlingRights & wk) {
            removePiece(WhiteRook, SQ_H1);
            placePiece(WhiteRook, SQ_F1);

            hashKey ^= updateKeyPiece(WhiteRook, SQ_H1);
            hashKey ^= updateKeyPiece(WhiteRook, SQ_F1);
        }
        else if (sideToMove == White && from == SQ_E1 && to == SQ_C1 && castlingRights & wq) {
            removePiece(WhiteRook, SQ_A1);
            placePiece(WhiteRook, SQ_D1);

            hashKey ^= updateKeyPiece(WhiteRook, SQ_A1);
            hashKey ^= updateKeyPiece(WhiteRook, SQ_D1);
        }
        else if (sideToMove == Black && from == SQ_E8 && to == SQ_G8 && castlingRights & bk) {
            removePiece(BlackRook, SQ_H8);
            placePiece(BlackRook, SQ_F8);

            hashKey ^= updateKeyPiece(BlackRook, SQ_H8);
            hashKey ^= updateKeyPiece(BlackRook, SQ_F8);
        }
        else if (sideToMove == Black && from == SQ_E8 && to == SQ_C8 && castlingRights & bq) {
            removePiece(BlackRook, SQ_A8);
            placePiece(BlackRook, SQ_D8);

            hashKey ^= updateKeyPiece(BlackRook, SQ_A8);
            hashKey ^= updateKeyPiece(BlackRook, SQ_D8);
        }
        hashKey ^= updateKeyCastling();
        if (sideToMove == White) {
            castlingRights &= ~(wk | wq);
        }
        else {
            castlingRights &= ~(bk | bq);
        }
        hashKey ^= updateKeyCastling();
    }
    else if (move.piece() == ROOK) {
        hashKey ^= updateKeyCastling();
        if (sideToMove == White && from == SQ_A1 ) {
            castlingRights &= ~wq;
        }
        else if (sideToMove == White && from == SQ_H1) {
            castlingRights &= ~wk;
        }
        else if (sideToMove == Black && from == SQ_A8) {
            castlingRights &= ~bq;
        }
        else if (sideToMove == Black && from == SQ_H8) {
            castlingRights &= ~bk;
        }
        hashKey ^= updateKeyCastling();
    }
    else if (move.piece() == PAWN) {
        halfMoveClock = 0;
        if (ep) {
            removePiece(makePiece(PAWN, ~sideToMove), Square(to - (sideToMove * -2 + 1) * 8));
            hashKey ^= updateKeyPiece(makePiece(PAWN, ~sideToMove), Square(to - (sideToMove * -2 + 1) * 8));
        }
        else if (std::abs(from - to) == 16) {
            U64 epMask = PawnAttacks(Square(to - (sideToMove * -2 + 1) * 8), sideToMove);
            if (epMask & Pawns(~sideToMove)) {
                enPassantSquare = Square(to - (sideToMove * -2 + 1) * 8);
                hashKey ^= updateKeyEnPassant(enPassantSquare);
            }
        }
    }

    if (capture != None) {
        halfMoveClock = 0;
        removePiece(capture, to);
        hashKey ^= updateKeyPiece(capture, to);
        if (capture % 6 == ROOK) {
            hashKey ^= updateKeyCastling();
            if (to == SQ_A1) {
                castlingRights &= ~wq;
            }
            else if (to == SQ_H1) {
                castlingRights &= ~wk;
            }
            else if (to == SQ_A8) {
                castlingRights &= ~bq;
            }
            else if (to == SQ_H8) {
                castlingRights &= ~bk;
            }
            hashKey ^= updateKeyCastling();
        }
    }

    if (move.promoted()) {
        halfMoveClock = 0;
        removePiece(makePiece(PAWN, sideToMove), from);
        placePiece(piece, to);

        hashKey ^= updateKeyPiece(makePiece(PAWN, sideToMove), from);
        hashKey ^= updateKeyPiece(piece, to);
    }
    else {
        removePiece(piece, from);
        hashKey ^= updateKeyPiece(piece, from);
        placePiece(piece, to);
        hashKey ^= updateKeyPiece(piece, to);

    }
    sideToMove = ~sideToMove;
    hashKey ^= updateKeySideToMove();

}

void Board::unmakeMove(Move& move) {
    State restore = prevStates.Get();
    enPassantSquare = restore.enPassant;
    castlingRights = restore.castling;
    halfMoveClock = restore.halfMove;
    Piece capture = restore.capturedPiece;
    hashKey = restore.h;
    fullMoveNumber--;

    Square from = move.from();
    Square to = move.to();
    bool promotion = move.promoted();
    sideToMove = ~sideToMove;
    Piece piece = makePiece(move.piece(), sideToMove);

    if (promotion) {
        removePiece(piece, to);
        placePiece(makePiece(PAWN, sideToMove), from);
        if (capture != None) {
            placePiece(capture, to);
        }
        return;
    }
    else {
        removePiece(piece, to);
        placePiece(piece, from);
    }

    if (to == enPassantSquare && move.piece() == PAWN) {
        int8_t offset = sideToMove == White ? -8 : 8;
        placePiece(makePiece(PAWN, ~sideToMove), Square(enPassantSquare + offset));
    }
    else if (capture != None) {
        placePiece(capture, to);
    }
    else {
        if (move.piece() == KING) {
            if (from == SQ_E1 && to == SQ_G1 && castlingRights & wk) {
                removePiece(WhiteRook, SQ_F1);
                placePiece(WhiteRook, SQ_H1);
            }
            else if (from == SQ_E1 && to == SQ_C1 && castlingRights & wq) {
                removePiece(WhiteRook, SQ_D1);
                placePiece(WhiteRook, SQ_A1);
            }

            else if (from == SQ_E8 && to == SQ_G8 && castlingRights & bk) {
                removePiece(BlackRook, SQ_F8);
                placePiece(BlackRook, SQ_H8);
            }
            else if (from == SQ_E8 && to == SQ_C8 && castlingRights & bq) {
                removePiece(BlackRook, SQ_D8);
                placePiece(BlackRook, SQ_A8);
            }
        }
    }
}

void Board::makeNullMove() {
    State store = State(enPassantSquare, castlingRights, halfMoveClock, None, hashKey);
    prevStates.Add(store);
    sideToMove = ~sideToMove;
    hashKey = zobristHash();
    enPassantSquare = NO_SQ;
    fullMoveNumber++;
}

void Board::unmakeNullMove() {
    State restore = prevStates.Get();
    enPassantSquare = restore.enPassant;
    castlingRights = restore.castling;
    halfMoveClock = restore.halfMove;
    hashKey = restore.h;
    fullMoveNumber--;
    sideToMove = ~sideToMove;

}
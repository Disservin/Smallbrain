#include "board.h"

Board::Board()
{
    initializeLookupTables();
    stateHistory.reserve(MAX_PLY);
    hashHistory.reserve(512);
    accumulatorStack.reserve(MAX_PLY);

    applyFen(DEFAULT_POS, true);

    sideToMove = White;
    enPassantSquare = NO_SQ;
    castlingRights = wk | wq | bk | bq;
    halfMoveClock = 0;
    fullMoveNumber = 1;

    pinHV = 0;
    pinD = 0;
    doubleCheck = 0;
    checkMask = DEFAULT_CHECKMASK;
    seen = 0;

    occEnemy = Enemy<White>();
    occUs = Us<White>();
    occAll = All();
    enemyEmptyBB = EnemyEmpty(White);

    hashKey = zobristHash();

    std::fill(std::begin(board), std::end(board), None);
}

void Board::accumulate()
{
    for (int i = 0; i < HIDDEN_BIAS; i++)
        accumulator[i] = hiddenBias[i];

    for (Square i = SQ_A1; i < NO_SQ; i++)
    {
        Piece p = board[i];
        bool input = p != None;
        if (!input)
            continue;
        int j = i + p * 64;
        NNUE::activate(accumulator, j);
    }
}

Piece Board::pieceAtBB(Square sq)
{
    for (Piece p = WhitePawn; p < None; p++)
    {
        if (Bitboards[p] & (1ULL << sq))
            return p;
    }
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
    for (int index = 0; index < static_cast<int>(position.size()); index++)
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
            for (int i = 0; i < static_cast<int>(curr - '0'); i++)
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

    stateHistory.clear();
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
    std::cout << "Side to move: " << static_cast<int>(sideToMove) << std::endl;
    std::cout << "Castling rights: " << static_cast<int>(castlingRights) << std::endl;
    std::cout << "Halfmoves: " << static_cast<int>(halfMoveClock) << std::endl;
    std::cout << "Fullmoves: " << static_cast<int>(fullMoveNumber) / 2 << std::endl;
    std::cout << "EP: " << static_cast<int>(enPassantSquare) << std::endl;
    std::cout << "Hash: " << hashKey << std::endl;
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

Square Board::KingSQ(Color c)
{
    return bsf(Kings(c));
}

U64 Board::Enemy(Color c)
{
    return Us(~c);
}

U64 Board::Us(Color c)
{
    return Bitboards[PAWN + c * 6] | Bitboards[KNIGHT + c * 6] | Bitboards[BISHOP + c * 6] | Bitboards[ROOK + c * 6] |
           Bitboards[QUEEN + c * 6] | Bitboards[KING + c * 6];
}

U64 Board::EnemyEmpty(Color c)
{
    return ~Us(c);
}
U64 Board::All()
{
    return Us<White>() | Us<Black>();
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

Color Board::colorOf(Square loc)
{
    return Color((pieceAtB(loc) / 6));
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

template <bool updateNNUE> void Board::makeMove(Move move)
{
    PieceType pt = piece(move);
    Piece p = makePiece(pt, sideToMove);
    Square from_sq = from(move);
    Square to_sq = to(move);
    Piece capture = board[to_sq];

    // *****************************
    // STORE STATE HISTORY
    // *****************************

    hashHistory.emplace_back(hashKey);

    State store = State(enPassantSquare, castlingRights, halfMoveClock, capture);
    stateHistory.push_back(store);

    if constexpr (updateNNUE)
        accumulatorStack.emplace_back(accumulator);

    halfMoveClock++;
    fullMoveNumber++;

    bool ep = to_sq == enPassantSquare;

    // *****************************
    // UPDATE HASH
    // *****************************

    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);
    enPassantSquare = NO_SQ;

    hashKey ^= updateKeyCastling();
    if (p == WhiteKing && from_sq == SQ_E1 && to_sq == SQ_G1)
    {
        hashKey ^= updateKeyPiece(WhiteRook, SQ_H1);
        hashKey ^= updateKeyPiece(WhiteRook, SQ_F1);
    }
    else if (p == WhiteKing && from_sq == SQ_E1 && to_sq == SQ_C1)
    {
        hashKey ^= updateKeyPiece(WhiteRook, SQ_A1);
        hashKey ^= updateKeyPiece(WhiteRook, SQ_D1);
    }
    else if (p == BlackKing && from_sq == SQ_E8 && to_sq == SQ_G8)
    {
        hashKey ^= updateKeyPiece(BlackRook, SQ_H8);
        hashKey ^= updateKeyPiece(BlackRook, SQ_F8);
    }
    else if (p == BlackKing && from_sq == SQ_E8 && to_sq == SQ_C8)
    {
        hashKey ^= updateKeyPiece(BlackRook, SQ_A8);
        hashKey ^= updateKeyPiece(BlackRook, SQ_D8);
    }

    if (pt == KING)
    {
        removeCastlingRightsAll(sideToMove);
    }
    else if (pt == ROOK)
    {
        removeCastlingRightsRook(sideToMove, from_sq);
    }
    else if (pt == PAWN)
    {
        halfMoveClock = 0;
        if (ep)
        {
            hashKey ^= updateKeyPiece(makePiece(PAWN, ~sideToMove), Square(to_sq - (sideToMove * -2 + 1) * 8));
        }
        else if (std::abs(from_sq - to_sq) == 16)
        {
            U64 epMask = PawnAttacks(Square(to_sq - (sideToMove * -2 + 1) * 8), sideToMove);
            if (epMask & Pawns(~sideToMove))
            {
                enPassantSquare = Square(to_sq - (sideToMove * -2 + 1) * 8);
                hashKey ^= updateKeyEnPassant(enPassantSquare);
            }
        }
    }

    if (capture != None)
    {
        halfMoveClock = 0;
        hashKey ^= updateKeyPiece(capture, to_sq);
        if (type_of_piece(capture) == ROOK && castlingMapRook.find(to_sq) != castlingMapRook.end())
        {
            castlingRights &= ~castlingMapRook[to_sq];
        }
    }

    if (promoted(move))
    {
        halfMoveClock = 0;

        hashKey ^= updateKeyPiece(makePiece(PAWN, sideToMove), from_sq);
        hashKey ^= updateKeyPiece(p, to_sq);
    }
    else
    {
        hashKey ^= updateKeyPiece(p, from_sq);
        hashKey ^= updateKeyPiece(p, to_sq);
    }

    hashKey ^= updateKeySideToMove();
    hashKey ^= updateKeyCastling();

    prefetch(&TTable[ttIndex(hashKey)]);

    // *****************************
    // UPDATE PIECES AND NNUE
    // *****************************

    if (pt == KING)
    {
        if (sideToMove == White && from_sq == SQ_E1 && to_sq == SQ_G1)
        {
            removePiece<updateNNUE>(WhiteRook, SQ_H1);
            placePiece<updateNNUE>(WhiteRook, SQ_F1);
        }
        else if (sideToMove == White && from_sq == SQ_E1 && to_sq == SQ_C1)
        {
            removePiece<updateNNUE>(WhiteRook, SQ_A1);
            placePiece<updateNNUE>(WhiteRook, SQ_D1);
        }
        else if (sideToMove == Black && from_sq == SQ_E8 && to_sq == SQ_G8)
        {
            removePiece<updateNNUE>(BlackRook, SQ_H8);
            placePiece<updateNNUE>(BlackRook, SQ_F8);
        }
        else if (sideToMove == Black && from_sq == SQ_E8 && to_sq == SQ_C8)
        {
            removePiece<updateNNUE>(BlackRook, SQ_A8);
            placePiece<updateNNUE>(BlackRook, SQ_D8);
        }
    }
    else if (pt == PAWN && ep)
    {
        removePiece<updateNNUE>(makePiece(PAWN, ~sideToMove), Square(to_sq - (sideToMove * -2 + 1) * 8));
    }

    if (capture != None)
    {
        removePiece<updateNNUE>(capture, to_sq);
    }

    if (promoted(move))
    {
        removePiece<updateNNUE>(makePiece(PAWN, sideToMove), from_sq);
        placePiece<updateNNUE>(p, to_sq);
    }
    else
    {
        removePiece<updateNNUE>(p, from_sq);
        placePiece<updateNNUE>(p, to_sq);
    }

    sideToMove = ~sideToMove;
}

template <bool updateNNUE> void Board::unmakeMove(Move move)
{
    State restore = stateHistory.back();
    stateHistory.pop_back();

    if (accumulatorStack.size())
    {
        accumulator = accumulatorStack.back();
        accumulatorStack.pop_back();
    }

    hashKey = hashHistory.back();
    hashHistory.pop_back();

    enPassantSquare = restore.enPassant;
    castlingRights = restore.castling;
    halfMoveClock = restore.halfMove;
    Piece capture = restore.capturedPiece;
    fullMoveNumber--;

    Square from_sq = from(move);
    Square to_sq = to(move);
    bool promotion = promoted(move);

    sideToMove = ~sideToMove;
    PieceType pt = piece(move);
    Piece p = makePiece(pt, sideToMove);

    if (promotion)
    {
        removePiece<updateNNUE>(p, to_sq);
        placePiece<updateNNUE>(makePiece(PAWN, sideToMove), from_sq);
        if (capture != None)
        {
            placePiece<updateNNUE>(capture, to_sq);
        }
        return;
    }
    else
    {
        removePiece<updateNNUE>(p, to_sq);
        placePiece<updateNNUE>(p, from_sq);
    }

    if (to_sq == enPassantSquare && pt == PAWN)
    {
        int8_t offset = sideToMove == White ? -8 : 8;
        placePiece<updateNNUE>(makePiece(PAWN, ~sideToMove), Square(enPassantSquare + offset));
    }
    else if (capture != None)
    {
        placePiece<updateNNUE>(capture, to_sq);
    }
    else if (pt == KING)
    {
        if (from_sq == SQ_E1 && to_sq == SQ_G1 && castlingRights & wk)
        {
            removePiece<updateNNUE>(WhiteRook, SQ_F1);
            placePiece<updateNNUE>(WhiteRook, SQ_H1);
        }
        else if (from_sq == SQ_E1 && to_sq == SQ_C1 && castlingRights & wq)
        {
            removePiece<updateNNUE>(WhiteRook, SQ_D1);
            placePiece<updateNNUE>(WhiteRook, SQ_A1);
        }

        else if (from_sq == SQ_E8 && to_sq == SQ_G8 && castlingRights & bk)
        {
            removePiece<updateNNUE>(BlackRook, SQ_F8);
            placePiece<updateNNUE>(BlackRook, SQ_H8);
        }
        else if (from_sq == SQ_E8 && to_sq == SQ_C8 && castlingRights & bq)
        {
            removePiece<updateNNUE>(BlackRook, SQ_D8);
            placePiece<updateNNUE>(BlackRook, SQ_A8);
        }
    }
}

void Board::makeNullMove()
{
    State store = State(enPassantSquare, castlingRights, halfMoveClock, None);
    stateHistory.push_back(store);
    sideToMove = ~sideToMove;

    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);

    enPassantSquare = NO_SQ;
    fullMoveNumber++;
}

void Board::unmakeNullMove()
{
    State restore = stateHistory.back();
    stateHistory.pop_back();

    enPassantSquare = restore.enPassant;
    castlingRights = restore.castling;
    halfMoveClock = restore.halfMove;

    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);

    fullMoveNumber--;
    sideToMove = ~sideToMove;
}

std::array<int16_t, HIDDEN_BIAS> &Board::getAccumulator()
{
    return accumulator;
}

/**
 * PRIVATE FUNCTIONS
 *
 */

U64 Board::zobristHash()
{
    U64 hash = 0ULL;
    U64 wPieces = Us<White>();
    U64 bPieces = Us<Black>();
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

void Board::initializeLookupTables()
{
    // initialize squares between table
    U64 sqs;
    for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
    {
        for (Square sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
        {
            sqs = (1ULL << sq1) | (1ULL << sq2);
            if (square_file(sq1) == square_file(sq2) || square_rank(sq1) == square_rank(sq2))
                SQUARES_BETWEEN_BB[sq1][sq2] = RookAttacks(sq1, sqs) & RookAttacks(sq2, sqs);

            else if (diagonal_of(sq1) == diagonal_of(sq2) || anti_diagonal_of(sq1) == anti_diagonal_of(sq2))
                SQUARES_BETWEEN_BB[sq1][sq2] = BishopAttacks(sq1, sqs) & BishopAttacks(sq2, sqs);
        }
    }
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

void Board::removeCastlingRightsAll(Color c)
{
    if (c == White)
    {
        castlingRights &= ~(wk | wq);
    }
    else if (c == Black)
    {
        castlingRights &= ~(bk | bq);
    }
}

void Board::removeCastlingRightsRook(Color c, Square sq)
{
    if (c == White && sq == SQ_A1)
    {
        castlingRights &= ~wq;
    }
    else if (c == White && sq == SQ_H1)
    {
        castlingRights &= ~wk;
    }
    else if (c == Black && sq == SQ_A8)
    {
        castlingRights &= ~bq;
    }
    else if (c == Black && sq == SQ_H8)
    {
        castlingRights &= ~bk;
    }
}

template void Board::makeMove<false>(Move move);
template void Board::makeMove<true>(Move move);
template void Board::unmakeMove<false>(Move move);
template void Board::unmakeMove<true>(Move move);
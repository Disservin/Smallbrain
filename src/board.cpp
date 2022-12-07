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
    {
        accumulator[i] = hiddenBias[i];
    }

    for (Square i = SQ_A1; i < NO_SQ; i++)
    {
        Piece p = board[i];
        bool input = p != None;
        if (!input)
            continue;
        NNUE::activate(accumulator, i, p);
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

Piece Board::pieceAtB(Square sq) const
{
    return board[sq];
}

void Board::applyFen(std::string fen, bool updateAcc)
{
    for (Piece p = WhitePawn; p < None; p++)
    {
        Bitboards[p] = 0ULL;
    }
    std::vector<std::string> params = splitInput(fen);
    std::string position = params[0];
    std::string move_right = params[1];
    std::string castling = params[2];
    std::string en_passant = params[3];
    // default
    std::string half_move_clock = "0";
    std::string full_move_counter = "1";
    sideToMove = (move_right == "w") ? White : Black;

    if (params.size() > 4)
    {
        half_move_clock = params[4];
        full_move_counter = params[5];
    }

    if (updateAcc)
    {
        for (int i = 0; i < HIDDEN_BIAS; i++)
        {
            accumulator[i] = hiddenBias[i];
        }
    }

    std::fill(std::begin(board), std::end(board), None);

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
            square = Square(square + (curr - '0'));
        }
    }

    std::vector<std::string> allowedCastlingFiles{"A", "B", "C", "D", "E", "F", "G", "H",
                                                  "a", "b", "c", "d", "e", "f", "g", "h"};

    chess960 = chess960 ? true : contains(allowedCastlingFiles, castling);

    castlingRights = 0;
    removeCastlingRightsAll(White);
    removeCastlingRightsAll(Black);

    if (!chess960)
    {
        for (size_t i = 0; i < castling.size(); i++)
        {
            if (readCastleString.find(castling[i]) != readCastleString.end())
                castlingRights |= readCastleString[castling[i]];
        }
    }
    else
    {
        int indexWhite = 0;
        int indexBlack = 0;
        for (size_t i = 0; i < castling.size(); i++)
        {
            if (!elementInVector(std::string{castling[i]}, allowedCastlingFiles))
                continue;

            if (isupper(castling[i]))
            {
                castlingRights |= 1ull << indexWhite;
                castlingRights960White[indexWhite++] = File(castling[i] - 65);
            }
            else
            {
                castlingRights |= 1ull << (2 + indexBlack);
                castlingRights960Black[indexBlack++] = File(castling[i] - 97);
            }
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

std::string Board::getFen() const
{
    std::stringstream ss;

    // Loop through the ranks of the board in reverse order
    for (int rank = 7; rank >= 0; rank--)
    {
        int free_space = 0;

        // Loop through the files of the board
        for (int file = 0; file < 8; file++)
        {
            // Calculate the square index
            int sq = rank * 8 + file;

            // Get the piece at the current square
            Piece piece = pieceAtB(Square(sq));

            // If there is a piece at the current square
            if (piece != None)
            {
                // If there were any empty squares before this piece,
                // append the number of empty squares to the FEN string
                if (free_space)
                {
                    ss << free_space;
                    free_space = 0;
                }

                // Append the character representing the piece to the FEN string
                ss << pieceToChar[piece];
            }
            else
            {
                // If there is no piece at the current square, increment the
                // counter for the number of empty squares
                free_space++;
            }
        }

        // If there are any empty squares at the end of the rank,
        // append the number of empty squares to the FEN string
        if (free_space != 0)
        {
            ss << free_space;
        }

        // Append a "/" character to the FEN string, unless this is the last rank
        ss << (rank > 0 ? "/" : "");
    }

    // Append " w " or " b " to the FEN string, depending on which player's turn it is
    ss << (sideToMove == White ? " w " : " b ");

    // Append the appropriate characters to the FEN string to indicate
    // whether or not castling is allowed for each player
    if (castlingRights & wk)
        ss << "K";
    if (castlingRights & wq)
        ss << "Q";
    if (castlingRights & bk)
        ss << "k";
    if (castlingRights & bq)
        ss << "q";
    if (castlingRights == 0)
        ss << "-";

    // Append information about the en passant square (if any)
    // and the halfmove clock and fullmove number to the FEN string
    if (enPassantSquare == NO_SQ)
        ss << " - ";
    else
        ss << " " << squareToString[enPassantSquare] << " ";

    ss << int(halfMoveClock) << " " << int(fullMoveNumber / 2);

    // Return the resulting FEN string
    return ss.str();
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
    return lsb(Kings(c));
}

U64 Board::Enemy(Color c)
{
    assert(Us(~c) != 0);

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
    assert(Bitboards[KING + c * 6] != 0);
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

    assert(from_sq >= 0 && from_sq < 64);
    assert(to_sq >= 0 && to_sq < 64);
    assert(type_of_piece(capture) != KING);
    assert(p != None);
    assert((promoted(move) && (pt != PAWN && pt != KING)) || !promoted(move));

    // *****************************
    // STORE STATE HISTORY
    // *****************************

    hashHistory.emplace_back(hashKey);

    State store =
        State(enPassantSquare, castlingRights, halfMoveClock, capture, castlingRights960White, castlingRights960Black);
    stateHistory.push_back(store);

    if constexpr (updateNNUE)
        accumulatorStack.emplace_back(accumulator);

    halfMoveClock++;
    fullMoveNumber++;

    bool ep = to_sq == enPassantSquare;
    const bool isCastlingWhite =
        (p == WhiteKing && capture == WhiteRook) || (p == WhiteKing && square_distance(to_sq, from_sq) >= 2);
    const bool isCastlingBlack =
        (p == BlackKing && capture == BlackRook) || (p == BlackKing && square_distance(to_sq, from_sq) >= 2);

    // *****************************
    // UPDATE HASH
    // *****************************

    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);
    enPassantSquare = NO_SQ;

    hashKey ^= updateKeyCastling();

    if (isCastlingWhite || isCastlingBlack)
    {
        Piece rook = sideToMove == White ? WhiteRook : BlackRook;
        Square rookSQ = file_rank_square(to_sq > from_sq ? FILE_F : FILE_D, square_rank(from_sq));

        assert(type_of_piece(pieceAtB(to_sq)) == ROOK);
        hashKey ^= updateKeyPiece(rook, to_sq);
        hashKey ^= updateKeyPiece(rook, rookSQ);
    }

    if (pt == KING)
    {
        removeCastlingRightsAll(sideToMove);
    }
    else if (pt == ROOK)
    {
        removeCastlingRightsRook(from_sq);
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

                assert(pieceAtB(enPassantSquare) == None);
            }
        }
    }

    if (capture != None && !(isCastlingWhite || isCastlingBlack))
    {
        halfMoveClock = 0;
        hashKey ^= updateKeyPiece(capture, to_sq);
        if (type_of_piece(capture) == ROOK)
            removeCastlingRightsRook(to_sq);
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

    if (isCastlingWhite || isCastlingBlack)
    {
        Square rookToSq;
        Piece rook = sideToMove == White ? WhiteRook : BlackRook;

        removePiece<updateNNUE>(p, from_sq);
        removePiece<updateNNUE>(rook, to_sq);

        rookToSq = file_rank_square(to_sq > from_sq ? FILE_F : FILE_D, square_rank(from_sq));
        to_sq = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C, square_rank(from_sq));

        placePiece<updateNNUE>(p, to_sq);
        placePiece<updateNNUE>(rook, rookToSq);
    }
    else if (pt == PAWN && ep)
    {
        assert(pieceAtB(Square(to_sq - (sideToMove * -2 + 1) * 8)) != None);

        removePiece<updateNNUE>(makePiece(PAWN, ~sideToMove), Square(to_sq - (sideToMove * -2 + 1) * 8));
    }
    else if (capture != None && !(isCastlingWhite || isCastlingBlack))
    {
        assert(pieceAtB(to_sq) != None);

        removePiece<updateNNUE>(capture, to_sq);
    }

    if (promoted(move))
    {
        assert(pieceAtB(to_sq) == None);

        removePiece<updateNNUE>(makePiece(PAWN, sideToMove), from_sq);
        placePiece<updateNNUE>(p, to_sq);
    }
    else if (!(isCastlingWhite || isCastlingBlack))
    {
        assert(pieceAtB(to_sq) == None);

        movePiece<updateNNUE>(p, from_sq, to_sq);
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
    castlingRights960White = restore.chess960White;
    castlingRights960Black = restore.chess960Black;

    fullMoveNumber--;

    Square from_sq = from(move);
    Square to_sq = to(move);
    bool promotion = promoted(move);

    sideToMove = ~sideToMove;
    PieceType pt = piece(move);
    Piece p = makePiece(pt, sideToMove);

    const bool isCastlingWhite =
        (p == WhiteKing && capture == WhiteRook) || (p == WhiteKing && square_distance(to_sq, from_sq) >= 2);
    const bool isCastlingBlack =
        (p == BlackKing && capture == BlackRook) || (p == BlackKing && square_distance(to_sq, from_sq) >= 2);

    if ((isCastlingWhite || isCastlingBlack))
    {
        Square rookToSq = to_sq;
        Piece rook = sideToMove == White ? WhiteRook : BlackRook;
        Square rookFromSq = file_rank_square(to_sq > from_sq ? FILE_F : FILE_D, square_rank(from_sq));
        to_sq = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C, square_rank(from_sq));

        // We need to remove both pieces first and then place them back.
        removePiece<updateNNUE>(rook, rookFromSq);
        removePiece<updateNNUE>(p, to_sq);

        placePiece<updateNNUE>(p, from_sq);
        placePiece<updateNNUE>(rook, rookToSq);
    }
    else if (promotion)
    {
        removePiece<updateNNUE>(p, to_sq);
        placePiece<updateNNUE>(makePiece(PAWN, sideToMove), from_sq);
        if (capture != None)
            placePiece<updateNNUE>(capture, to_sq);
        return;
    }
    else
    {
        movePiece<updateNNUE>(p, to_sq, from_sq);
    }

    if (to_sq == enPassantSquare && pt == PAWN)
    {
        int8_t offset = sideToMove == White ? -8 : 8;
        placePiece<updateNNUE>(makePiece(PAWN, ~sideToMove), Square(enPassantSquare + offset));
    }
    else if (capture != None && !(isCastlingWhite || isCastlingBlack))
    {
        placePiece<updateNNUE>(capture, to_sq);
    }
}

void Board::makeNullMove()
{
    State store =
        State(enPassantSquare, castlingRights, halfMoveClock, None, castlingRights960White, castlingRights960Black);
    stateHistory.push_back(store);
    sideToMove = ~sideToMove;

    // Update the hash key
    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);

    prefetch(&TTable[ttIndex(hashKey)]);

    // Set the en passant square to NO_SQ and increment the full move number
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
    castlingRights960White = restore.chess960White;
    castlingRights960Black = restore.chess960Black;

    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);

    fullMoveNumber--;
    sideToMove = ~sideToMove;
}

NNUE::accumulator &Board::getAccumulator()
{
    return accumulator;
}

U64 Board::attacksByPiece(PieceType pt, Square sq, Color c)
{
    switch (pt)
    {
    case PAWN:
        return PawnAttacks(sq, c);
    case KNIGHT:
        return KnightAttacks(sq);
    case BISHOP:
        return BishopAttacks(sq, All());
    case ROOK:
        return RookAttacks(sq, All());
    case QUEEN:
        return QueenAttacks(sq, All());
    case KING:
        return KingAttacks(sq);
    case NONETYPE:
        return 0ULL;
    default:
        return 0ULL;
    }
}

/********************
 * Static Exchange Evaluation, logical based on Weiss (https://github.com/TerjeKir/weiss) licensed under GPL-3.0
 *******************/
bool Board::see(Move move, int threshold)
{
    Square from_sq = from(move);
    Square to_sq = to(move);
    PieceType attacker = type_of_piece(pieceAtB(from_sq));
    PieceType victim = type_of_piece(pieceAtB(to_sq));
    int swap = pieceValuesDefault[victim] - threshold;
    if (swap < 0)
        return false;
    swap -= pieceValuesDefault[attacker];
    if (swap >= 0)
        return true;
    U64 occ = (All() ^ (1ULL << from_sq)) | (1ULL << to_sq);
    U64 attackers = allAttackers(to_sq, occ) & occ;

    U64 queens = Bitboards[WhiteQueen] | Bitboards[BlackQueen];

    U64 bishops = Bitboards[WhiteBishop] | Bitboards[BlackBishop] | queens;
    U64 rooks = Bitboards[WhiteRook] | Bitboards[BlackRook] | queens;

    Color sT = ~colorOf(from_sq);

    while (true)
    {
        attackers &= occ;
        U64 myAttackers = attackers & Us(sT);
        if (!myAttackers)
            break;

        int pt;
        for (pt = 0; pt <= 5; pt++)
        {
            if (myAttackers & (Bitboards[pt] | Bitboards[pt + 6]))
                break;
        }
        sT = ~sT;
        if ((swap = -swap - 1 - piece_values[MG][pt]) >= 0)
        {
            if (pt == KING && (attackers & Us(sT)))
                sT = ~sT;
            break;
        }

        occ ^= (1ULL << (lsb(myAttackers & (Bitboards[pt] | Bitboards[pt + 6]))));

        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= BishopAttacks(to_sq, occ) & bishops;
        if (pt == ROOK || pt == QUEEN)
            attackers |= RookAttacks(to_sq, occ) & rooks;
    }
    return sT != colorOf(from_sq);
}

std::ostream &operator<<(std::ostream &os, const Board &b)
{
    for (int i = 63; i >= 0; i -= 8)
    {
        os << " " << pieceToChar[b.board[i - 7]] << " " << pieceToChar[b.board[i - 6]] << " "
           << pieceToChar[b.board[i - 5]] << " " << pieceToChar[b.board[i - 4]] << " " << pieceToChar[b.board[i - 3]]
           << " " << pieceToChar[b.board[i - 2]] << " " << pieceToChar[b.board[i - 1]] << " " << pieceToChar[b.board[i]]
           << " \n";
    }
    os << "\n\n";
    os << "Fen: " << b.getFen() << "\n";
    os << "Side to move: " << static_cast<int>(b.sideToMove) << "\n";
    os << "Castling rights: " << static_cast<int>(b.castlingRights) << "\n";
    os << "Halfmoves: " << static_cast<int>(b.halfMoveClock) << "\n";
    os << "Fullmoves: " << static_cast<int>(b.fullMoveNumber) / 2 << "\n";
    os << "EP: " << static_cast<int>(b.enPassantSquare) << "\n";
    os << "Hash: " << b.hashKey << "\n";
    os << "Chess960: " << b.chess960;

    for (int j = 0; j < 2; j++)
    {
        os << " WhiteRights: " << b.castlingRights960White[j];
    }
    for (int j = 0; j < 2; j++)
    {
        os << " BlackRights: " << b.castlingRights960Black[j];
    }
    os << std::endl;
    return os;
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
            if (sq1 == sq2)
                SQUARES_BETWEEN_BB[sq1][sq2] = 0ull;
            else if (square_file(sq1) == square_file(sq2) || square_rank(sq1) == square_rank(sq2))
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
        castlingRights960White[0] = NO_FILE;
        castlingRights960White[1] = NO_FILE;
    }
    else
    {
        castlingRights &= ~(bk | bq);
        castlingRights960Black[0] = NO_FILE;
        castlingRights960Black[1] = NO_FILE;
    }
}

void Board::removeCastlingRightsRook(Square sq)
{
    if (chess960)
    {
        castlingRights960White[0] = castlingRights960White[0] == square_file(sq) && square_rank(sq) == RANK_1
                                        ? NO_FILE
                                        : castlingRights960White[0];
        castlingRights960White[1] = castlingRights960White[1] == square_file(sq) && square_rank(sq) == RANK_1
                                        ? NO_FILE
                                        : castlingRights960White[1];
        castlingRights960Black[0] = castlingRights960Black[0] == square_file(sq) && square_rank(sq) == RANK_8
                                        ? NO_FILE
                                        : castlingRights960Black[0];
        castlingRights960Black[1] = castlingRights960Black[1] == square_file(sq) && square_rank(sq) == RANK_8
                                        ? NO_FILE
                                        : castlingRights960Black[1];
    }

    if (castlingMapRook.find(sq) != castlingMapRook.end())
    {
        castlingRights &= ~castlingMapRook[sq];
    }
}

std::string uciRep(Board &board, Move move)
{
    std::stringstream ss;

    // Get the from and to squares
    Square from_sq = from(move);
    Square to_sq = to(move);

    // If the move is not a chess960 castling move and is a king moving more than one square,
    // update the to square to be the correct square for a regular castling move
    if (!board.chess960 && piece(move) == KING && square_distance(to_sq, from_sq) >= 2)
    {
        to_sq = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C, square_rank(from_sq));
    }

    // Add the from and to squares to the string stream
    ss << squareToString[from_sq];
    ss << squareToString[to_sq];

    // If the move is a promotion, add the promoted piece to the string stream
    if (promoted(move))
    {
        ss << PieceTypeToPromPiece[piece(move)];
    }

    return ss.str();
}

template void Board::makeMove<false>(Move move);
template void Board::makeMove<true>(Move move);
template void Board::unmakeMove<false>(Move move);
template void Board::unmakeMove<true>(Move move);
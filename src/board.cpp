#include "board.h"
#include "movegen.h"

Board::Board()
{
    initializeLookupTables();
    stateHistory.reserve(MAX_PLY);
    hashHistory.reserve(512);
    accumulatorStack.reserve(MAX_PLY);

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

    applyFen(DEFAULT_POS, true);

    occEnemy = Enemy(sideToMove);
    occUs = Us(sideToMove);
    occAll = All();
    enemyEmptyBB = ~occUs;

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

Piece Board::pieceAtBB(Square sq) const
{
    for (Piece p = WhitePawn; p < None; p++)
    {
        if (piecesBB[p] & (1ULL << sq))
            return p;
    }

    return None;
}

Piece Board::pieceAtB(Square sq) const
{
    return board[sq];
}

void Board::applyFen(const std::string &fen, bool updateAcc)
{
    for (Piece p = WhitePawn; p < None; p++)
    {
        piecesBB[p] = 0ULL;
    }

    std::vector<std::string> params = splitInput(fen);

    const std::string position = params[0];
    const std::string move_right = params[1];
    const std::string castling = params[2];
    const std::string en_passant = params[3];

    const std::string half_move_clock = params.size() > 4 ? params[4] : "0";
    const std::string full_move_counter = params.size() > 4 ? params[5] : "1";

    sideToMove = (move_right == "w") ? White : Black;

    std::fill(std::begin(board), std::end(board), None);

    Square square = Square(56);
    for (int index = 0; index < static_cast<int>(position.size()); index++)
    {
        char curr = position[index];
        if (charToPiece.find(curr) != charToPiece.end())
        {
            const Piece piece = charToPiece[curr];
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

    if (updateAcc)
    {
        accumulate();
    }

    const std::vector<std::string> allowedCastlingFiles{"A", "B", "C", "D", "E", "F", "G", "H",
                                                        "a", "b", "c", "d", "e", "f", "g", "h"};

    chess960 = chess960 || contains(allowedCastlingFiles, castling);

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
            if (!contains(allowedCastlingFiles, std::string{castling[i]}))
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

    stateHistory.clear();
    hashHistory.clear();
    accumulatorStack.clear();

    hashKey = zobristHash();
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

bool Board::isRepetition(int draw) const
{
    uint8_t c = 0;

    for (int i = static_cast<int>(hashHistory.size()) - 2;
         i >= 0 && i >= static_cast<int>(hashHistory.size()) - halfMoveClock - 1; i -= 2)
    {
        if (hashHistory[i] == hashKey)
            c++;
        if (c == draw)
            return true;
    }

    return false;
}

Result Board::isDrawn(bool inCheck)
{
    if (halfMoveClock >= 100)
    {
        if (inCheck && !Movegen::hasLegalMoves(*this))
            return Result::LOST;
        return Result::DRAWN;
    }

    const auto count = popcount(All());

    if (count == 2)
        return Result::DRAWN;

    if (count == 3)
    {
        if (pieces<WhiteBishop>() || pieces<BlackBishop>())
            return Result::DRAWN;
        if (pieces<WhiteKnight>() || pieces<BlackKnight>())
            return Result::DRAWN;
    }

    if (count == 4)
    {
        if (pieces<WhiteBishop>() && pieces<BlackBishop>() &&
            sameColor(lsb(pieces<WhiteBishop>()), lsb(pieces<BlackBishop>())))
            return Result::DRAWN;
    }

    return Result::NONE;
}

bool Board::nonPawnMat(Color c) const
{
    return pieces(KNIGHT, c) | pieces(BISHOP, c) | pieces(ROOK, c) | pieces(QUEEN, c);
}

Square Board::KingSQ(Color c) const
{
    return lsb(pieces(KING, c));
}

U64 Board::Enemy(Color c) const
{
    return Us(~c);
}

U64 Board::Us(Color c) const
{
    return piecesBB[PAWN + c * 6] | piecesBB[KNIGHT + c * 6] | piecesBB[BISHOP + c * 6] | piecesBB[ROOK + c * 6] |
           piecesBB[QUEEN + c * 6] | piecesBB[KING + c * 6];
}

U64 Board::All() const
{
    return Us<White>() | Us<Black>();
}

Color Board::colorOf(Square loc) const
{
    return Color((pieceAtB(loc) / 6));
}

bool Board::isSquareAttacked(Color c, Square sq) const
{
    if (pieces(PAWN, c) & PawnAttacks(sq, ~c))
        return true;
    if (pieces(KNIGHT, c) & KnightAttacks(sq))
        return true;
    if ((pieces(BISHOP, c) | pieces(QUEEN, c)) & BishopAttacks(sq, All()))
        return true;
    if ((pieces(ROOK, c) | pieces(QUEEN, c)) & RookAttacks(sq, All()))
        return true;
    if (pieces(KING, c) & KingAttacks(sq))
        return true;
    return false;
}

bool Board::isSquareAttacked(Color c, Square sq, U64 occ) const
{
    if (pieces(PAWN, c) & PawnAttacks(sq, ~c))
        return true;
    if (pieces(KNIGHT, c) & KnightAttacks(sq))
        return true;
    if ((pieces(BISHOP, c) | pieces(QUEEN, c)) & BishopAttacks(sq, occ))
        return true;
    if ((pieces(ROOK, c) | pieces(QUEEN, c)) & RookAttacks(sq, occ))
        return true;
    if (pieces(KING, c) & KingAttacks(sq))
        return true;
    return false;
}

U64 Board::allAttackers(Square sq, U64 occupiedBB)
{
    return attackersForSide(White, sq, occupiedBB) | attackersForSide(Black, sq, occupiedBB);
}

U64 Board::attackersForSide(Color attackerColor, Square sq, U64 occupiedBB)
{
    U64 attackingBishops = pieces(BISHOP, attackerColor);
    U64 attackingRooks = pieces(ROOK, attackerColor);
    U64 attackingQueens = pieces(QUEEN, attackerColor);
    U64 attackingKnights = pieces(KNIGHT, attackerColor);
    U64 attackingKing = pieces(KING, attackerColor);
    U64 attackingPawns = pieces(PAWN, attackerColor);

    U64 interCardinalRays = BishopAttacks(sq, occupiedBB);
    U64 cardinalRaysRays = RookAttacks(sq, occupiedBB);

    U64 attackers = interCardinalRays & (attackingBishops | attackingQueens);
    attackers |= cardinalRaysRays & (attackingRooks | attackingQueens);
    attackers |= KnightAttacks(sq) & attackingKnights;
    attackers |= KingAttacks(sq) & attackingKing;
    attackers |= PawnAttacks(sq, ~attackerColor) & attackingPawns;
    return attackers;
}

void Board::makeNullMove()
{
    stateHistory.emplace_back(enPassantSquare, castlingRights, halfMoveClock, None, castlingRights960White,
                              castlingRights960Black);
    sideToMove = ~sideToMove;

    // Update the hash key
    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);

    TTable.prefetchTT(hashKey);

    // Set the en passant square to NO_SQ and increment the full move number
    enPassantSquare = NO_SQ;
    fullMoveNumber++;
}

void Board::unmakeNullMove()
{
    const State restore = stateHistory.back();
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

const NNUE::accumulator &Board::getAccumulator() const
{
    return accumulator;
}

bool Board::isLegal(const Move move)
{
    const Color color = sideToMove;
    const Square from_sq = from(move);
    const Square to_sq = to(move);
    const Piece p = makePiece(piece(move), color);
    const Piece capture = pieceAtB(to_sq);
    const U64 all = All();

    Square kSQ = KingSQ(color);

    assert(type_of_piece(capture) != KING);

    if (piece(move) == PAWN && to_sq == enPassantSquare)
    {
        const Direction DOWN = color == Black ? NORTH : SOUTH;
        const Square capSq = to_sq + DOWN;

        U64 bb = all;
        bb &= ~(1ull << from_sq);
        bb |= 1ull << to_sq;
        bb &= ~(1ull << capSq);

        return !((RookAttacks(kSQ, bb) & (pieces(ROOK, ~color) | pieces(QUEEN, ~color))) ||
                 (BishopAttacks(kSQ, bb) & (pieces(BISHOP, ~color) | pieces(QUEEN, ~color))));
    }

    const bool isCastlingWhite = p == WhiteKing && capture == WhiteRook;
    const bool isCastlingBlack = p == BlackKing && capture == BlackRook;

    if (isCastlingWhite || isCastlingBlack)
    {
        const Square destKing = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C, square_rank(from_sq));
        const Square rookToSq = file_rank_square(to_sq > from_sq ? FILE_F : FILE_D, square_rank(from_sq));
        const Piece rook = sideToMove == White ? WhiteRook : BlackRook;

        if (isSquareAttacked(~color, from_sq, all) || isSquareAttacked(~color, destKing, all))
            return false;

        piecesBB[p] &= ~(1ull << from_sq);
        piecesBB[rook] &= ~(1ull << to_sq);

        piecesBB[p] |= (1ull << destKing);
        piecesBB[rook] |= (1ull << rookToSq);

        bool isAttacked = isSquareAttacked(~color, destKing);

        piecesBB[rook] &= ~(1ull << rookToSq);
        piecesBB[p] &= ~(1ull << destKing);

        piecesBB[p] |= (1ull << from_sq);
        piecesBB[rook] |= (1ull << to_sq);

        return !isAttacked;

        assert(All() == all);
        return true;
    }

    if (piece(move) == KING)
    {
        kSQ = to_sq;
    }

    if (promoted(move))
    {
        piecesBB[makePiece(PAWN, color)] &= ~(1ull << from_sq);
        piecesBB[p] |= (1ull << to_sq);
    }
    else
    {
        piecesBB[p] &= ~(1ull << from_sq);
        piecesBB[p] |= (1ull << to_sq);
    }

    if (capture != None)
        piecesBB[capture] &= ~(1ull << to_sq);

    const bool isAttacked = isSquareAttacked(~color, kSQ);

    if (capture != None)
        piecesBB[capture] |= (1ull << to_sq);

    if (promoted(move))
    {
        piecesBB[p] &= ~(1ull << to_sq);
        piecesBB[makePiece(PAWN, color)] |= (1ull << from_sq);
    }
    else
    {
        piecesBB[p] |= (1ull << from_sq);
        piecesBB[p] &= ~(1ull << to_sq);
    }

    assert(All() == all);
    return !isAttacked;
}

bool Board::isPseudoLegal(const Move move)
{
    if (move == NO_MOVE || move == NULL_MOVE)
        return false;

    const Color color = sideToMove;
    const Square from_sq = from(move);
    const Square to_sq = to(move);
    const Piece p = makePiece(piece(move), color);
    const Piece capture = pieceAtB(to_sq);
    const bool isCastlingWhite =
        (p == WhiteKing && capture == WhiteRook) || (p == WhiteKing && square_distance(to_sq, from_sq) >= 2);
    const bool isCastlingBlack =
        (p == BlackKing && capture == BlackRook) || (p == BlackKing && square_distance(to_sq, from_sq) >= 2);

    if (piece(move) >= NONETYPE)
        return false;

    if (p != pieceAtB(from_sq) && !promoted(move))
        return false;

    if (promoted(move) && (type_of_piece(pieceAtB(from_sq)) != PAWN || (piece(move) == PAWN || piece(move) == KING)))
        return false;

    if (colorOf(from_sq) != color)
        return false;

    if (from_sq == to_sq)
        return false;

    if (piece(move) != type_of_piece(pieceAtB(from_sq)) && !promoted(move))
        return false;

    const U64 occ = All();

    if (isCastlingWhite || isCastlingBlack)
    {
        const bool correctRank = color == White ? square_rank(from_sq) == RANK_1 && square_rank(to_sq) == RANK_1
                                                : square_rank(from_sq) == RANK_8 && square_rank(to_sq) == RANK_8;

        if (!correctRank)
            return false;

        const Square destKing = color == White ? (to_sq > from_sq ? SQ_G1 : SQ_C1) : (to_sq > from_sq ? SQ_G8 : SQ_C8);
        const Square destRook =
            color == White ? (destKing == SQ_G1 ? SQ_F1 : SQ_D1) : (destKing == SQ_G8 ? SQ_F8 : SQ_D8);
        const Piece rook = sideToMove == White ? WhiteRook : BlackRook;

        if (pieceAtB(to_sq) != rook)
            return false;

        U64 copy = SQUARES_BETWEEN_BB[from_sq][to_sq] | SQUARES_BETWEEN_BB[from_sq][destKing];
        U64 occCopy = occ;
        occCopy &= ~(1ull << to_sq);
        occCopy &= ~(1ull << from_sq);

        while (copy)
        {
            const Square sq = poplsb(copy);
            if ((1ull << sq) & occCopy)
                return false;
        }

        if (((1ull << destRook) & occCopy) || ((1ull << destKing) & occCopy))
            return false;

        copy = SQUARES_BETWEEN_BB[from_sq][destKing];
        while (copy)
        {
            const Square sq = poplsb(copy);
            if (isSquareAttacked(~color, sq, occ))
                return false;
        }

        const uint8_t rights = color == White ? to_sq > from_sq ? wk : wq : to_sq > from_sq ? bk : bq;

        if (!chess960 && !(rights & castlingRights))
            return false;

        if (chess960)
        {
            const bool rights960 = color == White ? castlingRights960White[0] == square_file(to_sq) ||
                                                        castlingRights960White[1] == square_file(to_sq)
                                                  : castlingRights960Black[0] == square_file(to_sq) ||
                                                        castlingRights960Black[1] == square_file(to_sq);
            if (!rights960)
                return false;
        }

        return true;
    }

    if (capture != None && colorOf(to_sq) == color)
        return false;

    if (piece(move) == PAWN || promoted(move))
    {
        const Direction DOWN = color == Black ? NORTH : SOUTH;
        const Rank promotionRank = color == Black ? RANK_1 : RANK_8;

        if ((!promoted(move) && square_rank(to_sq) == promotionRank) ||
            (promoted(move) && square_rank(to_sq) != promotionRank))
            return false;

        if (std::abs(from_sq - to_sq) == 16)
        {
            const Direction UP = color == Black ? SOUTH : NORTH;
            const Rank rankD = color == Black ? RANK_7 : RANK_2;

            return square_rank(from_sq) == rankD && pieceAtB(Square(int(from_sq + UP))) == None &&
                   pieceAtB(Square(int(from_sq + UP + UP))) == None && to_sq == from_sq + UP + UP;
        }
        else if (std::abs(from_sq - to_sq) == 8)
        {
            const Direction UP = color == Black ? SOUTH : NORTH;
            return pieceAtB(Square(int(to_sq))) == None && to_sq == from_sq + UP;
        }
        else
        {
            return enPassantSquare == to_sq
                       ? capture == None && attacksByPiece(PAWN, from_sq, color, occ) & (1ull << to_sq) &&
                             type_of_piece(pieceAtB(to_sq + DOWN)) == PAWN
                       : capture != None && attacksByPiece(PAWN, from_sq, color, occ) & (1ull << to_sq);
        }

        return true;
    }

    if ((piece(move) != KNIGHT && SQUARES_BETWEEN_BB[from_sq][to_sq] & All()) ||
        !(attacksByPiece(piece(move), from_sq, color, occ) & (1ull << to_sq)))
        return false;

    return true;
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

U64 Board::attacksByPiece(PieceType pt, Square sq, Color c, U64 occ)
{
    switch (pt)
    {
    case PAWN:
        return PawnAttacks(sq, c);
    case KNIGHT:
        return KnightAttacks(sq);
    case BISHOP:
        return BishopAttacks(sq, occ);
    case ROOK:
        return RookAttacks(sq, occ);
    case QUEEN:
        return QueenAttacks(sq, occ);
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

    U64 queens = piecesBB[WhiteQueen] | piecesBB[BlackQueen];

    U64 bishops = piecesBB[WhiteBishop] | piecesBB[BlackBishop] | queens;
    U64 rooks = piecesBB[WhiteRook] | piecesBB[BlackRook] | queens;

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
            if (myAttackers & (piecesBB[pt] | piecesBB[pt + 6]))
                break;
        }
        sT = ~sT;
        if ((swap = -swap - 1 - piece_values[MG][pt]) >= 0)
        {
            if (pt == KING && (attackers & Us(sT)))
                sT = ~sT;
            break;
        }

        occ ^= (1ULL << (lsb(myAttackers & (piecesBB[pt] | piecesBB[pt + 6]))));

        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= BishopAttacks(to_sq, occ) & bishops;
        if (pt == ROOK || pt == QUEEN)
            attackers |= RookAttacks(to_sq, occ) & rooks;
    }
    return sT != colorOf(from_sq);
}

void Board::clearStacks()
{
    accumulatorStack.clear();
    stateHistory.clear();
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

U64 Board::zobristHash() const
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

/**
 * PRIVATE FUNCTIONS
 *
 */

void Board::initializeLookupTables()
{
    // initialize squares between table
    std::array<std::array<U64, MAX_SQ>, MAX_SQ> arr = {};

    U64 sqs;
    for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
    {
        for (Square sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
        {
            sqs = (1ULL << sq1) | (1ULL << sq2);
            if (sq1 == sq2)
                SQUARES_BETWEEN_BB[sq1][sq2] = 0ull;
            else if (square_file(sq1) == square_file(sq2) || square_rank(sq1) == square_rank(sq2))
                arr[sq1][sq2] = RookAttacks(sq1, sqs) & RookAttacks(sq2, sqs);
            else if (diagonal_of(sq1) == diagonal_of(sq2) || anti_diagonal_of(sq1) == anti_diagonal_of(sq2))
                arr[sq1][sq2] = BishopAttacks(sq1, sqs) & BishopAttacks(sq2, sqs);
        }
    }

    SQUARES_BETWEEN_BB = arr;
}

U64 Board::updateKeyPiece(Piece piece, Square sq) const
{
    return RANDOM_ARRAY[64 * hash_piece[piece] + sq];
}

U64 Board::updateKeyEnPassant(Square sq) const
{
    return RANDOM_ARRAY[772 + square_file(sq)];
}

U64 Board::updateKeyCastling() const
{
    return castlingKey[castlingRights];
}

U64 Board::updateKeySideToMove() const
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

std::string uciMove(const Board &board, Move move)
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

Square extractSquare(std::string_view squareStr)
{
    char letter = squareStr[0];
    int file = letter - 96;
    int rank = squareStr[1] - 48;
    int index = (rank - 1) * 8 + file - 1;
    return Square(index);
}

Move convertUciToMove(const Board &board, const std::string &input)
{
    Square source = extractSquare(input.substr(0, 2));
    Square target = extractSquare(input.substr(2, 2));
    PieceType piece = type_of_piece(board.pieceAtBB(source));

    // convert to king captures rook
    if (!board.chess960 && piece == KING && square_distance(target, source) == 2)
    {
        target = file_rank_square(target > source ? FILE_H : FILE_A, square_rank(source));
    }

    switch (input.length())
    {
    case 4:
        return make(piece, source, target, false);
    case 5:
        return make(pieceToInt[input.at(4)], source, target, true);
    default:
        std::cout << "FALSE INPUT" << std::endl;
        return make(NONETYPE, NO_SQ, NO_SQ, false);
    }
}
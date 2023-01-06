#pragma once

#include <array>
#include <iostream>
#include <string>

#include "attacks.h"
#include "helper.h"
#include "nnue.h"
#include "tt.h"
#include "types.h"
#include "zobrist.h"

extern TranspositionTable TTable;
extern uint8_t inputValues[INPUT_WEIGHTS];
extern int16_t inputWeights[INPUT_WEIGHTS * HIDDEN_WEIGHTS];
extern int16_t hiddenBias[HIDDEN_BIAS];
extern int16_t hiddenWeights[HIDDEN_WEIGHTS];
extern int32_t outputBias[OUTPUT_BIAS];

struct State
{
    Square enPassant{};
    uint8_t castling{};
    uint8_t halfMove{};
    Piece capturedPiece = None;
    std::array<File, 2> chess960White = {};
    std::array<File, 2> chess960Black = {};
    State(Square enpassantCopy = {}, uint8_t castlingRightsCopy = {}, uint8_t halfMoveCopy = {},
          Piece capturedPieceCopy = None, std::array<File, 2> c960W = {NO_FILE}, std::array<File, 2> c960B = {NO_FILE})
        : enPassant(enpassantCopy), castling(castlingRightsCopy), halfMove(halfMoveCopy),
          capturedPiece(capturedPieceCopy), chess960White(c960W), chess960Black(c960B)
    {
    }
};

class Board
{
  public:
    bool chess960 = false;
    std::array<File, 2> castlingRights960White = {NO_FILE};
    std::array<File, 2> castlingRights960Black = {NO_FILE};

    Color sideToMove;

    // NO_SQ when enpassant is not possible
    Square enPassantSquare;

    // castling right is encoded in 8bit
    // wk = 1
    // wq = 2
    // bk = 4
    // bq = 8
    // 0  0  0  0  0  0  0  0
    // wk wq bk bq
    uint8_t castlingRights;

    // halfmoves start at 0
    uint8_t halfMoveClock;

    // full moves start at 1
    uint16_t fullMoveNumber;

    // keeps track of previous hashes, used for
    // repetition detection
    std::vector<U64> hashHistory;

    // keeps track on how many checks there currently are
    // 2 = only king moves are valid
    // 1 = king move, block/capture
    uint8_t doubleCheck;

    // the path between horizontal/vertical pinners and
    // the pinned is set
    U64 pinHV;

    // the path between diagonal pinners and
    // the pinned is set
    U64 pinD;

    // all bits set if we are not in check
    // otherwise the path between the king and the checker
    // in case of knights giving check only the knight square
    // is checked
    U64 checkMask = DEFAULT_CHECKMASK;

    // all squares that are seen by an enemy piece
    U64 seen;

    // Occupation Bitboards
    U64 occEnemy;
    U64 occUs;
    U64 occAll;
    U64 enemyEmptyBB;

    // current hashkey
    U64 hashKey;

    U64 SQUARES_BETWEEN_BB[MAX_SQ][MAX_SQ];

    std::vector<State> stateHistory;

    U64 Bitboards[12] = {};
    Piece board[MAX_SQ];

    /// @brief constructor for the board, loads startpos and initializes SQUARES_BETWEEN_BB array
    Board();

    /// @brief reload the entire nnue
    void accumulate();

    /// @brief Finds what piece is on the square using bitboards (slow)
    /// @param sq
    /// @return found piece otherwise None
    Piece pieceAtBB(Square sq);

    /// @brief Finds what piece is on the square using the board (more performant)
    /// @param sq
    /// @return found piece otherwise None
    Piece pieceAtB(Square sq) const;

    /// @brief applys a new Fen to the board and also reload the entire nnue
    /// @param fen
    /// @param updateAcc
    void applyFen(std::string fen, bool updateAcc = true);

    /// @brief returns a Fen string of the current board
    /// @return fen string
    std::string getFen() const;

    /// @brief detects if the position is a repetition by default 1, fide would be 3
    /// @param draw
    /// @return true for repetition otherwise false
    bool isRepetition(int draw = 1) const;

    /// @brief only pawns + king = true else false
    /// @param c
    /// @return
    bool nonPawnMat(Color c) const;

    /// @brief returns the King Square of the specified color
    /// @param c
    /// @return
    Square KingSQ(Color c) const;

    /// @brief returns the King Square of the specified color
    /// @tparam c
    /// @return
    template <Color c> Square KingSQ() const;

    /// @brief returns all pieces of the other color
    /// @tparam c
    /// @return
    template <Color c> U64 Enemy() const;

    /// @brief returns a bitboard of our pieces
    /// @param c
    /// @return
    U64 Us(Color c) const;

    /// @brief returns a bitboard of our pieces
    /// @tparam c
    /// @return
    template <Color c> U64 Us() const;

    /// @brief returns all empty squares or squares with an enemy on them
    /// @param c
    /// @return
    U64 EnemyEmpty(Color c) const;

    /// @brief returns all pieces
    /// @return
    U64 All() const;

    // Gets individual bitboards

    U64 Pawns(const Color c) const;
    U64 Knights(const Color c) const;
    U64 Bishops(const Color c) const;
    U64 Rooks(const Color c) const;
    U64 Queens(const Color c) const;
    U64 Kings(const Color c) const;
    template <Color c> U64 Pawns() const;
    template <Color c> U64 Knights() const;
    template <Color c> U64 Bishops() const;
    template <Color c> U64 Rooks() const;
    template <Color c> U64 Queens() const;
    template <Color c> U64 Kings() const;

    /// @brief returns the color of a piece at a square
    /// @param loc
    /// @return
    Color colorOf(Square loc) const;

    /// @brief is square attacked by color c
    /// @param c
    /// @param sq
    /// @return
    bool isSquareAttacked(Color c, Square sq) const;

    /// @brief
    /// @param c
    /// @param sq
    /// @param occ
    /// @return
    bool isSquareAttacked(Color c, Square sq, U64 occ) const;

    // attackers used for SEE
    U64 allAttackers(Square sq, U64 occupiedBB);
    U64 attackersForSide(Color attackerColor, Square sq, U64 occupiedBB);

    /// @brief plays the move on the internal board
    /// @tparam updateNNUE update true = update nnue
    /// @param move
    template <bool updateNNUE> void makeMove(Move move);

    /// @brief unmake a move played on the internal board
    /// @tparam updateNNUE update true = update nnue
    /// @param move
    template <bool updateNNUE> void unmakeMove(Move move);

    /// @brief make a nullmove
    void makeNullMove();

    /// @brief unmake a nullmove
    void unmakeNullMove();

    const NNUE::accumulator &getAccumulator() const;

    // update the internal board representation

    /// @brief Remove a Piece from the board
    /// @tparam update true = update nnue
    /// @param piece
    /// @param sq
    template <bool updateNNUE> void removePiece(Piece piece, Square sq);

    /// @brief Place a Piece on the board
    /// @tparam update
    /// @param piece
    /// @param sq
    template <bool updateNNUE> void placePiece(Piece piece, Square sq);

    /// @brief Move a piece on the board
    /// @tparam updateNNUE
    /// @param piece
    /// @param fromSq
    /// @param toSq
    template <bool updateNNUE> void movePiece(Piece piece, Square fromSq, Square toSq);

    bool isLegal(const Move move);

    bool isPseudoLegal(const Move move);

    U64 attacksByPiece(PieceType pt, Square sq, Color c);

    U64 attacksByPiece(PieceType pt, Square sq, Color c, U64 occupied);

    /********************
     * Static Exchange Evaluation, logical based on Weiss (https://github.com/TerjeKir/weiss) licensed under GPL-3.0
     *******************/
    bool see(Move move, int threshold);

    friend std::ostream &operator<<(std::ostream &os, const Board &b);

  private:
    /// @brief current accumulator
    NNUE::accumulator accumulator;

    /// @brief previous accumulators
    std::vector<NNUE::accumulator> accumulatorStack;

    /// @brief calculate the current zobrist hash from scratch
    /// @return
    U64 zobristHash() const;

    /// @brief initialize SQUARES_BETWEEN_BB array
    void initializeLookupTables();

    // update the hash

    U64 updateKeyPiece(Piece piece, Square sq) const;
    U64 updateKeyCastling() const;
    U64 updateKeyEnPassant(Square sq) const;
    U64 updateKeySideToMove() const;

    void removeCastlingRightsAll(Color c);
    void removeCastlingRightsRook(Square sq);
};

template <bool updateNNUE> void Board::removePiece(Piece piece, Square sq)
{
    Bitboards[piece] &= ~(1ULL << sq);
    board[sq] = None;
    if constexpr (updateNNUE)
    {
        NNUE::deactivate(accumulator, sq, piece);
    }
}

template <bool updateNNUE> void Board::placePiece(Piece piece, Square sq)
{
    Bitboards[piece] |= (1ULL << sq);
    board[sq] = piece;
    if constexpr (updateNNUE)
    {
        NNUE::activate(accumulator, sq, piece);
    }
}

template <bool updateNNUE> void Board::movePiece(Piece piece, Square fromSq, Square toSq)
{
    Bitboards[piece] &= ~(1ULL << fromSq);
    Bitboards[piece] |= (1ULL << toSq);
    board[fromSq] = None;
    board[toSq] = piece;
    if constexpr (updateNNUE)
    {
        NNUE::move(accumulator, fromSq, toSq, piece);
    }
}

template <Color c> U64 Board::Pawns() const
{
    return Bitboards[c == White ? WhitePawn : BlackPawn];
}

template <Color c> U64 Board::Knights() const
{
    return Bitboards[c == White ? WhiteKnight : BlackKnight];
}

template <Color c> U64 Board::Bishops() const
{
    return Bitboards[c == White ? WhiteBishop : BlackBishop];
}

template <Color c> U64 Board::Rooks() const
{
    return Bitboards[c == White ? WhiteRook : BlackRook];
}

template <Color c> U64 Board::Queens() const
{
    return Bitboards[c == White ? WhiteQueen : BlackQueen];
}

template <Color c> U64 Board::Kings() const
{
    assert(Bitboards[c == White ? WhiteKing : BlackKing] != 0);

    return Bitboards[c == White ? WhiteKing : BlackKing];
}

template <Color c> U64 Board::Us() const
{
    return Pawns<c>() | Knights<c>() | Bishops<c>() | Rooks<c>() | Queens<c>() | Kings<c>();
}

template <Color c> U64 Board::Enemy() const
{
    assert(Us(~c) != 0);

    return Us<~c>();
}

template <Color c> Square Board::KingSQ() const
{
    assert(Kings<c>() != 0);
    return lsb(Kings<c>());
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

    const State store =
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

    TTable.prefetchTT(hashKey);

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
    const State restore = stateHistory.back();
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

/// @brief get uci representation of a move
/// @param board
/// @param move
/// @return
std::string uciMove(const Board &board, Move move);

static constexpr int piece_values[2][7] = {{98, 337, 365, 477, 1025, 0, 0}, {114, 281, 297, 512, 936, 0, 0}};
static constexpr int pieceValuesDefault[7] = {100, 320, 330, 500, 900, 0, 0};
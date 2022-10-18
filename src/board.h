#pragma once

#include <array>
#include <bitset>
#include <iostream>
#include <string>

#include "attacks.h"
#include "helper.h"
#include "nnue.h"
#include "tt.h"
#include "types.h"
#include "zobrist.h"

extern uint8_t inputValues[INPUT_WEIGHTS];
extern int16_t inputWeights[INPUT_WEIGHTS * HIDDEN_WEIGHTS];
extern int16_t hiddenBias[HIDDEN_BIAS];
extern int16_t hiddenWeights[HIDDEN_WEIGHTS];
extern int32_t outputBias[OUTPUT_BIAS];

extern TEntry *TTable;

struct State
{
    Square enPassant{};
    uint8_t castling{};
    uint8_t halfMove{};
    Piece capturedPiece = None;
    State(Square enpassantCopy = {}, uint8_t castlingRightsCopy = {}, uint8_t halfMoveCopy = {},
          Piece capturedPieceCopy = None)
        : enPassant(enpassantCopy), castling(castlingRightsCopy), halfMove(halfMoveCopy),
          capturedPiece(capturedPieceCopy)
    {
    }
};

class Board
{
  public:
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

    // King's Super Piece Attacks
    U64 kingHV;
    U64 kingD;
    U64 kingFFD;
    U64 kingP;

    // Occupation Bitboards
    U64 occEnemy;
    U64 occUs;
    U64 occAll;
    U64 enemyEmptyBB;

    // current hashkey
    U64 hashKey;

    U64 SQUARES_BETWEEN_BB[MAX_SQ][MAX_SQ];

    std::vector<State> prevStates;

    U64 Bitboards[12] = {};
    Piece board[MAX_SQ];

    Board();

    // load all weights and inputs from scratch
    void accumulate();

    // Finds what piece is on the square using the bitboards
    Piece pieceAtBB(Square sq);

    // Finds what piece is on the square using the board (more performant)
    Piece pieceAtB(Square sq);

    // applys a new Fen to the board
    void applyFen(std::string fen, bool updateAcc = true);

    // returns a Fen string of the current board
    std::string getFen();

    // prints any bitboard
    void printBitboard(U64 bb);

    // prints the current board
    void print();

    // detects if the position is a repetition by default 2, fide would be 3
    bool isRepetition(int draw = 1);

    // only pawns + king = true else false
    bool nonPawnMat(Color c);

    // returns the King Square of the specified color
    Square KingSQ(Color c);

    // returns the King Square of the specified color
    template <Color c> Square KingSQ();

    // returns all pieces of the other color
    U64 Enemy(Color c);

    // returns all pieces of the other color
    template <Color c> U64 Enemy();

    // returns a bitboard of our pieces
    U64 Us(Color c);

    // returns a bitboard of our pieces
    template <Color c> U64 Us();

    // returns all empty squares or squares with an enemy on them
    U64 EnemyEmpty(Color c);

    // returns all pieces color
    U64 All();

    // Gets the piece of the specified color

    U64 Pawns(Color c);
    U64 Knights(Color c);
    U64 Bishops(Color c);
    U64 Rooks(Color c);
    U64 Queens(Color c);
    U64 Kings(Color c);
    template <Color c> U64 Pawns();
    template <Color c> U64 Knights();
    template <Color c> U64 Bishops();
    template <Color c> U64 Rooks();
    template <Color c> U64 Queens();
    template <Color c> U64 Kings();

    // Is square attacked by color c
    bool isSquareAttacked(Color c, Square sq);

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

    // make a nullmove
    void makeNullMove();

    // unmake a nullmove
    void unmakeNullMove();

    std::array<int16_t, HIDDEN_BIAS> &getAccumulator();

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

  private:
    std::array<int16_t, HIDDEN_BIAS> accumulator;
    std::vector<std::array<int16_t, HIDDEN_BIAS>> accumulatorStack;

    // calculate the current zobrist hash from scratch
    U64 zobristHash();

    // initialize SQUARES_BETWEEN_BB array
    void initializeLookupTables();

    // update the hash

    U64 updateKeyPiece(Piece piece, Square sq);
    U64 updateKeyCastling();
    U64 updateKeyEnPassant(Square sq);
    U64 updateKeySideToMove();

    void removeCastlingRightsAll(Color c);
    void removeCastlingRightsRook(Color c, Square sq);
};

template <bool updateNNUE> void Board::removePiece(Piece piece, Square sq)
{
    Bitboards[piece] &= ~(1ULL << sq);
    board[sq] = None;
    if constexpr (updateNNUE)
        NNUE::deactivate(accumulator, sq + piece * 64);
}

template <bool updateNNUE> void Board::placePiece(Piece piece, Square sq)
{
    Bitboards[piece] |= (1ULL << sq);
    board[sq] = piece;
    if constexpr (updateNNUE)
        NNUE::activate(accumulator, sq + piece * 64);
}

template <Color c> U64 Board::Pawns()
{
    return Bitboards[c == White ? WhitePawn : BlackPawn];
}

template <Color c> U64 Board::Knights()
{
    return Bitboards[c == White ? WhiteKnight : BlackKnight];
}

template <Color c> U64 Board::Bishops()
{
    return Bitboards[c == White ? WhiteBishop : BlackBishop];
}

template <Color c> U64 Board::Rooks()
{
    return Bitboards[c == White ? WhiteRook : BlackRook];
}

template <Color c> U64 Board::Queens()
{
    return Bitboards[c == White ? WhiteQueen : BlackQueen];
}

template <Color c> U64 Board::Kings()
{
    return Bitboards[c == White ? WhiteKing : BlackKing];
}

template <Color c> U64 Board::Us()
{
    return Pawns<c>() | Knights<c>() | Bishops<c>() | Rooks<c>() | Queens<c>() | Kings<c>();
}

template <Color c> U64 Board::Enemy()
{
    return Us<~c>();
}

template <Color c> Square Board::KingSQ()
{
    return bsf(Kings<c>());
}
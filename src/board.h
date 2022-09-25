#pragma once
#include <array>
#include <bitset>
#include <iostream>
#include <map>
#include <string>

#include "attacks.h"
#include "helper.h"
#include "nnue.h"
#include "scores.h"
#include "tt.h"
#include "types.h"
#include "zobrist.h"

extern uint8_t inputValues[INPUT_WEIGHTS];
extern int16_t inputWeights[INPUT_WEIGHTS * HIDDEN_WEIGHTS];
extern int16_t hiddenBias[HIDDEN_BIAS];
extern int16_t hiddenWeights[HIDDEN_WEIGHTS];
extern int32_t outputBias[OUTPUT_BIAS];

static constexpr U64 DEFAULT_CHECKMASK = 18446744073709551615ULL;

extern TEntry *TTable;
struct State
{
    Square enPassant{};
    uint8_t castling{};
    uint8_t halfMove{};
    Piece capturedPiece = None;
    State(Square enpassantCopy = {}, uint8_t castlingRightsCopy = {}, uint8_t halfMoveCopy = {},
          Piece capturedPieceCopy = None, U64 hashCopy = {})
        : enPassant(enpassantCopy), castling(castlingRightsCopy), halfMove(halfMoveCopy),
          capturedPiece(capturedPieceCopy)
    {
    }
};

struct States
{
    std::vector<State> list;

    void Add(State s)
    {
        list.push_back(s);
    }

    State Get()
    {
        State s = list.back();
        list.pop_back();
        return s;
    }
};

struct Movelist
{
    Move list[MAX_MOVES]{};
    int values[MAX_MOVES]{};
    uint8_t size{};

    void Add(Move move)
    {
        list[size] = move;
        size++;
    }
};

class Board
{
  public:
    uint8_t castlingRights{};
    Square enPassantSquare{};
    uint8_t halfMoveClock{};
    uint16_t fullMoveNumber{};
    Color sideToMove = White;

    Piece board[MAX_SQ]{None};
    U64 Bitboards[12]{};
    U64 SQUARES_BETWEEN_BB[MAX_SQ][MAX_SQ]{};

    // all bits set
    U64 checkMask = DEFAULT_CHECKMASK;

    U64 pinHV{};
    U64 pinD{};
    uint8_t doubleCheck{};
    U64 hashKey{};
    U64 occUs;
    U64 occEnemy;
    U64 occAll{};
    U64 enemyEmptyBB{};
    U64 seen{};

    std::vector<U64> hashHistory;
    States prevStates;

    std::array<int16_t, HIDDEN_BIAS> accumulator;
    std::vector<std::array<int16_t, HIDDEN_BIAS>> accumulatorStack;

    void accumulate();

    Board();
    U64 zobristHash();

    void initializeLookupTables();

    // Finds what piece are on the square using the bitboards
    Piece pieceAtBB(Square sq);

    // Finds what piece are on the square using the board (more performant)
    Piece pieceAtB(Square sq);

    // aplys a new Fen to the board
    void applyFen(std::string fen, bool updateAcc = true);

    // returns a Fen string of the current board
    std::string getFen();

    // prints any bitboard passed to it
    void printBitboard(U64 bb);

    // prints the board
    void print();

    // makes a Piece from only the piece type and color
    Piece makePiece(PieceType type, Color c);

    // detects if the position is a repetition by default 2, fide would be 3
    bool isRepetition(int draw = 1);

    // only pawns + king = true else false
    bool nonPawnMat(Color c);

    // Gets the piece of the specified color
    U64 Pawns(Color c);
    U64 Knights(Color c);
    U64 Bishops(Color c);
    U64 Rooks(Color c);
    U64 Queens(Color c);
    U64 Kings(Color c);

    // can also updates accumulator
    template <bool update> void removePiece(Piece piece, Square sq);
    template <bool update> void placePiece(Piece piece, Square sq);

    U64 updateKeyPiece(Piece piece, Square sq);
    U64 updateKeyCastling();
    U64 updateKeyEnPassant(Square sq);
    U64 updateKeySideToMove();

    // returns the King Square of the specified color
    Square KingSQ(Color c);

    // returns all pieces of color
    U64 Us(Color c);

    // returns all pieces of the other color
    U64 Enemy(Color c);

    // returns all pieces color
    U64 All();

    // returns all empty squares
    U64 Empty();

    // returns all empty squares or squares with an enemy on them
    U64 EnemyEmpty(Color c);

    // creates the checkmask
    U64 DoCheckmask(Color c, Square sq);

    // creates the pinmask
    void DoPinMask(Color c, Square sq);

    // seen squares
    void seenSquares(Color c);

    // creates the pinmask and checkmask
    void init(Color c, Square sq);

    // Is square attacked by color c
    bool isSquareAttacked(Color c, Square sq);
    bool isSquareAttackedStatic(Color c, Square sq);
    U64 allAttackers(Square sq, U64 occupiedBB);
    U64 attackersForSide(Color attackerColor, Square sq, U64 occupiedBB);

    // returns a pawn push (only 1 square)
    U64 PawnPushSingle(Color c, Square sq);
    U64 PawnPushBoth(Color c, Square sq);

    // pseudo legal moves number estimation
    int pseudoLegalMovesNumber();

    // legal captures + promotions
    U64 LegalPawnNoisy(Color c, Square sq, Square ep);
    U64 LegalKingCaptures(Square sq);

    // all legal moves for each piece
    U64 LegalPawnMoves(Color c, Square sq);
    U64 LegalPawnMovesEP(Color c, Square sq, Square ep);
    U64 LegalKnightMoves(Square sq);
    U64 LegalBishopMoves(Square sq);
    U64 LegalRookMoves(Square sq);
    U64 LegalQueenMoves(Square sq);
    U64 LegalKingMoves(Square sq);
    U64 LegalKingMovesCastling(Color c, Square sq);

    // all legal moves for a position
    Movelist legalmoves();
    Movelist capturemoves();
    bool hasLegalMoves();

    // plays the move on the internal board
    template <bool updateNNUE> void makeMove(Move move);
    template <bool updateNNUE> void unmakeMove(Move move);
    void makeNullMove();
    void unmakeNullMove();
};

template <bool update> void Board::removePiece(Piece piece, Square sq)
{
    Bitboards[piece] &= ~(1ULL << sq);
    board[sq] = None;
    if constexpr (update)
        NNUE::deactivate(accumulator, sq + piece * 64);
}

template <bool update> void Board::placePiece(Piece piece, Square sq)
{
    Bitboards[piece] |= (1ULL << sq);
    board[sq] = piece;
    if constexpr (update)
        NNUE::activate(accumulator, sq + piece * 64);
}
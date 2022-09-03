#pragma once
#include <array>
#include <bitset>
#include <iostream>
#include <map>
#include <string>

#include "helper.h"
#include "nnue.h"
#include "scores.h"
#include "types.h"
#include "zobrist.h"

extern uint8_t inputValues[INPUT_WEIGHTS];
extern int16_t inputWeights[INPUT_WEIGHTS * HIDDEN_WEIGHTS];
extern int16_t hiddenBias[HIDDEN_BIAS];
extern int16_t hiddenWeights[HIDDEN_WEIGHTS];
extern int32_t outputBias[OUTPUT_BIAS];

static constexpr U64 DEFAULT_CHECKMASK = 18446744073709551615ULL;

struct State
{
    Square enPassant{};
    uint8_t castling{};
    uint8_t halfMove{};
    Piece capturedPiece = None;
    U64 h{};
    State(Square enpassantCopy = {}, uint8_t castlingRightsCopy = {}, uint8_t halfMoveCopy = {},
          Piece capturedPieceCopy = None, U64 hashCopy = {})
        : enPassant(enpassantCopy), castling(castlingRightsCopy), halfMove(halfMoveCopy),
          capturedPiece(capturedPieceCopy), h(hashCopy)
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

    // Attacks of each piece
    U64 PawnAttacks(Square sq, Color c);
    U64 KnightAttacks(Square sq);
    U64 BishopAttacks(Square sq, U64 occupied);
    U64 RookAttacks(Square sq, U64 occupied);
    U64 QueenAttacks(Square sq, U64 occupied);
    U64 KingAttacks(Square sq);

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

template <bool updateNNUE> void Board::makeMove(Move move)
{
    Piece p = makePiece(piece(move), sideToMove);
    Square from_sq = from(move);
    Square to_sq = to(move);
    Piece capture = board[to_sq];

    hashHistory.emplace_back(hashKey);

    State store = State(enPassantSquare, castlingRights, halfMoveClock, capture, hashKey);
    prevStates.Add(store);

    if constexpr (updateNNUE)
        accumulatorStack.emplace_back(accumulator);

    halfMoveClock++;
    fullMoveNumber++;

    bool ep = to_sq == enPassantSquare;

    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);
    enPassantSquare = NO_SQ;

    if (piece(move) == KING)
    {
        if (sideToMove == White && from_sq == SQ_E1 && to_sq == SQ_G1 && castlingRights & wk)
        {
            removePiece<updateNNUE>(WhiteRook, SQ_H1);
            placePiece<updateNNUE>(WhiteRook, SQ_F1);

            hashKey ^= updateKeyPiece(WhiteRook, SQ_H1);
            hashKey ^= updateKeyPiece(WhiteRook, SQ_F1);
        }
        else if (sideToMove == White && from_sq == SQ_E1 && to_sq == SQ_C1 && castlingRights & wq)
        {
            removePiece<updateNNUE>(WhiteRook, SQ_A1);
            placePiece<updateNNUE>(WhiteRook, SQ_D1);

            hashKey ^= updateKeyPiece(WhiteRook, SQ_A1);
            hashKey ^= updateKeyPiece(WhiteRook, SQ_D1);
        }
        else if (sideToMove == Black && from_sq == SQ_E8 && to_sq == SQ_G8 && castlingRights & bk)
        {
            removePiece<updateNNUE>(BlackRook, SQ_H8);
            placePiece<updateNNUE>(BlackRook, SQ_F8);

            hashKey ^= updateKeyPiece(BlackRook, SQ_H8);
            hashKey ^= updateKeyPiece(BlackRook, SQ_F8);
        }
        else if (sideToMove == Black && from_sq == SQ_E8 && to_sq == SQ_C8 && castlingRights & bq)
        {
            removePiece<updateNNUE>(BlackRook, SQ_A8);
            placePiece<updateNNUE>(BlackRook, SQ_D8);

            hashKey ^= updateKeyPiece(BlackRook, SQ_A8);
            hashKey ^= updateKeyPiece(BlackRook, SQ_D8);
        }
        hashKey ^= updateKeyCastling();
        if (sideToMove == White)
        {
            castlingRights &= ~(wk | wq);
        }
        else
        {
            castlingRights &= ~(bk | bq);
        }
        hashKey ^= updateKeyCastling();
    }
    else if (piece(move) == ROOK)
    {
        hashKey ^= updateKeyCastling();
        if (sideToMove == White && from_sq == SQ_A1)
        {
            castlingRights &= ~wq;
        }
        else if (sideToMove == White && from_sq == SQ_H1)
        {
            castlingRights &= ~wk;
        }
        else if (sideToMove == Black && from_sq == SQ_A8)
        {
            castlingRights &= ~bq;
        }
        else if (sideToMove == Black && from_sq == SQ_H8)
        {
            castlingRights &= ~bk;
        }
        hashKey ^= updateKeyCastling();
    }
    else if (piece(move) == PAWN)
    {
        halfMoveClock = 0;
        if (ep)
        {
            removePiece<updateNNUE>(makePiece(PAWN, ~sideToMove), Square(to_sq - (sideToMove * -2 + 1) * 8));
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
        removePiece<updateNNUE>(capture, to_sq);
        hashKey ^= updateKeyPiece(capture, to_sq);
        if (capture % 6 == ROOK)
        {
            hashKey ^= updateKeyCastling();
            if (to_sq == SQ_A1)
            {
                castlingRights &= ~wq;
            }
            else if (to_sq == SQ_H1)
            {
                castlingRights &= ~wk;
            }
            else if (to_sq == SQ_A8)
            {
                castlingRights &= ~bq;
            }
            else if (to_sq == SQ_H8)
            {
                castlingRights &= ~bk;
            }
            hashKey ^= updateKeyCastling();
        }
    }

    if (promoted(move))
    {
        halfMoveClock = 0;
        removePiece<updateNNUE>(makePiece(PAWN, sideToMove), from_sq);
        placePiece<updateNNUE>(p, to_sq);

        hashKey ^= updateKeyPiece(makePiece(PAWN, sideToMove), from_sq);
        hashKey ^= updateKeyPiece(p, to_sq);
    }
    else
    {
        removePiece<updateNNUE>(p, from_sq);
        hashKey ^= updateKeyPiece(p, from_sq);
        placePiece<updateNNUE>(p, to_sq);
        hashKey ^= updateKeyPiece(p, to_sq);
    }
    sideToMove = ~sideToMove;
    hashKey ^= updateKeySideToMove();
}

template <bool updateNNUE> void Board::unmakeMove(Move move)
{
    State restore = prevStates.Get();
    enPassantSquare = restore.enPassant;
    castlingRights = restore.castling;
    halfMoveClock = restore.halfMove;
    Piece capture = restore.capturedPiece;
    hashKey = restore.h;
    fullMoveNumber--;

    if (accumulatorStack.size() > 0)
    {
        accumulator = accumulatorStack.back();
        accumulatorStack.pop_back();
    }

    hashHistory.pop_back();

    Square from_sq = from(move);
    Square to_sq = to(move);
    bool promotion = promoted(move);
    sideToMove = ~sideToMove;
    Piece p = makePiece(piece(move), sideToMove);

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

    if (to_sq == enPassantSquare && piece(move) == PAWN)
    {
        int8_t offset = sideToMove == White ? -8 : 8;
        placePiece<updateNNUE>(makePiece(PAWN, ~sideToMove), Square(enPassantSquare + offset));
    }
    else if (capture != None)
    {
        placePiece<updateNNUE>(capture, to_sq);
    }
    else
    {
        if (piece(move) == KING)
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
}
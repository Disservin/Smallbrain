#pragma once

#include "board.h"
#include "helper.h"
#include "types.h"

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

namespace Movegen
{

// creates the checkmask
U64 DoCheckmask(Board &board, Square sq);

// creates the pinmask
void DoPinMask(Board &board, Square sq);

// seen squares
template <Color c> void seenSquares(Board &board);

// creates the pinmask and checkmask
template <Color c> void init(const Board &board, Square sq);

// returns a pawn push (only 1 square)
template <Color c> U64 PawnPushSingle(U64 occAll, Square sq);

// returns a double pawn push
template <Color c> U64 PawnPushBoth(U64 occAll, Square sq);

// all legal moves for each piece

// pawn moves
template <Color c> U64 LegalPawnMoves(const Board &board, Square sq);

// pawn moves with ep square
template <Color c> U64 LegalPawnMovesEP(const Board &board, Square sq);
U64 LegalKnightMoves(const Board &board, Square sq);
U64 LegalBishopMoves(const Board &board, Square sq);
U64 LegalRookMoves(const Board &board, Square sq);
U64 LegalQueenMoves(const Board &board, Square sq);

// legal king moves without castling
U64 LegalKingMoves(const Board &board, Square sq);

// legal king moves with castling
template <Color c> U64 LegalKingMovesCastling(const Board &board, Square sq);

// legal captures + promotions
template <Color c> U64 LegalPawnCaptures(Board &board, Square sq, Square ep);
U64 LegalKnightCaptures(const Board &board, Square sq);
U64 LegalBishopCaptures(const Board &board, Square sq);
U64 LegalRookCaptures(const Board &board, Square sq);
U64 LegalQueenCaptures(const Board &board, Square sq);
U64 LegalKingCaptures(const Board &board, Square sq);

// all legal moves for a position
template <Color c> Movelist legalmoves(Board &board);
template <Color c> Movelist capturemoves(Board &board);
template <Color c> bool hasLegalMoves(Board &board);
Movelist legalmoves(Board &board);
Movelist capturemoves(Board &board);
bool hasLegalMoves(Board &board);

// pseudo legal moves number estimation
template <Color c> int pseudoLegalMovesNumber(Board &board);
} // namespace Movegen

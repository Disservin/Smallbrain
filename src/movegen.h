#pragma once

#include "board.h"
#include "helper.h"
#include "types.h"

PACK(struct ExtMove {
    int value = -10 * VALUE_INFINITE;
    Move move;
});

struct Movelist
{
    ExtMove list[MAX_MOVES] = {};
    uint8_t size{};

    void Add(Move move)
    {
        list[size].move = move;
        list[size].value = 0;
        size++;
    }

    inline constexpr ExtMove &operator[](int i)
    {
        return list[i];
    }
};

inline constexpr bool operator>(const ExtMove &a, const ExtMove &b)
{
    return a.value > b.value;
}

inline constexpr bool operator<(const ExtMove &a, const ExtMove &b)
{
    return a.value < b.value;
}

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
template <Color c> U64 PawnPushBoth(U64 occAll, Square sq);
template <Direction direction> constexpr U64 shift(U64 b);

// all legal moves for each piece
template <Color c> U64 LegalPawnMovesSingle(const Board &board, Square sq);
template <Color c> U64 LegalPawnMovesEPSingle(Board &board, Square sq, Square ep);
template <Color c> void LegalPawnMovesAll(Board &board, Movelist &movelist);
template <Color c> void LegalPawnMovesCapture(Board &board, Movelist &movelist);

U64 LegalKnightMoves(const Board &board, Square sq);
U64 LegalBishopMoves(const Board &board, Square sq);
U64 LegalRookMoves(const Board &board, Square sq);
U64 LegalQueenMoves(const Board &board, Square sq);
U64 LegalKingMoves(const Board &board, Square sq);
template <Color c> U64 LegalKingMovesCastling(const Board &board, Square sq);

// legal captures + promotions
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

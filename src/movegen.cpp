#include "movegen.h"

namespace Movegen
{

template <Color c> int pseudoLegalMovesNumber(Board &board)
{
    int total = 0;
    U64 pawns_mask = board.Pawns<c>();
    U64 knights_mask = board.Knights<c>();
    U64 bishops_mask = board.Bishops<c>();
    U64 rooks_mask = board.Rooks<c>();
    U64 queens_mask = board.Queens<c>();
    while (pawns_mask)
    {
        Square from = poplsb(pawns_mask);
        total += popcount(PawnPushBoth<c>(board.occAll, from) | PawnAttacks(c, c));
    }
    while (knights_mask)
    {
        Square from = poplsb(knights_mask);
        total += popcount(KnightAttacks(from) & board.enemyEmptyBB);
    }
    while (bishops_mask)
    {
        Square from = poplsb(bishops_mask);
        total += popcount(BishopAttacks(from, board.occAll) & board.enemyEmptyBB);
    }
    while (rooks_mask)
    {
        Square from = poplsb(rooks_mask);
        total += popcount(RookAttacks(from, board.occAll) & board.enemyEmptyBB);
    }
    while (queens_mask)
    {
        Square from = poplsb(queens_mask);
        total += popcount(QueenAttacks(from, board.occAll) & board.enemyEmptyBB);
    }
    Square from = board.KingSQ<c>();
    total += popcount(KingAttacks(from) & board.enemyEmptyBB);
    return total;
}

template <Color c> bool hasLegalMoves(Board &board)
{
    init<c>(board, board.KingSQ<c>());

    Square from = board.KingSQ<c>();
    U64 moves = !board.castlingRights || board.checkMask != DEFAULT_CHECKMASK ? LegalKingMoves<ALL>(board, from)
                                                                              : LegalKingMovesCastling<c>(board, from);
    if (moves)
        return true;

    if (board.doubleCheck == 2)
        return false;

    U64 pawns_mask = board.Pawns<c>();
    U64 knights_mask = board.Knights<c>();
    U64 bishops_mask = board.Bishops<c>();
    U64 rooks_mask = board.Rooks<c>();
    U64 queens_mask = board.Queens<c>();

    const bool noEP = board.enPassantSquare == NO_SQ;

    while (pawns_mask)
    {
        Square from = poplsb(pawns_mask);
        U64 moves =
            noEP ? LegalPawnMovesSingle<c>(board, from) : LegalPawnMovesEPSingle<c>(board, from, board.enPassantSquare);
        while (moves)
        {
            return true;
        }
    }
    while (knights_mask)
    {
        Square from = poplsb(knights_mask);
        U64 moves = LegalKnightMoves<ALL>(board, from);
        while (moves)
        {
            return true;
        }
    }
    while (bishops_mask)
    {
        Square from = poplsb(bishops_mask);
        U64 moves = LegalBishopMoves<ALL>(board, from);
        while (moves)
        {
            return true;
        }
    }
    while (rooks_mask)
    {
        Square from = poplsb(rooks_mask);
        U64 moves = LegalRookMoves<ALL>(board, from);
        while (moves)
        {
            return true;
        }
    }
    while (queens_mask)
    {
        Square from = poplsb(queens_mask);
        U64 moves = LegalQueenMoves<ALL>(board, from);
        while (moves)
        {
            return true;
        }
    }

    return false;
}

bool hasLegalMoves(Board &board)
{
    if (board.sideToMove == White)
        return hasLegalMoves<White>(board);
    else
        return hasLegalMoves<Black>(board);
}
} // namespace Movegen
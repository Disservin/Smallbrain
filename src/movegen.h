#pragma once

#include "board.h"
#include "helper.h"
#include "types.h"

#include <iterator>

struct ExtMove
{
    int value = 0;
    Move move = NO_MOVE;
};

inline constexpr bool operator==(const ExtMove &a, const ExtMove &b)
{
    return a.move == b.move;
}

inline constexpr bool operator>(const ExtMove &a, const ExtMove &b)
{
    return a.value > b.value;
}

inline constexpr bool operator<(const ExtMove &a, const ExtMove &b)
{
    return a.value < b.value;
}

struct Movelist
{
    ExtMove list[MAX_MOVES] = {};
    uint8_t size = 0;
    typedef ExtMove *iterator;
    typedef const ExtMove *const_iterator;

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

    inline constexpr int find(Move m)
    {
        for (int i = 0; i < size; i++)
        {
            if (list[i].move == m)
                return i;
        }
        return -1;
    }

    iterator begin()
    {
        return (std::begin(list));
    }
    const_iterator begin() const
    {
        return (std::begin(list));
    }
    iterator end()
    {
        auto it = std::begin(list);
        std::advance(it, size);
        return it;
    }
    const_iterator end() const
    {
        auto it = std::begin(list);
        std::advance(it, size);
        return it;
    }
};

namespace Movegen
{

template <Color c> U64 pawnLeftAttacks(const U64 pawns)
{
    return c == White ? (pawns << 7) & ~MASK_FILE[FILE_H] : (pawns >> 7) & ~MASK_FILE[FILE_A];
}

template <Color c> U64 pawnRightAttacks(const U64 pawns)
{
    return c == White ? (pawns << 9) & ~MASK_FILE[FILE_A] : (pawns >> 9) & ~MASK_FILE[FILE_H];
}

/********************
 * Creates the checkmask.
 * A checkmask is the path from the enemy checker to our king.
 * Knight and pawns get themselves added to the checkmask, otherwise the path is added.
 * When there is no check at all all bits are set (DEFAULT_CHECKMASK)
 *******************/
template <Color c> U64 DoCheckmask(Board &board, Square sq)
{
    U64 Occ = board.occAll;
    U64 checks = 0ULL;
    U64 pawn_mask = board.pieces<PAWN, ~c>() & PawnAttacks(sq, c);
    U64 knight_mask = board.pieces<KNIGHT, ~c>() & KnightAttacks(sq);
    U64 bishop_mask = (board.pieces<BISHOP, ~c>() | board.pieces<QUEEN, ~c>()) & BishopAttacks(sq, Occ);
    U64 rook_mask = (board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>()) & RookAttacks(sq, Occ);

    /********************
     * We keep track of the amount of checks, in case there are
     * two checks on the board only the king can move!
     *******************/
    board.doubleCheck = 0;

    if (pawn_mask)
    {
        checks |= pawn_mask;
        board.doubleCheck++;
    }
    if (knight_mask)
    {
        checks |= knight_mask;
        board.doubleCheck++;
    }
    if (bishop_mask)
    {
        int8_t index = lsb(bishop_mask);

        // Now we add the path!
        checks |= board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        board.doubleCheck++;
    }
    if (rook_mask)
    {
        /********************
         * 3nk3/4P3/8/8/8/8/8/2K1R3 w - - 0 1, pawn promotes to queen or rook and
         * suddenly the same piecetype gives check two times
         * in that case we have a double check and can return early
         * because king moves dont require the checkmask.
         *******************/
        if (popcount(rook_mask) > 1)
        {
            board.doubleCheck = 2;
            return checks;
        }

        int8_t index = lsb(rook_mask);

        // Now we add the path!
        checks |= board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        board.doubleCheck++;
    }
    return checks;
}

/********************
 * Create the pinmask
 * We define the pin mask as the path from the enemy pinner
 * through the pinned piece to our king.
 * We assume that the king can do rook and bishop moves and generate the attacks
 * for these in case we hit an enemy rook/bishop/queen we might have a possible pin.
 * We need to confirm the pin because there could be 2 or more pieces from our king to
 * the possible pinner. We do this by simply using the popcount
 * of our pieces that lay on the pin mask, if it is only 1 piece then that piece is pinned.
 *******************/
template <Color c> void DoPinMask(Board &board, Square sq)
{
    U64 them = board.occEnemy;
    U64 rook_mask = (board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>()) & RookAttacks(sq, them);
    U64 bishop_mask = (board.pieces<BISHOP, ~c>() | board.pieces<QUEEN, ~c>()) & BishopAttacks(sq, them);

    board.pinD = 0ULL;
    board.pinHV = 0ULL;
    while (rook_mask)
    {
        Square index = poplsb(rook_mask);
        U64 possible_pin = (board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & board.occUs) == 1)
            board.pinHV |= possible_pin;
    }
    while (bishop_mask)
    {
        Square index = poplsb(bishop_mask);
        U64 possible_pin = (board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & board.occUs) == 1)
            board.pinD |= possible_pin;
    }
}

/********************
 * Seen squares
 * We keep track of all attacked squares by the enemy
 * this is used for king move generation.
 *******************/
template <Color c> void seenSquares(Board &board)
{
    U64 pawns = board.pieces<PAWN, c>();
    U64 knights = board.pieces<KNIGHT, c>();
    U64 queens = board.pieces<QUEEN, c>();
    U64 bishops = board.pieces<BISHOP, c>() | queens;
    U64 rooks = board.pieces<ROOK, c>() | queens;

    board.seen = 0ULL;

    // Remove our king
    Square kSq = board.KingSQ(~c);
    board.occAll &= ~(1ULL << kSq);

    board.seen |= pawnLeftAttacks<c>(pawns) | pawnRightAttacks<c>(pawns);

    while (knights)
    {
        Square index = poplsb(knights);
        board.seen |= KnightAttacks(index);
    }
    while (bishops)
    {
        Square index = poplsb(bishops);
        board.seen |= BishopAttacks(index, board.occAll);
    }
    while (rooks)
    {
        Square index = poplsb(rooks);
        board.seen |= RookAttacks(index, board.occAll);
    }

    Square index = lsb(board.pieces<KING, c>());
    board.seen |= KingAttacks(index);

    // Place our King back
    board.occAll |= (1ULL << kSq);
}

/********************
 * Creates the pinmask and checkmask
 * setup important variables that we use for move generation.
 *******************/
template <Color c> void init(Board &board, Square sq)
{
    board.occUs = board.Us<c>();
    board.occEnemy = board.Us<~c>();
    board.occAll = board.occUs | board.occEnemy;
    board.enemyEmptyBB = ~board.occUs;

    seenSquares<~c>(board);

    U64 newMask = DoCheckmask<c>(board, sq);
    board.checkMask = newMask ? newMask : DEFAULT_CHECKMASK;
    DoPinMask<c>(board, sq);
}

/// @brief shift a mask in a direction
/// @tparam direction
/// @param b
/// @return
template <Direction direction> constexpr U64 shift(const U64 b)
{
    switch (direction)
    {
    case NORTH:
        return b << 8;
    case SOUTH:
        return b >> 8;
    case NORTH_WEST:
        return (b & ~MASK_FILE[0]) << 7;
    case WEST:
        return (b & ~MASK_FILE[0]) >> 1;
    case SOUTH_WEST:
        return (b & ~MASK_FILE[0]) >> 9;
    case NORTH_EAST:
        return (b & ~MASK_FILE[7]) << 9;
    case EAST:
        return (b & ~MASK_FILE[7]) << 1;
    case SOUTH_EAST:
        return (b & ~MASK_FILE[7]) >> 7;
    }
}

// all legal moves for each piece

/// @brief all legal pawn moves, generated at once
/// @tparam c
/// @tparam mt
/// @param board
/// @param movelist
template <Color c, Movetype mt> void LegalPawnMovesAll(Board &board, Movelist &movelist)
{
    const U64 pawns_mask = board.pieces<PAWN, c>();

    constexpr Direction UP = c == White ? NORTH : SOUTH;
    constexpr Direction DOWN = c == Black ? NORTH : SOUTH;
    constexpr Direction DOWN_LEFT = c == Black ? NORTH_EAST : SOUTH_WEST;
    constexpr Direction DOWN_RIGHT = c == Black ? NORTH_WEST : SOUTH_EAST;
    constexpr U64 RANK_BEFORE_PROMO = c == White ? MASK_RANK[RANK_7] : MASK_RANK[RANK_2];
    constexpr U64 RANK_PROMO = c == White ? MASK_RANK[RANK_8] : MASK_RANK[RANK_1];
    constexpr U64 doublePushRank = c == White ? MASK_RANK[RANK_3] : MASK_RANK[RANK_6];

    // These pawns can maybe take Left or Right
    const U64 pawnsLR = pawns_mask & ~board.pinHV;

    const U64 unpinnedpawnsLR = pawnsLR & ~board.pinD;
    const U64 pinnedpawnsLR = pawnsLR & board.pinD;

    U64 Lpawns = (pawnLeftAttacks<c>(unpinnedpawnsLR)) | (pawnLeftAttacks<c>(pinnedpawnsLR) & board.pinD);
    U64 Rpawns = (pawnRightAttacks<c>(unpinnedpawnsLR)) | (pawnRightAttacks<c>(pinnedpawnsLR) & board.pinD);

    // Prune moves that dont capture a piece and are not on the checkmask.
    Lpawns &= board.occEnemy & board.checkMask;
    Rpawns &= board.occEnemy & board.checkMask;

    // These pawns can walk Forward
    const U64 pawnsHV = pawns_mask & ~board.pinD;

    const U64 pawnsPinnedHV = pawnsHV & board.pinHV;
    const U64 pawnsUnPinnedHV = pawnsHV & ~board.pinHV;

    // Prune moves that are blocked by a piece
    const U64 singlePushUnpinned = shift<UP>(pawnsUnPinnedHV) & ~board.occAll;
    const U64 singlePushPinned = shift<UP>(pawnsPinnedHV) & board.pinHV & ~board.occAll;

    // Prune moves that are not on the checkmask.
    U64 singlePush = (singlePushUnpinned | singlePushPinned) & board.checkMask;

    U64 doublePush = ((shift<UP>(singlePushUnpinned & doublePushRank) & ~board.occAll) |
                      (shift<UP>(singlePushPinned & doublePushRank) & ~board.occAll)) &
                     board.checkMask;

    /********************
     * Add promotion moves.
     * These are always generated unless we only want quiet moves.
     *******************/
    if ((mt != Movetype::QUIET) && pawns_mask & RANK_BEFORE_PROMO)
    {
        U64 Promote_Left = Lpawns & RANK_PROMO;
        U64 Promote_Right = Rpawns & RANK_PROMO;
        U64 Promote_Move = singlePush & RANK_PROMO;

        while (Promote_Move)
        {
            Square to = poplsb(Promote_Move);
            movelist.Add(make<QUEEN, true>(to + DOWN, to));
            movelist.Add(make<ROOK, true>(to + DOWN, to));
            movelist.Add(make<KNIGHT, true>(to + DOWN, to));
            movelist.Add(make<BISHOP, true>(to + DOWN, to));
        }

        while (Promote_Right)
        {
            Square to = poplsb(Promote_Right);
            movelist.Add(make<QUEEN, true>(to + DOWN_LEFT, to));
            movelist.Add(make<ROOK, true>(to + DOWN_LEFT, to));
            movelist.Add(make<KNIGHT, true>(to + DOWN_LEFT, to));
            movelist.Add(make<BISHOP, true>(to + DOWN_LEFT, to));
        }

        while (Promote_Left)
        {
            Square to = poplsb(Promote_Left);
            movelist.Add(make<QUEEN, true>(to + DOWN_RIGHT, to));
            movelist.Add(make<ROOK, true>(to + DOWN_RIGHT, to));
            movelist.Add(make<KNIGHT, true>(to + DOWN_RIGHT, to));
            movelist.Add(make<BISHOP, true>(to + DOWN_RIGHT, to));
        }
    }

    // Remove the promotion pawns
    singlePush &= ~RANK_PROMO;
    Lpawns &= ~RANK_PROMO;
    Rpawns &= ~RANK_PROMO;

    /********************
     * Add single pushs.
     *******************/
    while (mt != Movetype::CAPTURE && singlePush)
    {
        Square to = poplsb(singlePush);
        movelist.Add(make<PAWN, false>(to + DOWN, to));
    }

    /********************
     * Add double pushs.
     *******************/
    while (mt != Movetype::CAPTURE && doublePush)
    {
        Square to = poplsb(doublePush);
        movelist.Add(make<PAWN, false>(to + DOWN + DOWN, to));
    }

    /********************
     * Add right pawn captures.
     *******************/
    while (mt != Movetype::QUIET && Rpawns)
    {
        Square to = poplsb(Rpawns);
        movelist.Add(make<PAWN, false>(to + DOWN_LEFT, to));
    }

    /********************
     * Add left pawn captures.
     *******************/
    while (mt != Movetype::QUIET && Lpawns)
    {
        Square to = poplsb(Lpawns);
        movelist.Add(make<PAWN, false>(to + DOWN_RIGHT, to));
    }

    /********************
     * Add en passant captures.
     *******************/
    if (mt != Movetype::QUIET && board.enPassantSquare != NO_SQ)
    {
        const Square ep = board.enPassantSquare;
        const Square epPawn = ep + DOWN;

        U64 epMask = (1ull << epPawn) | (1ull << ep);

        /********************
         * In case the en passant square and the enemy pawn
         * that just moved are not on the checkmask
         * en passant is not available.
         *******************/
        if ((board.checkMask & epMask) == 0)
            return;

        const Square kSQ = board.KingSQ(c);
        const U64 kingMask = (1ull << kSQ) & MASK_RANK[square_rank(epPawn)];
        const U64 enemyQueenRook = board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>();

        const bool isPossiblePin = kingMask && enemyQueenRook;
        U64 epBB = PawnAttacks(ep, ~c) & pawnsLR;

        /********************
         * For one en passant square two pawns could potentially take there.
         *******************/
        while (epBB)
        {
            Square from = poplsb(epBB);
            Square to = ep;

            /********************
             * If the pawn is pinned but the en passant square is not on the
             * pin mask then the move is illegal.
             *******************/
            if ((1ULL << from) & board.pinD && !(board.pinD & (1ull << ep)))
                continue;

            const U64 connectingPawns = (1ull << epPawn) | (1ull << from);

            /********************
             * 7k/4p3/8/2KP3r/8/8/8/8 b - - 0 1
             * If e7e5 there will be a potential ep square for us on e6.
             * However we cannot take en passant because that would put our king
             * in check. For this scenario we check if theres an enemy rook/queen
             * that would give check if the two pawns were removed.
             * If thats the case then the move is illegal and we can break immediately.
             *******************/
            if (isPossiblePin && (RookAttacks(kSQ, board.occAll & ~connectingPawns) & enemyQueenRook) != 0)
                break;

            movelist.Add(make<PAWN, false>(from, to));
        }
    }
}

inline U64 LegalKnightMoves(Square sq, U64 movableSquare)
{
    return KnightAttacks(sq) & movableSquare;
}

inline U64 LegalBishopMoves(const Board &board, Square sq, U64 movableSquare)
{
    // The Bishop is pinned diagonally thus can only move diagonally.
    if (board.pinD & (1ULL << sq))
        return BishopAttacks(sq, board.occAll) & movableSquare & board.pinD;
    return BishopAttacks(sq, board.occAll) & movableSquare;
}

inline U64 LegalRookMoves(const Board &board, Square sq, U64 movableSquare)
{
    // The Rook is pinned horizontally thus can only move horizontally.
    if (board.pinHV & (1ULL << sq))
        return RookAttacks(sq, board.occAll) & movableSquare & board.pinHV;
    return RookAttacks(sq, board.occAll) & movableSquare;
}

inline U64 LegalQueenMoves(const Board &board, Square sq, U64 movableSquare)
{
    U64 moves = 0ULL;
    if (board.pinD & (1ULL << sq))
        moves |= BishopAttacks(sq, board.occAll) & movableSquare & board.pinD;
    else if (board.pinHV & (1ULL << sq))
        moves |= RookAttacks(sq, board.occAll) & movableSquare & board.pinHV;
    else
    {
        moves |= RookAttacks(sq, board.occAll) & movableSquare;
        moves |= BishopAttacks(sq, board.occAll) & movableSquare;
    }

    return moves;
}

template <Movetype mt> U64 LegalKingMoves(const Board &board, Square sq)
{
    U64 bb;

    if (mt == Movetype::ALL)
        bb = board.enemyEmptyBB;
    else if (mt == Movetype::CAPTURE)
        bb = board.occEnemy;
    else
        bb = ~board.occAll;

    return KingAttacks(sq) & bb & ~board.seen;
}

template <Color c, Movetype mt> U64 LegalKingMovesCastling(const Board &board, Square sq)
{
    U64 bb;

    if (mt == Movetype::ALL)
        bb = board.enemyEmptyBB;
    else
        bb = ~board.occAll;

    U64 moves = KingAttacks(sq) & bb & ~board.seen;
    U64 emptyAndNotAttacked = ~board.seen & ~board.occAll;

    switch (c)
    {
    case White:
        if (board.castlingRights & wk && emptyAndNotAttacked & (1ULL << SQ_F1) && emptyAndNotAttacked & (1ull << SQ_G1))
            moves |= (1ULL << SQ_H1);
        if (board.castlingRights & wq && emptyAndNotAttacked & (1ULL << SQ_D1) &&
            emptyAndNotAttacked & (1ull << SQ_C1) && (1ull << SQ_B1) & ~board.occAll)
            moves |= (1ULL << SQ_A1);
        break;
    case Black:
        if (board.castlingRights & bk && emptyAndNotAttacked & (1ULL << SQ_F8) && emptyAndNotAttacked & (1ull << SQ_G8))
            moves |= (1ULL << SQ_H8);
        if (board.castlingRights & bq && emptyAndNotAttacked & (1ULL << SQ_D8) &&
            emptyAndNotAttacked & (1ull << SQ_C8) && (1ull << SQ_B8) & ~board.occAll)
            moves |= (1ULL << SQ_A8);

        break;
    default:
        return moves;
    }

    return moves;
}

template <Color c, Movetype mt> U64 LegalKingMovesCastling960(const Board &board, Square sq)
{
    U64 bb;

    if (mt == Movetype::ALL)
        bb = board.enemyEmptyBB;
    else
        bb = ~board.occAll;

    U64 moves = KingAttacks(sq) & bb & ~board.seen;

    switch (c)
    {
    case White: {
        for (int i = 0; i < 2; i++)
        {
            int side = board.castlingRights960White[i];
            Square destRook = side >= square_file(sq) ? SQ_F1 : SQ_D1;
            Square destKing = side >= square_file(sq) ? SQ_G1 : SQ_C1;
            U64 withoutRook = board.occAll & ~(1ull << side);
            U64 emptyAndNotAttacked = ~board.seen & ~(board.occAll & ~(1ull << side));

            if (side != NO_FILE &&
                (board.SQUARES_BETWEEN_BB[sq][destKing] & emptyAndNotAttacked) ==
                    board.SQUARES_BETWEEN_BB[sq][destKing] &&
                (board.SQUARES_BETWEEN_BB[sq][side] & ~board.occAll) == board.SQUARES_BETWEEN_BB[sq][side] &&
                !((1ull << side) & board.pinHV & MASK_RANK[RANK_1]) && !((1ull << destRook) & withoutRook) &&
                !((1ull << destKing) & (board.seen | (withoutRook & ~(1ull << sq)))))
                moves |= (1ULL << side);
        }
        break;
    }

    case Black: {
        for (int i = 0; i < 2; i++)
        {
            File side = board.castlingRights960Black[i];
            Square destRook = side >= square_file(sq) ? SQ_F8 : SQ_D8;
            Square destKing = side >= square_file(sq) ? SQ_G8 : SQ_C8;
            U64 withoutRook = board.occAll & ~(1ull << (56 + side));
            U64 emptyAndNotAttacked = ~board.seen & ~(board.occAll & ~(1ull << (56 + side)));

            if (side != NO_FILE &&
                (board.SQUARES_BETWEEN_BB[sq][destKing] & emptyAndNotAttacked) ==
                    board.SQUARES_BETWEEN_BB[sq][destKing] &&
                (board.SQUARES_BETWEEN_BB[sq][56 + side] & ~board.occAll) == board.SQUARES_BETWEEN_BB[sq][56 + side] &&
                !((1ull << (56 + side)) & board.pinHV & MASK_RANK[RANK_8]) && !((1ull << destRook) & withoutRook) &&
                !((1ull << destKing) & (board.seen | (withoutRook & ~(1ull << sq)))))
                moves |= (1ULL << (56 + side));
        }
        break;
    }

    default:
        return moves;
    }
    return moves;
}

// all legal moves for a position
template <Color c, Movetype mt> void legalmoves(Board &board, Movelist &movelist)
{
    /********************
     * The size of the movelist might not
     * be 0! This is done on purpose since it enables
     * you to append new move types to any movelist.
     *******************/

    init<c>(board, board.KingSQ(c));

    assert(board.doubleCheck <= 2);

    /********************
     * Moves have to be on the checkmask
     *******************/
    U64 movableSquare = board.checkMask;

    /********************
     * Slider, Knights and King moves can only go to enemy or empty squares.
     *******************/
    if (mt == Movetype::ALL)
        movableSquare &= board.enemyEmptyBB;
    else if (mt == Movetype::CAPTURE)
        movableSquare &= board.occEnemy;
    else // QUIET moves
        movableSquare &= ~board.occAll;

    Square from = board.KingSQ(c);
    U64 moves;

    if (board.chess960)
    {
        if (mt == Movetype::CAPTURE || board.checkMask != DEFAULT_CHECKMASK)
            moves = LegalKingMoves<mt>(board, from);
        else
            moves = LegalKingMovesCastling960<c, mt>(board, from);
    }
    else
    {
        if (mt == Movetype::CAPTURE || !board.castlingRights || board.checkMask != DEFAULT_CHECKMASK)
            moves = LegalKingMoves<mt>(board, from);
        else
            moves = LegalKingMovesCastling<c, mt>(board, from);
    }

    while (moves)
    {
        Square to = poplsb(moves);
        movelist.Add(make<KING, false>(from, to));
    }

    /********************
     * Early return for double check as described earlier
     *******************/
    if (board.doubleCheck == 2)
        return;

    /********************
     * Prune knights that are pinned since these cannot move.
     *******************/
    U64 knights_mask = board.pieces<KNIGHT, c>() & ~(board.pinD | board.pinHV);

    /********************
     * Prune horizontally pinned bishops
     *******************/
    U64 bishops_mask = board.pieces<BISHOP, c>() & ~board.pinHV;

    /********************
     * Prune diagonally pinned rooks
     *******************/
    U64 rooks_mask = board.pieces<ROOK, c>() & ~board.pinD;

    /********************
     * Prune double pinned queens
     *******************/
    U64 queens_mask = board.pieces<QUEEN, c>() & ~(board.pinD & board.pinHV);

    /********************
     * Add the moves to the movelist.
     *******************/
    LegalPawnMovesAll<c, mt>(board, movelist);

    while (knights_mask)
    {
        Square from = poplsb(knights_mask);
        U64 moves = LegalKnightMoves(from, movableSquare);
        while (moves)
        {
            Square to = poplsb(moves);
            movelist.Add(make<KNIGHT, false>(from, to));
        }
    }

    while (bishops_mask)
    {
        Square from = poplsb(bishops_mask);
        U64 moves = LegalBishopMoves(board, from, movableSquare);
        while (moves)
        {
            Square to = poplsb(moves);
            movelist.Add(make<BISHOP, false>(from, to));
        }
    }

    while (rooks_mask)
    {
        Square from = poplsb(rooks_mask);
        U64 moves = LegalRookMoves(board, from, movableSquare);
        while (moves)
        {
            Square to = poplsb(moves);
            movelist.Add(make<ROOK, false>(from, to));
        }
    }

    while (queens_mask)
    {
        Square from = poplsb(queens_mask);
        U64 moves = LegalQueenMoves(board, from, movableSquare);
        while (moves)
        {
            Square to = poplsb(moves);
            movelist.Add(make<QUEEN, false>(from, to));
        }
    }
}

/********************
 * Entry function for the
 * Color template.
 *******************/
template <Movetype mt> void legalmoves(Board &board, Movelist &movelist)
{
    if (board.sideToMove == White)
        legalmoves<White, mt>(board, movelist);
    else
        legalmoves<Black, mt>(board, movelist);
}

// basically a mirror of legalmoves but with early returns
template <Color c> bool hasLegalMoves(Board &board)
{
    Movelist movelist;

    init<c>(board, board.KingSQ(c));

    assert(board.doubleCheck <= 2);

    U64 movableSquare = board.checkMask & board.enemyEmptyBB;

    Square from = board.KingSQ(c);
    U64 moves;

    if (board.chess960)
    {
        if (board.checkMask != DEFAULT_CHECKMASK)
            moves = LegalKingMoves<Movetype::ALL>(board, from);
        else
            moves = LegalKingMovesCastling960<c, Movetype::ALL>(board, from);
    }
    else
    {
        if (!board.castlingRights || board.checkMask != DEFAULT_CHECKMASK)
            moves = LegalKingMoves<Movetype::ALL>(board, from);
        else
            moves = LegalKingMovesCastling<c, Movetype::ALL>(board, from);
    }

    if (moves)
        return true;

    if (board.doubleCheck == 2)
        return false;

    U64 knights_mask = board.pieces<KNIGHT, c>() & ~(board.pinD | board.pinHV);
    U64 bishops_mask = board.pieces<BISHOP, c>() & ~board.pinHV;
    U64 rooks_mask = board.pieces<ROOK, c>() & ~board.pinD;
    U64 queens_mask = board.pieces<QUEEN, c>() & ~(board.pinD & board.pinHV);

    LegalPawnMovesAll<c, Movetype::ALL>(board, movelist);

    while (knights_mask)
    {
        Square from = poplsb(knights_mask);
        U64 moves = LegalKnightMoves(from, movableSquare);
        if (moves)
            return true;
    }

    while (bishops_mask)
    {
        Square from = poplsb(bishops_mask);
        U64 moves = LegalBishopMoves(board, from, movableSquare);
        if (moves)
            return true;
    }

    while (rooks_mask)
    {
        Square from = poplsb(rooks_mask);
        U64 moves = LegalRookMoves(board, from, movableSquare);
        if (moves)
            return true;
    }

    while (queens_mask)
    {
        Square from = poplsb(queens_mask);
        U64 moves = LegalQueenMoves(board, from, movableSquare);
        if (moves)
            return true;
    }

    return false;
}

bool hasLegalMoves(Board &board);

// pseudo legal moves number estimation
template <Color c> int pseudoLegalMovesNumber(const Board &board)
{
    int total = 0;

    U64 knights_mask = board.pieces<KNIGHT, c>();
    U64 bishops_mask = board.pieces<BISHOP, c>();
    U64 rooks_mask = board.pieces<ROOK, c>();
    U64 queens_mask = board.pieces<QUEEN, c>();

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
    Square from = board.KingSQ(c);
    total += popcount(KingAttacks(from) & board.enemyEmptyBB);
    return total;
}
} // namespace Movegen

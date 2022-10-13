#pragma once

#include "board.h"
#include "helper.h"
#include "types.h"

PACK(struct ExtMove {
    int value = -10 * VALUE_INFINITE;
    Move move;
});

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

namespace Movegen
{

// creates the checkmask
template <Color c> U64 DoCheckmask(Board &board, Square sq)
{
    U64 Occ = board.occAll;
    U64 checks = 0ULL;
    U64 pawn_mask = board.Pawns<~c>() & PawnAttacks(sq, c);
    U64 knight_mask = board.Knights<~c>() & KnightAttacks(sq);
    U64 bishop_mask = (board.Bishops<~c>() | board.Queens<~c>()) & BishopAttacks(sq, Occ);
    U64 rook_mask = (board.Rooks<~c>() | board.Queens<~c>()) & RookAttacks(sq, Occ);
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
        if (popcount(bishop_mask) > 1)
            board.doubleCheck++;

        int8_t index = bsf(bishop_mask);
        checks |= board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        board.doubleCheck++;
    }
    if (rook_mask)
    {
        if (popcount(rook_mask) > 1)
            board.doubleCheck++;

        int8_t index = bsf(rook_mask);
        checks |= board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        board.doubleCheck++;
    }
    return checks;
}

// creates the pinmask
template <Color c> void DoPinMask(Board &board, Square sq)
{
    U64 them = board.occEnemy;
    U64 rook_mask = (board.Rooks<~c>() | board.Queens<~c>()) & RookAttacks(sq, them);
    U64 bishop_mask = (board.Bishops<~c>() | board.Queens<~c>()) & BishopAttacks(sq, them);

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

// seen squares
template <Color c> void seenSquares(Board &board)
{
    U64 pawns = board.Pawns<c>();
    U64 knights = board.Knights<c>();
    U64 bishops = board.Bishops<c>();
    U64 rooks = board.Rooks<c>();
    U64 queens = board.Queens<c>();
    U64 kings = board.Kings<c>();

    board.seen = 0ULL;
    Square kSq = board.KingSQ<~c>();
    board.occAll &= ~(1ULL << kSq);

    if (c == White)
    {
        board.seen |= pawns << 9 & ~MASK_FILE[0];
        board.seen |= pawns << 7 & ~MASK_FILE[7];
    }
    else
    {
        board.seen |= pawns >> 7 & ~MASK_FILE[0];
        board.seen |= pawns >> 9 & ~MASK_FILE[7];
    }
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
    while (queens)
    {
        Square index = poplsb(queens);
        board.seen |= QueenAttacks(index, board.occAll);
    }
    while (kings)
    {
        Square index = poplsb(kings);
        board.seen |= KingAttacks(index);
    }

    board.occAll |= (1ULL << kSq);
}

// creates the pinmask and checkmask
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

template <Color c> U64 pawnLeftAttacks(const U64 pawns)
{
    return c == White ? (pawns << 7) & ~MASK_FILE[7] : (pawns >> 7) & ~MASK_FILE[0];
}

template <Color c> U64 pawnRightAttacks(const U64 pawns)
{
    return c == White ? (pawns << 9) & ~MASK_FILE[0] : (pawns >> 9) & ~MASK_FILE[7];
}

// returns a pawn push (only 1 square)
template <Color c> U64 PawnPushSingle(U64 occAll, Square sq)
{
    if (c == White)
    {
        return ((1ULL << sq) << 8) & ~occAll;
    }
    else
    {
        return ((1ULL << sq) >> 8) & ~occAll;
    }
}

template <Color c> U64 PawnPushBoth(U64 occAll, Square sq)
{
    U64 push = (1ULL << sq);
    if (c == White)
    {
        push = (push << 8) & ~occAll;
        return square_rank(sq) == 1 ? push | ((push << 8) & ~occAll) : push;
    }
    else
    {
        push = (push >> 8) & ~occAll;
        return square_rank(sq) == 6 ? push | ((push >> 8) & ~occAll) : push;
    }
}

template <Color c> U64 LegalPawnMovesSingle(const Board &board, Square sq)
{
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the board.checkMask
    if (board.pinD & (1ULL << sq))
        return PawnAttacks(sq, c) & board.pinD & board.checkMask & board.occEnemy;

    // If we are pinned horizontally we can do no moves but if we are pinned vertically we can only do pawn pushs
    if (board.pinHV & (1ULL << sq))
        return PawnPushBoth<c>(board.occAll, sq) & board.pinHV & board.checkMask;
    return ((PawnAttacks(sq, c) & board.occEnemy) | PawnPushBoth<c>(board.occAll, sq)) & board.checkMask;
}

template <Color c> U64 LegalPawnMovesEPSingle(Board &board, Square sq, Square ep)
{
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the board.checkMask
    if (board.pinD & (1ULL << sq))
        return PawnAttacks(sq, c) & board.pinD & board.checkMask & (board.occEnemy | (1ULL << ep));

    // If we are pinned horizontally we can do no moves but if we are pinned vertically we can only do pawn pushs
    if (board.pinHV & (1ULL << sq))
        return PawnPushBoth<c>(board.occAll, sq) & board.pinHV & board.checkMask;
    U64 attacks = PawnAttacks(sq, c);
    if (board.checkMask != DEFAULT_CHECKMASK)
    {
        // If we are in check and the en passant square lies on our attackmask and the en passant piece gives check
        // return the ep mask as a move square
        if (attacks & (1ULL << ep) && board.checkMask & (1ULL << (ep - (c * -2 + 1) * 8)))
            return 1ULL << ep;
        // If we are in check we can do all moves that are on the board.checkMask
        return ((attacks & board.occEnemy) | PawnPushBoth<c>(board.occAll, sq)) & board.checkMask;
    }

    U64 moves = ((attacks & board.occEnemy) | PawnPushBoth<c>(board.occAll, sq)) & board.checkMask;
    // We need to make extra sure that ep moves dont leave the king in check
    // 7k/8/8/K1Pp3r/8/8/8/8 w - d6 0 1
    // Horizontal rook pins our pawn through another pawn, our pawn can push but not take enpassant
    // remove both the pawn that made the push and our pawn that could take in theory and check if that would give check
    if ((1ULL << ep) & attacks)
    {
        Square tP = c == White ? Square(static_cast<int>(ep) - 8) : Square(static_cast<int>(ep) + 8);
        Square kSQ = board.KingSQ<c>();
        U64 enemyQueenRook = board.Rooks(~c) | board.Queens(~c);
        if ((enemyQueenRook)&MASK_RANK[square_rank(kSQ)])
        {
            Piece ourPawn = makePiece(PAWN, c);
            Piece theirPawn = makePiece(PAWN, ~c);
            board.removePiece<false>(ourPawn, sq);
            board.removePiece<false>(theirPawn, tP);
            board.placePiece<false>(ourPawn, ep);
            if (!((RookAttacks(kSQ, board.All()) & (enemyQueenRook))))
                moves |= (1ULL << ep);
            board.placePiece<false>(ourPawn, sq);
            board.placePiece<false>(theirPawn, tP);
            board.removePiece<false>(ourPawn, ep);
        }
        else
        {
            moves |= (1ULL << ep);
        }
    }
    return moves;
}

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

template <Color c, Movetype mt> void LegalPawnMovesAll(Board &board, Movelist &movelist)
{
    const U64 pawns_mask = board.Pawns<c>();

    constexpr Direction UP = c == White ? NORTH : SOUTH;
    constexpr Direction DOWN = c == Black ? NORTH : SOUTH;
    constexpr Direction DOWN_LEFT = c == Black ? NORTH_EAST : SOUTH_WEST;
    constexpr Direction DOWN_RIGHT = c == Black ? NORTH_WEST : SOUTH_EAST;
    constexpr U64 RANK_BEFORE_PROMO = c == White ? MASK_RANK[6] : MASK_RANK[1];
    constexpr U64 RANK_PROMO = c == White ? MASK_RANK[7] : MASK_RANK[0];
    constexpr U64 doublePushRank = c == White ? MASK_RANK[2] : MASK_RANK[5];

    // These pawns can walk L or R
    const uint64_t pawnsLR = pawns_mask & ~board.pinHV;

    const uint64_t unpinnedpawnsLR = pawnsLR & ~board.pinD;
    const uint64_t pinnedpawnsLR = pawnsLR & board.pinD;

    uint64_t Lpawns = (pawnLeftAttacks<c>(unpinnedpawnsLR)) | (pawnLeftAttacks<c>(pinnedpawnsLR) & board.pinD);
    uint64_t Rpawns = (pawnRightAttacks<c>(unpinnedpawnsLR)) | (pawnRightAttacks<c>(pinnedpawnsLR) & board.pinD);

    Lpawns &= board.occEnemy & board.checkMask;
    Rpawns &= board.occEnemy & board.checkMask;

    // These pawns can walk Forward
    const uint64_t pawnsHV = pawns_mask & ~board.pinD;

    const uint64_t pawnsPinnedHV = pawnsHV & board.pinHV;
    const uint64_t pawnsUnPinnedHV = pawnsHV & ~board.pinHV;

    const uint64_t singlePushUnpinned = shift<UP>(pawnsUnPinnedHV) & ~board.occAll;
    const uint64_t singlePushPinned = shift<UP>(pawnsPinnedHV) & board.pinHV & ~board.occAll;

    uint64_t singlePush = (singlePushUnpinned | singlePushPinned) & board.checkMask;

    uint64_t doublePush = ((shift<UP>(singlePushUnpinned & doublePushRank) & ~board.occAll) |
                           (shift<UP>(singlePushPinned & doublePushRank) & ~board.occAll)) &
                          board.checkMask;

    if (pawns_mask & RANK_BEFORE_PROMO)
    {
        uint64_t Promote_Left = Lpawns & RANK_PROMO;
        uint64_t Promote_Right = Rpawns & RANK_PROMO;
        uint64_t Promote_Move = singlePush & RANK_PROMO;

        while (Promote_Move && (mt == ALL || mt == CAPTURE))
        {
            const Square to = poplsb(Promote_Move);
            movelist.Add(make<QUEEN, true>(Square(to + DOWN), to));
            movelist.Add(make<ROOK, true>(Square(to + DOWN), to));
            movelist.Add(make<KNIGHT, true>(Square(to + DOWN), to));
            movelist.Add(make<BISHOP, true>(Square(to + DOWN), to));
        }

        while (Promote_Right && (mt == ALL || mt == CAPTURE))
        {
            const Square to = poplsb(Promote_Right);
            movelist.Add(make<QUEEN, true>(Square(to + DOWN_LEFT), to));
            movelist.Add(make<ROOK, true>(Square(to + DOWN_LEFT), to));
            movelist.Add(make<KNIGHT, true>(Square(to + DOWN_LEFT), to));
            movelist.Add(make<BISHOP, true>(Square(to + DOWN_LEFT), to));
        }

        while (Promote_Left && (mt == ALL || mt == CAPTURE))
        {
            const Square to = poplsb(Promote_Left);
            movelist.Add(make<QUEEN, true>(Square(to + DOWN_RIGHT), to));
            movelist.Add(make<ROOK, true>(Square(to + DOWN_RIGHT), to));
            movelist.Add(make<KNIGHT, true>(Square(to + DOWN_RIGHT), to));
            movelist.Add(make<BISHOP, true>(Square(to + DOWN_RIGHT), to));
        }

        singlePush &= ~RANK_PROMO;
        Lpawns &= ~RANK_PROMO;
        Rpawns &= ~RANK_PROMO;
    }

    if (board.enPassantSquare != NO_SQ && (mt == ALL || mt == CAPTURE))
    {
        const Square ep = board.enPassantSquare;
        const U64 epBB = (1ULL << ep);
        U64 left = pawnLeftAttacks<c>(pawnsLR) & epBB;
        U64 right = pawnRightAttacks<c>(pawnsLR) & epBB;

        Square to;
        Square from;

        while (left || right)
        {
            if (left)
            {
                to = poplsb(left);
                from = Square(to + DOWN_RIGHT);
            }
            else
            {
                to = poplsb(right);
                from = Square(to + DOWN_LEFT);
            }

            // if pinned but ep square is not on the pinmask then we cant move there
            if ((1ULL << from) & board.pinD && !(board.pinD & epBB))
                continue;

            // If we are in check and the en passant square lies on our attackmask and the en passant piece gives
            // check return the ep mask as a move square
            if (board.checkMask != DEFAULT_CHECKMASK && board.checkMask & (1ULL << (ep - (c * -2 + 1) * 8)))
            {
                movelist.Add(make<PAWN, false>(from, to));
                continue;
            }

            const Square tP = c == White ? Square(static_cast<int>(ep) - 8) : Square(static_cast<int>(ep) + 8);
            const Square kSQ = board.KingSQ<c>();
            const U64 enemyQueenRook = board.Rooks(~c) | board.Queens(~c);
            if (enemyQueenRook & MASK_RANK[square_rank(kSQ)])
            {
                Piece ourPawn = makePiece(PAWN, c);
                Piece theirPawn = makePiece(PAWN, ~c);
                board.removePiece<false>(ourPawn, from);
                board.removePiece<false>(theirPawn, tP);
                board.placePiece<false>(ourPawn, ep);
                if (!((RookAttacks(kSQ, board.All()) & (enemyQueenRook))))
                    movelist.Add(make<PAWN, false>(from, to));
                board.placePiece<false>(ourPawn, from);
                board.placePiece<false>(theirPawn, tP);
                board.removePiece<false>(ourPawn, ep);
            }
            else
                movelist.Add(make<PAWN, false>(from, to));
        }
    }

    while (singlePush && (mt == ALL || mt == QUIET))
    {
        Square to = poplsb(singlePush);
        movelist.Add(make<PAWN, false>(Square(to + DOWN), to));
    }

    while (doublePush && (mt == ALL || mt == QUIET))
    {
        Square to = poplsb(doublePush);
        movelist.Add(make<PAWN, false>(Square(to + DOWN + DOWN), to));
    }

    while (Rpawns && (mt == ALL || mt == CAPTURE))
    {
        Square to = poplsb(Rpawns);
        movelist.Add(make<PAWN, false>(Square(to + DOWN_LEFT), to));
    }

    while (Lpawns && (mt == ALL || mt == CAPTURE))
    {
        Square to = poplsb(Lpawns);
        movelist.Add(make<PAWN, false>(Square(to + DOWN_RIGHT), to));
    }
}

template <Movetype mt> U64 LegalKnightMoves(const Board &board, Square sq)
{
    U64 bb;

    if (mt == ALL)
        bb = board.enemyEmptyBB;
    else if (mt == CAPTURE)
        bb = board.occEnemy;
    else
        bb = ~board.occAll;

    if (board.pinD & (1ULL << sq) || board.pinHV & (1ULL << sq))
        return 0ULL;
    return KnightAttacks(sq) & bb & board.checkMask;
}

template <Movetype mt> U64 LegalBishopMoves(const Board &board, Square sq)
{
    U64 bb;

    if (mt == ALL)
        bb = board.enemyEmptyBB;
    else if (mt == CAPTURE)
        bb = board.occEnemy;
    else
        bb = ~board.occAll;

    if (board.pinHV & (1ULL << sq))
        return 0ULL;
    if (board.pinD & (1ULL << sq))
        return BishopAttacks(sq, board.occAll) & bb & board.pinD & board.checkMask;
    return BishopAttacks(sq, board.occAll) & bb & board.checkMask;
}

template <Movetype mt> U64 LegalRookMoves(const Board &board, Square sq)
{
    U64 bb;

    if (mt == ALL)
        bb = board.enemyEmptyBB;
    else if (mt == CAPTURE)
        bb = board.occEnemy;
    else
        bb = ~board.occAll;

    if (board.pinD & (1ULL << sq))
        return 0ULL;
    if (board.pinHV & (1ULL << sq))
        return RookAttacks(sq, board.occAll) & bb & board.pinHV & board.checkMask;
    return RookAttacks(sq, board.occAll) & bb & board.checkMask;
}

template <Movetype mt> U64 LegalQueenMoves(const Board &board, Square sq)
{
    return LegalRookMoves<mt>(board, sq) | LegalBishopMoves<mt>(board, sq);
}

template <Movetype mt> U64 LegalKingMoves(const Board &board, Square sq)
{
    U64 bb;

    if (mt == ALL)
        bb = board.enemyEmptyBB;
    else if (mt == CAPTURE)
        bb = board.occEnemy;
    else
        bb = ~board.occAll;

    return KingAttacks(sq) & bb & ~board.seen;
}

template <Color c> U64 LegalKingMovesCastling(const Board &board, Square sq)
{
    U64 moves = KingAttacks(sq) & board.enemyEmptyBB & ~board.seen;

    if (c == White)
    {
        if (board.castlingRights & wk && !(WK_CASTLE_MASK & board.occAll) && moves & (1ULL << SQ_F1) &&
            !(board.seen & (1ULL << SQ_G1)))
        {
            moves |= (1ULL << SQ_G1);
        }
        if (board.castlingRights & wq && !(WQ_CASTLE_MASK & board.occAll) && moves & (1ULL << SQ_D1) &&
            !(board.seen & (1ULL << SQ_C1)))
        {
            moves |= (1ULL << SQ_C1);
        }
    }
    else
    {
        if (board.castlingRights & bk && !(BK_CASTLE_MASK & board.occAll) && moves & (1ULL << SQ_F8) &&
            !(board.seen & (1ULL << SQ_G8)))
        {
            moves |= (1ULL << SQ_G8);
        }
        if (board.castlingRights & bq && !(BQ_CASTLE_MASK & board.occAll) && moves & (1ULL << SQ_D8) &&
            !(board.seen & (1ULL << SQ_C8)))
        {
            moves |= (1ULL << SQ_C8);
        }
    }
    return moves;
}

// all legal moves for a position
template <Color c, Movetype mt> void legalmoves(Board &board, Movelist &movelist)
{
    init<c>(board, board.KingSQ<c>());

    Square from = board.KingSQ<c>();
    U64 moves;
    if (!board.castlingRights || board.checkMask != DEFAULT_CHECKMASK || mt == CAPTURE)
        moves = LegalKingMoves<mt>(board, from);
    else
        moves = LegalKingMovesCastling<c>(board, from);

    while (moves)
    {
        Square to = poplsb(moves);
        movelist.Add(make<KING, false>(from, to));
    }

    if (board.doubleCheck == 2)
        return;

    U64 knights_mask = board.Knights<c>();
    U64 bishops_mask = board.Bishops<c>();
    U64 rooks_mask = board.Rooks<c>();
    U64 queens_mask = board.Queens<c>();

    LegalPawnMovesAll<c, mt>(board, movelist);

    while (knights_mask)
    {
        Square from = poplsb(knights_mask);
        U64 moves = LegalKnightMoves<mt>(board, from);
        while (moves)
        {
            Square to = poplsb(moves);
            movelist.Add(make<KNIGHT, false>(from, to));
        }
    }
    while (bishops_mask)
    {
        Square from = poplsb(bishops_mask);
        U64 moves = LegalBishopMoves<mt>(board, from);
        while (moves)
        {
            Square to = poplsb(moves);
            movelist.Add(make<BISHOP, false>(from, to));
        }
    }
    while (rooks_mask)
    {
        Square from = poplsb(rooks_mask);
        U64 moves = LegalRookMoves<mt>(board, from);
        while (moves)
        {
            Square to = poplsb(moves);
            movelist.Add(make<ROOK, false>(from, to));
        }
    }
    while (queens_mask)
    {
        Square from = poplsb(queens_mask);
        U64 moves = LegalQueenMoves<mt>(board, from);
        while (moves)
        {
            Square to = poplsb(moves);
            movelist.Add(make<QUEEN, false>(from, to));
        }
    }
}

template <Movetype mt> void legalmoves(Board &board, Movelist &movelist)
{
    if (board.sideToMove == White)
        legalmoves<White, mt>(board, movelist);
    else
        legalmoves<Black, mt>(board, movelist);
}

template <Color c> bool hasLegalMoves(Board &board);
bool hasLegalMoves(Board &board);

// pseudo legal moves number estimation
template <Color c> int pseudoLegalMovesNumber(Board &board);
} // namespace Movegen

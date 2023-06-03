#pragma once

#include <iterator>

#include "board.h"
#include "helper.h"
#include "types.h"

struct ExtMove {
    int value = 0;
    Move move = NO_MOVE;
};

inline constexpr bool operator==(const ExtMove &a, const ExtMove &b) { return a.move == b.move; }

inline constexpr bool operator>(const ExtMove &a, const ExtMove &b) { return a.value > b.value; }

inline constexpr bool operator<(const ExtMove &a, const ExtMove &b) { return a.value < b.value; }

struct Movelist {
    ExtMove list[MAX_MOVES] = {};
    uint8_t size = 0;
    typedef ExtMove *iterator;
    typedef const ExtMove *const_iterator;

    void Add(Move move) {
        list[size].move = move;
        list[size].value = 0;
        size++;
    }

    inline constexpr ExtMove &operator[](int i) { return list[i]; }

    inline constexpr int find(Move m) {
        for (int i = 0; i < size; i++) {
            if (list[i].move == m) return i;
        }
        return -1;
    }

    iterator begin() { return (std::begin(list)); }
    const_iterator begin() const { return (std::begin(list)); }
    iterator end() {
        auto it = std::begin(list);
        std::advance(it, size);
        return it;
    }
    const_iterator end() const {
        auto it = std::begin(list);
        std::advance(it, size);
        return it;
    }
};

namespace Movegen {

template <Color c>
U64 pawnLeftAttacks(const U64 pawns) {
    return c == White ? (pawns << 7) & ~MASK_FILE[FILE_H] : (pawns >> 7) & ~MASK_FILE[FILE_A];
}

template <Color c>
U64 pawnRightAttacks(const U64 pawns) {
    return c == White ? (pawns << 9) & ~MASK_FILE[FILE_A] : (pawns >> 9) & ~MASK_FILE[FILE_H];
}

/********************
 * Creates the checkmask.
 * A checkmask is the path from the enemy checker to our king.
 * Knight and pawns get themselves added to the checkmask, otherwise the path is added.
 * When there is no check at all all bits are set (DEFAULT_CHECKMASK)
 *******************/
template <Color c>
U64 checkMask(const Board &board, Square sq, U64 occAll, int &doubleCheck) {
    U64 checks = 0ULL;
    U64 pawn_mask = board.pieces<PAWN, ~c>() & PawnAttacks(sq, c);
    U64 knight_mask = board.pieces<KNIGHT, ~c>() & KnightAttacks(sq);
    U64 bishop_mask =
        (board.pieces<BISHOP, ~c>() | board.pieces<QUEEN, ~c>()) & BishopAttacks(sq, occAll);
    U64 rook_mask =
        (board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>()) & RookAttacks(sq, occAll);

    /********************
     * We keep track of the amount of checks, in case there are
     * two checks on the board only the king can move!
     *******************/
    doubleCheck = 0;

    if (pawn_mask) {
        checks |= pawn_mask;
        doubleCheck++;
    }
    if (knight_mask) {
        checks |= knight_mask;
        doubleCheck++;
    }
    if (bishop_mask) {
        int8_t index = lsb(bishop_mask);

        // Now we add the path!
        checks |= board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        doubleCheck++;
    }
    if (rook_mask) {
        /********************
         * 3nk3/4P3/8/8/8/8/8/2K1R3 w - - 0 1, pawn promotes to queen or rook and
         * suddenly the same piecetype gives check two times
         * in that case we have a double check and can return early
         * because king moves dont require the checkmask.
         *******************/
        if (popcount(rook_mask) > 1) {
            doubleCheck = 2;
            return checks;
        }

        int8_t index = lsb(rook_mask);

        // Now we add the path!
        checks |= board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        doubleCheck++;
    }

    if (!checks) return DEFAULT_CHECKMASK;

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
template <Color c>
U64 pinMaskRooks(const Board &board, Square sq, U64 occUs, U64 occEnemy) {
    U64 rook_mask =
        (board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>()) & RookAttacks(sq, occEnemy);

    U64 pinHV = 0ULL;
    while (rook_mask) {
        const Square index = poplsb(rook_mask);
        const U64 possible_pin = (board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & occUs) == 1) pinHV |= possible_pin;
    }
    return pinHV;
}

template <Color c>
U64 pinMaskBishops(const Board &board, Square sq, U64 occUs, U64 occEnemy) {
    U64 bishop_mask =
        (board.pieces<BISHOP, ~c>() | board.pieces<QUEEN, ~c>()) & BishopAttacks(sq, occEnemy);

    U64 pinD = 0ULL;

    while (bishop_mask) {
        const Square index = poplsb(bishop_mask);
        const U64 possible_pin = (board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & occUs) == 1) pinD |= possible_pin;
    }

    return pinD;
}

/********************
 * Seen squares
 * We keep track of all attacked squares by the enemy
 * this is used for king move generation.
 *******************/
template <Color c>
U64 seenSquares(const Board &board, U64 occAll) {
    const Square kSq = board.KingSQ(~c);

    U64 pawns = board.pieces<PAWN, c>();
    U64 knights = board.pieces<KNIGHT, c>();
    U64 queens = board.pieces<QUEEN, c>();
    U64 bishops = board.pieces<BISHOP, c>() | queens;
    U64 rooks = board.pieces<ROOK, c>() | queens;

    // Remove our king
    occAll &= ~(1ULL << kSq);

    U64 seen = pawnLeftAttacks<c>(pawns) | pawnRightAttacks<c>(pawns);

    while (knights) {
        Square index = poplsb(knights);
        seen |= KnightAttacks(index);
    }
    while (bishops) {
        Square index = poplsb(bishops);
        seen |= BishopAttacks(index, occAll);
    }
    while (rooks) {
        Square index = poplsb(rooks);
        seen |= RookAttacks(index, occAll);
    }

    Square index = lsb(board.pieces<KING, c>());
    seen |= KingAttacks(index);

    return seen;
}

/// @brief shift a mask in a direction
/// @tparam direction
/// @param b
/// @return
template <Direction direction>
constexpr U64 shift(const U64 b) {
    switch (direction) {
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
template <Color c, Movetype mt>
void LegalPawnMovesAll(const Board &board, Movelist &movelist, U64 occAll, U64 occEnemy,
                       U64 checkMask, U64 pinHV, U64 pinD) {
    const U64 pawns_mask = board.pieces<PAWN, c>();

    constexpr Direction UP = c == White ? NORTH : SOUTH;
    constexpr Direction DOWN = c == Black ? NORTH : SOUTH;
    constexpr Direction DOWN_LEFT = c == Black ? NORTH_EAST : SOUTH_WEST;
    constexpr Direction DOWN_RIGHT = c == Black ? NORTH_WEST : SOUTH_EAST;
    constexpr U64 RANK_BEFORE_PROMO = c == White ? MASK_RANK[RANK_7] : MASK_RANK[RANK_2];
    constexpr U64 RANK_PROMO = c == White ? MASK_RANK[RANK_8] : MASK_RANK[RANK_1];
    constexpr U64 doublePushRank = c == White ? MASK_RANK[RANK_3] : MASK_RANK[RANK_6];

    // These pawns can maybe take Left or Right
    const U64 pawnsLR = pawns_mask & ~pinHV;

    const U64 unpinnedpawnsLR = pawnsLR & ~pinD;
    const U64 pinnedpawnsLR = pawnsLR & pinD;

    U64 Lpawns = (pawnLeftAttacks<c>(unpinnedpawnsLR)) | (pawnLeftAttacks<c>(pinnedpawnsLR) & pinD);
    U64 Rpawns =
        (pawnRightAttacks<c>(unpinnedpawnsLR)) | (pawnRightAttacks<c>(pinnedpawnsLR) & pinD);

    // Prune moves that dont capture a piece and are not on the checkmask.
    Lpawns &= occEnemy & checkMask;
    Rpawns &= occEnemy & checkMask;

    // These pawns can walk Forward
    const U64 pawnsHV = pawns_mask & ~pinD;

    const U64 pawnsPinnedHV = pawnsHV & pinHV;
    const U64 pawnsUnPinnedHV = pawnsHV & ~pinHV;

    // Prune moves that are blocked by a piece
    const U64 singlePushUnpinned = shift<UP>(pawnsUnPinnedHV) & ~occAll;
    const U64 singlePushPinned = shift<UP>(pawnsPinnedHV) & pinHV & ~occAll;

    // Prune moves that are not on the checkmask.
    U64 singlePush = (singlePushUnpinned | singlePushPinned) & checkMask;

    U64 doublePush = ((shift<UP>(singlePushUnpinned & doublePushRank) & ~occAll) |
                      (shift<UP>(singlePushPinned & doublePushRank) & ~occAll)) &
                     checkMask;

    /********************
     * Add promotion moves.
     * These are always generated unless we only want quiet moves.
     *******************/
    if ((mt != Movetype::QUIET) && pawns_mask & RANK_BEFORE_PROMO) {
        U64 Promote_Left = Lpawns & RANK_PROMO;
        U64 Promote_Right = Rpawns & RANK_PROMO;
        U64 Promote_Move = singlePush & RANK_PROMO;

        while (Promote_Move) {
            Square to = poplsb(Promote_Move);
            movelist.Add(make<PROMOTION>(to + DOWN, to, QUEEN));
            movelist.Add(make<PROMOTION>(to + DOWN, to, ROOK));
            movelist.Add(make<PROMOTION>(to + DOWN, to, KNIGHT));
            movelist.Add(make<PROMOTION>(to + DOWN, to, BISHOP));
        }

        while (Promote_Right) {
            Square to = poplsb(Promote_Right);
            movelist.Add(make<PROMOTION>(to + DOWN_LEFT, to, QUEEN));
            movelist.Add(make<PROMOTION>(to + DOWN_LEFT, to, ROOK));
            movelist.Add(make<PROMOTION>(to + DOWN_LEFT, to, KNIGHT));
            movelist.Add(make<PROMOTION>(to + DOWN_LEFT, to, BISHOP));
        }

        while (Promote_Left) {
            Square to = poplsb(Promote_Left);
            movelist.Add(make<PROMOTION>(to + DOWN_RIGHT, to, QUEEN));
            movelist.Add(make<PROMOTION>(to + DOWN_RIGHT, to, ROOK));
            movelist.Add(make<PROMOTION>(to + DOWN_RIGHT, to, KNIGHT));
            movelist.Add(make<PROMOTION>(to + DOWN_RIGHT, to, BISHOP));
        }
    }

    // Remove the promotion pawns
    singlePush &= ~RANK_PROMO;
    Lpawns &= ~RANK_PROMO;
    Rpawns &= ~RANK_PROMO;

    /********************
     * Add single pushs.
     *******************/
    while (mt != Movetype::CAPTURE && singlePush) {
        Square to = poplsb(singlePush);
        movelist.Add(make(to + DOWN, to));
    }

    /********************
     * Add double pushs.
     *******************/
    while (mt != Movetype::CAPTURE && doublePush) {
        Square to = poplsb(doublePush);
        movelist.Add(make(to + DOWN + DOWN, to));
    }

    /********************
     * Add right pawn captures.
     *******************/
    while (mt != Movetype::QUIET && Rpawns) {
        Square to = poplsb(Rpawns);
        movelist.Add(make(to + DOWN_LEFT, to));
    }

    /********************
     * Add left pawn captures.
     *******************/
    while (mt != Movetype::QUIET && Lpawns) {
        Square to = poplsb(Lpawns);
        movelist.Add(make(to + DOWN_RIGHT, to));
    }

    /********************
     * Add en passant captures.
     *******************/
    if (mt != Movetype::QUIET && board.enPassantSquare != NO_SQ) {
        const Square ep = board.enPassantSquare;
        const Square epPawn = ep + DOWN;

        U64 epMask = (1ull << epPawn) | (1ull << ep);

        /********************
         * In case the en passant square and the enemy pawn
         * that just moved are not on the checkmask
         * en passant is not available.
         *******************/
        if ((checkMask & epMask) == 0) return;

        const Square kSQ = board.KingSQ(c);
        const U64 kingMask = (1ull << kSQ) & MASK_RANK[square_rank(epPawn)];
        const U64 enemyQueenRook = board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>();

        const bool isPossiblePin = kingMask && enemyQueenRook;
        U64 epBB = PawnAttacks(ep, ~c) & pawnsLR;

        /********************
         * For one en passant square two pawns could potentially take there.
         *******************/
        while (epBB) {
            Square from = poplsb(epBB);
            Square to = ep;

            /********************
             * If the pawn is pinned but the en passant square is not on the
             * pin mask then the move is illegal.
             *******************/
            if ((1ULL << from) & pinD && !(pinD & (1ull << ep))) continue;

            const U64 connectingPawns = (1ull << epPawn) | (1ull << from);

            /********************
             * 7k/4p3/8/2KP3r/8/8/8/8 b - - 0 1
             * If e7e5 there will be a potential ep square for us on e6.
             * However we cannot take en passant because that would put our king
             * in check. For this scenario we check if theres an enemy rook/queen
             * that would give check if the two pawns were removed.
             * If thats the case then the move is illegal and we can break immediately.
             *******************/
            if (isPossiblePin &&
                (RookAttacks(kSQ, occAll & ~connectingPawns) & enemyQueenRook) != 0)
                break;

            movelist.Add(make<ENPASSANT>(from, to));
        }
    }
}

inline U64 LegalKnightMoves(Square sq, U64 movableSquare) {
    return KnightAttacks(sq) & movableSquare;
}

inline U64 LegalBishopMoves(Square sq, U64 movableSquare, U64 pinMask, U64 occAll) {
    // The Bishop is pinned diagonally thus can only move diagonally.
    if (pinMask & (1ULL << sq)) return BishopAttacks(sq, occAll) & movableSquare & pinMask;
    return BishopAttacks(sq, occAll) & movableSquare;
}

inline U64 LegalRookMoves(Square sq, U64 movableSquare, U64 pinMask, U64 occAll) {
    // The Rook is pinned horizontally thus can only move horizontally.
    if (pinMask & (1ULL << sq)) return RookAttacks(sq, occAll) & movableSquare & pinMask;
    return RookAttacks(sq, occAll) & movableSquare;
}

inline U64 LegalQueenMoves(Square sq, U64 movableSquare, U64 pinD, U64 pinHV, U64 occAll) {
    U64 moves = 0ULL;
    if (pinD & (1ULL << sq))
        moves |= BishopAttacks(sq, occAll) & movableSquare & pinD;
    else if (pinHV & (1ULL << sq))
        moves |= RookAttacks(sq, occAll) & movableSquare & pinHV;
    else {
        moves |= RookAttacks(sq, occAll) & movableSquare;
        moves |= BishopAttacks(sq, occAll) & movableSquare;
    }

    return moves;
}

template <Movetype mt>
U64 LegalKingMoves(Square sq, U64 movableSquare, U64 seen) {
    return KingAttacks(sq) & movableSquare & ~seen;
}

constexpr Square relativeSquare(Color c, Square s) {
    return Square(s ^ (static_cast<int>(c) * 56));
}

template <Color c, Movetype mt>
inline U64 legalCastleMoves(const Board &board, Square sq, U64 seen, U64 pinHV, U64 occAll) {
    if constexpr (mt == Movetype::CAPTURE) return 0ull;
    const auto rights = board.castlingRights;

    U64 moves = 0ull;

    for (const auto side : {CastleSide::KING_SIDE, CastleSide::QUEEN_SIDE}) {
        if (!rights.hasCastlingRight(c, side)) continue;

        const auto end_king_sq =
            relativeSquare(c, side == CastleSide::KING_SIDE ? Square::SQ_G1 : Square::SQ_C1);
        const auto end_rook_sq =
            relativeSquare(c, side == CastleSide::KING_SIDE ? Square::SQ_F1 : Square::SQ_D1);

        const auto from_rook_sq = file_rank_square(rights.getRookFile(c, side), square_rank(sq));

        const U64 not_occ_path = board.SQUARES_BETWEEN_BB[sq][from_rook_sq];
        const U64 not_attacked_path = board.SQUARES_BETWEEN_BB[sq][end_king_sq];
        const U64 empty_not_attacked = ~seen & ~(occAll & ~(1ull << from_rook_sq));
        const U64 withoutRook = occAll & ~(1ull << from_rook_sq);

        if ((not_attacked_path & empty_not_attacked) == not_attacked_path &&
            ((not_occ_path & ~occAll) == not_occ_path) &&
            !((1ull << from_rook_sq) & pinHV & MASK_RANK[static_cast<int>(square_rank(sq))]) &&
            !((1ull << end_rook_sq) & withoutRook) &&
            !((1ull << end_king_sq) & (seen | (withoutRook & ~(1ull << sq))))) {
            moves |= (1ull << from_rook_sq);
        }
    }

    return moves;
}

// all legal moves for a position
template <Color c, Movetype mt>
void legalmoves(Board &board, Movelist &movelist) {
    /********************
     * The size of the movelist might not
     * be 0! This is done on purpose since it enables
     * you to append new move types to any movelist.
     *******************/

    Square king_sq = board.KingSQ(c);

    int doubleCheck = 0;

    U64 occUs = board.Us<c>();
    U64 occEnemy = board.Us<~c>();
    U64 occAll = occUs | occEnemy;
    U64 enemyEmptyBB = ~occUs;

    U64 seen = seenSquares<~c>(board, occAll);
    U64 check_mask = checkMask<c>(board, king_sq, occAll, doubleCheck);
    U64 pinHV = pinMaskRooks<c>(board, king_sq, occUs, occEnemy);
    U64 pinD = pinMaskBishops<c>(board, king_sq, occUs, occEnemy);

    assert(doubleCheck <= 2);

    /********************
     * Moves have to be on the check_mask
     *******************/
    U64 movableSquare;

    /********************
     * Slider, Knights and King moves can only go to enemy or empty squares.
     *******************/
    if (mt == Movetype::ALL)
        movableSquare = enemyEmptyBB;
    else if (mt == Movetype::CAPTURE)
        movableSquare = occEnemy;
    else  // QUIET moves
        movableSquare = ~occAll;

    U64 moves = LegalKingMoves<mt>(king_sq, movableSquare, seen);

    movableSquare &= check_mask;

    while (moves) {
        Square to = poplsb(moves);
        movelist.Add(make(king_sq, to));
    }

    if (mt != Movetype::CAPTURE && square_rank(king_sq) == (c == White ? RANK_1 : RANK_8) &&
        board.castlingRights.hasCastlingRight(c) && check_mask == DEFAULT_CHECKMASK) {
        moves = legalCastleMoves<c, mt>(board, king_sq, seen, pinHV, occAll);

        while (moves) {
            Square to = poplsb(moves);
            movelist.Add(make<CASTLING>(king_sq, to));
        }
    }

    /********************
     * Early return for double check as described earlier
     *******************/
    if (doubleCheck == 2) return;

    /********************
     * Prune knights that are pinned since these cannot move.
     *******************/
    U64 knights_mask = board.pieces<KNIGHT, c>() & ~(pinD | pinHV);

    /********************
     * Prune horizontally pinned bishops
     *******************/
    U64 bishops_mask = board.pieces<BISHOP, c>() & ~pinHV;

    /********************
     * Prune diagonally pinned rooks
     *******************/
    U64 rooks_mask = board.pieces<ROOK, c>() & ~pinD;

    /********************
     * Prune double pinned queens
     *******************/
    U64 queens_mask = board.pieces<QUEEN, c>() & ~(pinD & pinHV);

    /********************
     * Add the moves to the movelist.
     *******************/
    LegalPawnMovesAll<c, mt>(board, movelist, occAll, occEnemy, check_mask, pinHV, pinD);

    while (knights_mask) {
        Square from = poplsb(knights_mask);
        U64 moves = LegalKnightMoves(from, movableSquare);
        while (moves) {
            Square to = poplsb(moves);
            movelist.Add(make(from, to));
        }
    }

    while (bishops_mask) {
        Square from = poplsb(bishops_mask);
        U64 moves = LegalBishopMoves(from, movableSquare, pinD, occAll);
        while (moves) {
            Square to = poplsb(moves);
            movelist.Add(make(from, to));
        }
    }

    while (rooks_mask) {
        Square from = poplsb(rooks_mask);
        U64 moves = LegalRookMoves(from, movableSquare, pinHV, occAll);
        while (moves) {
            Square to = poplsb(moves);
            movelist.Add(make(from, to));
        }
    }

    while (queens_mask) {
        Square from = poplsb(queens_mask);
        U64 moves = LegalQueenMoves(from, movableSquare, pinD, pinHV, occAll);
        while (moves) {
            Square to = poplsb(moves);
            movelist.Add(make(from, to));
        }
    }
}

/********************
 * Entry function for the
 * Color template.
 *******************/
template <Movetype mt>
void legalmoves(Board &board, Movelist &movelist) {
    if (board.sideToMove == White)
        legalmoves<White, mt>(board, movelist);
    else
        legalmoves<Black, mt>(board, movelist);
}

}  // namespace Movegen

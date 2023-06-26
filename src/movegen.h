#pragma once

#include <iterator>

#include "board.h"
#include "helper.h"
#include "types.h"

static auto init_squares_between = []() constexpr {
    // initialize squares between table
    std::array<std::array<U64, 64>, 64> squares_between_bb{};
    U64 sqs = 0;
    for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1) {
        for (Square sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2) {
            sqs = (1ULL << sq1) | (1ULL << sq2);
            if (sq1 == sq2)
                squares_between_bb[sq1][sq2] = 0ull;
            else if (square_file(sq1) == square_file(sq2) || square_rank(sq1) == square_rank(sq2))
                squares_between_bb[sq1][sq2] = attacks::Rook(sq1, sqs) & attacks::Rook(sq2, sqs);
            else if (diagonal_of(sq1) == diagonal_of(sq2) ||
                     anti_diagonal_of(sq1) == anti_diagonal_of(sq2))
                squares_between_bb[sq1][sq2] =
                    attacks::Bishop(sq1, sqs) & attacks::Bishop(sq2, sqs);
        }
    }
    return squares_between_bb;
};

static const std::array<std::array<U64, 64>, 64> SQUARES_BETWEEN_BB = init_squares_between();

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

    void add(Move move) {
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

namespace movegen {

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
U64 checkMask(const Board &board, Square sq, U64 occ_all, int &double_check) {
    U64 checks = 0ULL;
    U64 pawn_mask = board.pieces<PAWN, ~c>() & attacks::Pawn(sq, c);
    U64 knight_mask = board.pieces<KNIGHT, ~c>() & attacks::Knight(sq);
    U64 bishop_mask =
        (board.pieces<BISHOP, ~c>() | board.pieces<QUEEN, ~c>()) & attacks::Bishop(sq, occ_all);
    U64 rook_mask =
        (board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>()) & attacks::Rook(sq, occ_all);

    /********************
     * We keep track of the amount of checks, in case there are
     * two checks on the board only the king can move!
     *******************/
    double_check = 0;

    if (pawn_mask) {
        checks |= pawn_mask;
        double_check++;
    }
    if (knight_mask) {
        checks |= knight_mask;
        double_check++;
    }
    if (bishop_mask) {
        int8_t index = builtin::lsb(bishop_mask);

        // Now we add the path!
        checks |= SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        double_check++;
    }
    if (rook_mask) {
        /********************
         * 3nk3/4P3/8/8/8/8/8/2K1R3 w - - 0 1, pawn promotes to queen or rook and
         * suddenly the same piecetype gives check two times
         * in that case we have a double check and can return early
         * because king moves dont require the checkmask.
         *******************/
        if (builtin::popcount(rook_mask) > 1) {
            double_check = 2;
            return checks;
        }

        int8_t index = builtin::lsb(rook_mask);

        // Now we add the path!
        checks |= SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        double_check++;
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
 * the possible pinner. We do this by simply using the builtin::popcount
 * of our pieces that lay on the pin mask, if it is only 1 piece then that piece is pinned.
 *******************/
template <Color c>
U64 pinMaskRooks(const Board &board, Square sq, U64 occ_us, U64 occ_enemy) {
    U64 rook_mask =
        (board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>()) & attacks::Rook(sq, occ_enemy);

    U64 pin_hv = 0ULL;
    while (rook_mask) {
        const Square index = builtin::poplsb(rook_mask);
        const U64 possible_pin = (SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (builtin::popcount(possible_pin & occ_us) == 1) pin_hv |= possible_pin;
    }
    return pin_hv;
}

template <Color c>
U64 pinMaskBishops(const Board &board, Square sq, U64 occ_us, U64 occ_enemy) {
    U64 bishop_mask =
        (board.pieces<BISHOP, ~c>() | board.pieces<QUEEN, ~c>()) & attacks::Bishop(sq, occ_enemy);

    U64 pin_d = 0ULL;

    while (bishop_mask) {
        const Square index = builtin::poplsb(bishop_mask);
        const U64 possible_pin = (SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (builtin::popcount(possible_pin & occ_us) == 1) pin_d |= possible_pin;
    }

    return pin_d;
}

/********************
 * Seen squares
 * We keep track of all attacked squares by the enemy
 * this is used for king move generation.
 *******************/
template <Color c>
U64 seenSquares(const Board &board, U64 occ_all) {
    const Square kSq = board.kingSQ(~c);

    U64 pawns = board.pieces<PAWN, c>();
    U64 knights = board.pieces<KNIGHT, c>();
    U64 queens = board.pieces<QUEEN, c>();
    U64 bishops = board.pieces<BISHOP, c>() | queens;
    U64 rooks = board.pieces<ROOK, c>() | queens;

    // Remove our king
    occ_all &= ~(1ULL << kSq);

    U64 seen = pawnLeftAttacks<c>(pawns) | pawnRightAttacks<c>(pawns);

    while (knights) {
        Square index = builtin::poplsb(knights);
        seen |= attacks::Knight(index);
    }
    while (bishops) {
        Square index = builtin::poplsb(bishops);
        seen |= attacks::Bishop(index, occ_all);
    }
    while (rooks) {
        Square index = builtin::poplsb(rooks);
        seen |= attacks::Rook(index, occ_all);
    }

    Square index = builtin::lsb(board.pieces<KING, c>());
    seen |= attacks::King(index);

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
void legalPawnMovesAll(const Board &board, Movelist &movelist, U64 occ_all, U64 occ_enemy,
                       U64 check_mask, U64 pin_hv, U64 pin_d) {
    const U64 pawns_mask = board.pieces<PAWN, c>();

    constexpr Direction UP = c == White ? NORTH : SOUTH;
    constexpr Direction DOWN = c == Black ? NORTH : SOUTH;
    constexpr Direction DOWN_LEFT = c == Black ? NORTH_EAST : SOUTH_WEST;
    constexpr Direction DOWN_RIGHT = c == Black ? NORTH_WEST : SOUTH_EAST;
    constexpr U64 RANK_BEFORE_PROMO = c == White ? MASK_RANK[RANK_7] : MASK_RANK[RANK_2];
    constexpr U64 RANK_PROMO = c == White ? MASK_RANK[RANK_8] : MASK_RANK[RANK_1];
    constexpr U64 doublePushRank = c == White ? MASK_RANK[RANK_3] : MASK_RANK[RANK_6];

    // These pawns can maybe take Left or Right
    const U64 pawnsLR = pawns_mask & ~pin_hv;

    const U64 unpinnedpawnsLR = pawnsLR & ~pin_d;
    const U64 pinnedpawnsLR = pawnsLR & pin_d;

    U64 Lpawns =
        (pawnLeftAttacks<c>(unpinnedpawnsLR)) | (pawnLeftAttacks<c>(pinnedpawnsLR) & pin_d);
    U64 Rpawns =
        (pawnRightAttacks<c>(unpinnedpawnsLR)) | (pawnRightAttacks<c>(pinnedpawnsLR) & pin_d);

    // Prune moves that dont capture a piece and are not on the check_mask.
    Lpawns &= occ_enemy & check_mask;
    Rpawns &= occ_enemy & check_mask;

    // These pawns can walk Forward
    const U64 pawnsHV = pawns_mask & ~pin_d;

    const U64 pawnsPinnedHV = pawnsHV & pin_hv;
    const U64 pawnsUnPinnedHV = pawnsHV & ~pin_hv;

    // Prune moves that are blocked by a piece
    const U64 singlePushUnpinned = shift<UP>(pawnsUnPinnedHV) & ~occ_all;
    const U64 singlePushPinned = shift<UP>(pawnsPinnedHV) & pin_hv & ~occ_all;

    // Prune moves that are not on the check_mask.
    U64 singlePush = (singlePushUnpinned | singlePushPinned) & check_mask;

    U64 doublePush = ((shift<UP>(singlePushUnpinned & doublePushRank) & ~occ_all) |
                      (shift<UP>(singlePushPinned & doublePushRank) & ~occ_all)) &
                     check_mask;

    /********************
     * Add promotion moves.
     * These are always generated unless we only want quiet moves.
     *******************/
    if ((mt != Movetype::QUIET) && pawns_mask & RANK_BEFORE_PROMO) {
        U64 Promote_Left = Lpawns & RANK_PROMO;
        U64 Promote_Right = Rpawns & RANK_PROMO;
        U64 Promote_Move = singlePush & RANK_PROMO;

        while (Promote_Move) {
            Square to = builtin::poplsb(Promote_Move);
            movelist.add(make<PROMOTION>(to + DOWN, to, QUEEN));
            movelist.add(make<PROMOTION>(to + DOWN, to, ROOK));
            movelist.add(make<PROMOTION>(to + DOWN, to, KNIGHT));
            movelist.add(make<PROMOTION>(to + DOWN, to, BISHOP));
        }

        while (Promote_Right) {
            Square to = builtin::poplsb(Promote_Right);
            movelist.add(make<PROMOTION>(to + DOWN_LEFT, to, QUEEN));
            movelist.add(make<PROMOTION>(to + DOWN_LEFT, to, ROOK));
            movelist.add(make<PROMOTION>(to + DOWN_LEFT, to, KNIGHT));
            movelist.add(make<PROMOTION>(to + DOWN_LEFT, to, BISHOP));
        }

        while (Promote_Left) {
            Square to = builtin::poplsb(Promote_Left);
            movelist.add(make<PROMOTION>(to + DOWN_RIGHT, to, QUEEN));
            movelist.add(make<PROMOTION>(to + DOWN_RIGHT, to, ROOK));
            movelist.add(make<PROMOTION>(to + DOWN_RIGHT, to, KNIGHT));
            movelist.add(make<PROMOTION>(to + DOWN_RIGHT, to, BISHOP));
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
        Square to = builtin::poplsb(singlePush);
        movelist.add(make(to + DOWN, to));
    }

    /********************
     * Add double pushs.
     *******************/
    while (mt != Movetype::CAPTURE && doublePush) {
        Square to = builtin::poplsb(doublePush);
        movelist.add(make(to + DOWN + DOWN, to));
    }

    /********************
     * Add right pawn captures.
     *******************/
    while (mt != Movetype::QUIET && Rpawns) {
        Square to = builtin::poplsb(Rpawns);
        movelist.add(make(to + DOWN_LEFT, to));
    }

    /********************
     * Add left pawn captures.
     *******************/
    while (mt != Movetype::QUIET && Lpawns) {
        Square to = builtin::poplsb(Lpawns);
        movelist.add(make(to + DOWN_RIGHT, to));
    }

    /********************
     * Add en passant captures.
     *******************/
    if (mt != Movetype::QUIET && board.en_passant_square != NO_SQ) {
        const Square ep = board.en_passant_square;
        const Square epPawn = ep + DOWN;

        U64 epMask = (1ull << epPawn) | (1ull << ep);

        /********************
         * In case the en passant square and the enemy pawn
         * that just moved are not on the check_mask
         * en passant is not available.
         *******************/
        if ((check_mask & epMask) == 0) return;

        const Square kSQ = board.kingSQ(c);
        const U64 kingMask = (1ull << kSQ) & MASK_RANK[square_rank(epPawn)];
        const U64 enemyQueenRook = board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>();

        const bool isPossiblePin = kingMask && enemyQueenRook;
        U64 epBB = attacks::Pawn(ep, ~c) & pawnsLR;

        /********************
         * For one en passant square two pawns could potentially take there.
         *******************/
        while (epBB) {
            Square from = builtin::poplsb(epBB);
            Square to = ep;

            /********************
             * If the pawn is pinned but the en passant square is not on the
             * pin mask then the move is illegal.
             *******************/
            if ((1ULL << from) & pin_d && !(pin_d & (1ull << ep))) continue;

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
                (attacks::Rook(kSQ, occ_all & ~connectingPawns) & enemyQueenRook) != 0)
                break;

            movelist.add(make<ENPASSANT>(from, to));
        }
    }
}

inline U64 legalKnightMoves(Square sq, U64 movable_square) {
    return attacks::Knight(sq) & movable_square;
}

inline U64 legalBishopMoves(Square sq, U64 movable_square, U64 pinMask, U64 occ_all) {
    // The Bishop is pinned diagonally thus can only move diagonally.
    if (pinMask & (1ULL << sq)) return attacks::Bishop(sq, occ_all) & movable_square & pinMask;
    return attacks::Bishop(sq, occ_all) & movable_square;
}

inline U64 legalRookMoves(Square sq, U64 movable_square, U64 pinMask, U64 occ_all) {
    // The Rook is pinned horizontally thus can only move horizontally.
    if (pinMask & (1ULL << sq)) return attacks::Rook(sq, occ_all) & movable_square & pinMask;
    return attacks::Rook(sq, occ_all) & movable_square;
}

inline U64 legalQueenMoves(Square sq, U64 movable_square, U64 pin_d, U64 pin_hv, U64 occ_all) {
    U64 moves = 0ULL;
    if (pin_d & (1ULL << sq))
        moves |= attacks::Bishop(sq, occ_all) & movable_square & pin_d;
    else if (pin_hv & (1ULL << sq))
        moves |= attacks::Rook(sq, occ_all) & movable_square & pin_hv;
    else {
        moves |= attacks::Rook(sq, occ_all) & movable_square;
        moves |= attacks::Bishop(sq, occ_all) & movable_square;
    }

    return moves;
}

template <Movetype mt>
U64 legalKingMoves(Square sq, U64 movable_square, U64 seen) {
    return attacks::King(sq) & movable_square & ~seen;
}

constexpr Square relativeSquare(Color c, Square s) {
    return Square(s ^ (static_cast<int>(c) * 56));
}

template <Color c, Movetype mt>
inline U64 legalCastleMoves(const Board &board, Square sq, U64 seen, U64 pin_hv, U64 occ_all) {
    if constexpr (mt == Movetype::CAPTURE) return 0ull;
    const auto rights = board.castling_rights;

    U64 moves = 0ull;

    for (const auto side : {CastleSide::KING_SIDE, CastleSide::QUEEN_SIDE}) {
        if (!rights.hasCastlingRight(c, side)) continue;

        const auto end_king_sq =
            relativeSquare(c, side == CastleSide::KING_SIDE ? Square::SQ_G1 : Square::SQ_C1);
        const auto end_rook_sq =
            relativeSquare(c, side == CastleSide::KING_SIDE ? Square::SQ_F1 : Square::SQ_D1);

        const auto from_rook_sq = file_rank_square(rights.getRookFile(c, side), square_rank(sq));

        const U64 not_occ_path = SQUARES_BETWEEN_BB[sq][from_rook_sq];
        const U64 not_attacked_path = SQUARES_BETWEEN_BB[sq][end_king_sq];
        const U64 empty_not_attacked = ~seen & ~(occ_all & ~(1ull << from_rook_sq));
        const U64 withoutRook = occ_all & ~(1ull << from_rook_sq);

        if ((not_attacked_path & empty_not_attacked) == not_attacked_path &&
            ((not_occ_path & ~occ_all) == not_occ_path) &&
            !((1ull << from_rook_sq) & pin_hv & MASK_RANK[static_cast<int>(square_rank(sq))]) &&
            !((1ull << end_rook_sq) & withoutRook) &&
            !((1ull << end_king_sq) & (seen | (withoutRook & ~(1ull << sq))))) {
            moves |= (1ull << from_rook_sq);
        }
    }

    return moves;
}

// all legal moves for a position
template <Color c, Movetype mt>
void legalmoves(const Board &board, Movelist &movelist) {
    /********************
     * The size of the movelist might not
     * be 0! This is done on purpose since it enables
     * you to append new move types to any movelist.
     *******************/

    Square king_sq = board.kingSQ(c);

    int double_check = 0;

    U64 occ_us = board.us<c>();
    U64 occ_enemy = board.us<~c>();
    U64 occ_all = occ_us | occ_enemy;
    U64 enemy_empty_bb = ~occ_us;

    U64 seen = seenSquares<~c>(board, occ_all);
    U64 check_mask = checkMask<c>(board, king_sq, occ_all, double_check);
    U64 pin_hv = pinMaskRooks<c>(board, king_sq, occ_us, occ_enemy);
    U64 pin_d = pinMaskBishops<c>(board, king_sq, occ_us, occ_enemy);

    assert(double_check <= 2);

    /********************
     * Moves have to be on the check_mask
     *******************/
    U64 movable_square;

    /********************
     * Slider, Knights and King moves can only go to enemy or empty squares.
     *******************/
    if (mt == Movetype::ALL)
        movable_square = enemy_empty_bb;
    else if (mt == Movetype::CAPTURE)
        movable_square = occ_enemy;
    else  // QUIET moves
        movable_square = ~occ_all;

    U64 moves = legalKingMoves<mt>(king_sq, movable_square, seen);

    movable_square &= check_mask;

    while (moves) {
        Square to = builtin::poplsb(moves);
        movelist.add(make(king_sq, to));
    }

    if (mt != Movetype::CAPTURE && square_rank(king_sq) == (c == White ? RANK_1 : RANK_8) &&
        board.castling_rights.hasCastlingRight(c) && check_mask == DEFAULT_CHECKMASK) {
        moves = legalCastleMoves<c, mt>(board, king_sq, seen, pin_hv, occ_all);

        while (moves) {
            Square to = builtin::poplsb(moves);
            movelist.add(make<CASTLING>(king_sq, to));
        }
    }

    /********************
     * Early return for double check as described earlier
     *******************/
    if (double_check == 2) return;

    /********************
     * Prune knights that are pinned since these cannot move.
     *******************/
    U64 knights_mask = board.pieces<KNIGHT, c>() & ~(pin_d | pin_hv);

    /********************
     * Prune horizontally pinned bishops
     *******************/
    U64 bishops_mask = board.pieces<BISHOP, c>() & ~pin_hv;

    /********************
     * Prune diagonally pinned rooks
     *******************/
    U64 rooks_mask = board.pieces<ROOK, c>() & ~pin_d;

    /********************
     * Prune double pinned queens
     *******************/
    U64 queens_mask = board.pieces<QUEEN, c>() & ~(pin_d & pin_hv);

    /********************
     * Add the moves to the movelist.
     *******************/
    legalPawnMovesAll<c, mt>(board, movelist, occ_all, occ_enemy, check_mask, pin_hv, pin_d);

    while (knights_mask) {
        Square from = builtin::poplsb(knights_mask);
        U64 moves = legalKnightMoves(from, movable_square);
        while (moves) {
            Square to = builtin::poplsb(moves);
            movelist.add(make(from, to));
        }
    }

    while (bishops_mask) {
        Square from = builtin::poplsb(bishops_mask);
        U64 moves = legalBishopMoves(from, movable_square, pin_d, occ_all);
        while (moves) {
            Square to = builtin::poplsb(moves);
            movelist.add(make(from, to));
        }
    }

    while (rooks_mask) {
        Square from = builtin::poplsb(rooks_mask);
        U64 moves = legalRookMoves(from, movable_square, pin_hv, occ_all);
        while (moves) {
            Square to = builtin::poplsb(moves);
            movelist.add(make(from, to));
        }
    }

    while (queens_mask) {
        Square from = builtin::poplsb(queens_mask);
        U64 moves = legalQueenMoves(from, movable_square, pin_d, pin_hv, occ_all);
        while (moves) {
            Square to = builtin::poplsb(moves);
            movelist.add(make(from, to));
        }
    }
}

/********************
 * Entry function for the
 * Color template.
 *******************/
template <Movetype mt>
void legalmoves(const Board &board, Movelist &movelist) {
    if (board.side_to_move == White)
        legalmoves<White, mt>(board, movelist);
    else
        legalmoves<Black, mt>(board, movelist);
}

/********************
 * Static Exchange Evaluation, logical based on Weiss (https://github.com/TerjeKir/weiss)
 *licensed under GPL-3.0
 *******************/
inline bool see(const Board &board, Move move, int threshold) {
    Square from_sq = from(move);
    Square to_sq = to(move);
    PieceType attacker = type_of_piece(board.at(from_sq));
    PieceType victim = type_of_piece(board.at(to_sq));
    int swap = pieceValuesDefault[victim] - threshold;
    if (swap < 0) return false;
    swap -= pieceValuesDefault[attacker];
    if (swap >= 0) return true;
    U64 occ = (board.all() ^ (1ULL << from_sq)) | (1ULL << to_sq);
    U64 attackers = board.allAttackers(to_sq, occ) & occ;

    U64 queens = board.pieces(WhiteQueen) | board.pieces(BlackQueen);

    U64 bishops = board.pieces(WhiteBishop) | board.pieces(BlackBishop) | queens;
    U64 rooks = board.pieces(WhiteRook) | board.pieces(BlackRook) | queens;

    Color sT = ~board.colorOf(from_sq);

    while (true) {
        attackers &= occ;
        U64 myAttackers = attackers & board.us(sT);
        if (!myAttackers) break;

        int pt;
        for (pt = 0; pt <= 5; pt++) {
            if (myAttackers &
                (board.pieces(static_cast<Piece>(pt)) | board.pieces(static_cast<Piece>(pt + 6))))
                break;
        }
        sT = ~sT;
        if ((swap = -swap - 1 - piece_values[MG][pt]) >= 0) {
            if (pt == KING && (attackers & board.us(sT))) sT = ~sT;
            break;
        }

        occ ^= (1ULL << (builtin::lsb(myAttackers & (board.pieces(static_cast<Piece>(pt)) |
                                                     board.pieces(static_cast<Piece>(pt + 6))))));

        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= attacks::Bishop(to_sq, occ) & bishops;
        if (pt == ROOK || pt == QUEEN) attackers |= attacks::Rook(to_sq, occ) & rooks;
    }
    return sT != board.colorOf(from_sq);
}

}  // namespace movegen

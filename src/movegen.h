#pragma once

#include <iterator>

#include "board.h"
#include "builtin.h"
#include "helper.h"
#include "types.h"

static auto init_squares_between = []() constexpr {
    // initialize squares between table
    std::array<std::array<Bitboard, 64>, 64> squares_between_bb{};
    Bitboard sqs = 0;
    for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1) {
        for (Square sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2) {
            sqs = (1ULL << sq1) | (1ULL << sq2);
            if (sq1 == sq2)
                squares_between_bb[sq1][sq2] = 0ull;
            else if (squareFile(sq1) == squareFile(sq2) || squareRank(sq1) == squareRank(sq2))
                squares_between_bb[sq1][sq2] = attacks::rook(sq1, sqs) & attacks::rook(sq2, sqs);
            else if (diagonalOf(sq1) == diagonalOf(sq2) ||
                     antiDiagonalOf(sq1) == antiDiagonalOf(sq2))
                squares_between_bb[sq1][sq2] =
                    attacks::bishop(sq1, sqs) & attacks::bishop(sq2, sqs);
        }
    }
    return squares_between_bb;
};

static const std::array<std::array<Bitboard, 64>, 64> SQUARES_BETWEEN_BB = init_squares_between();

struct ExtMove {
    int value = 0;
    Move move = NO_MOVE;
};

constexpr bool operator==(const ExtMove &a, const ExtMove &b) { return a.move == b.move; }

constexpr bool operator>(const ExtMove &a, const ExtMove &b) { return a.value > b.value; }

constexpr bool operator<(const ExtMove &a, const ExtMove &b) { return a.value < b.value; }

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

    [[nodiscard]] constexpr ExtMove &operator[](int i) { return list[i]; }

    [[nodiscard]] constexpr int find(Move m) {
        for (int i = 0; i < size; i++) {
            if (list[i].move == m) return i;
        }
        return -1;
    }

    iterator begin() { return (std::begin(list)); }

    [[nodiscard]] const_iterator begin() const { return (std::begin(list)); }

    iterator end() {
        auto it = std::begin(list);
        std::advance(it, size);
        return it;
    }

    [[nodiscard]] const_iterator end() const {
        auto it = std::begin(list);
        std::advance(it, size);
        return it;
    }
};

namespace movegen {

template <Color c>
[[nodiscard]] Bitboard pawnLeftAttacks(const Bitboard pawns) {
    return c == WHITE ? (pawns << 7) & ~MASK_FILE[FILE_H] : (pawns >> 7) & ~MASK_FILE[FILE_A];
}

template <Color c>
[[nodiscard]] Bitboard pawnRightAttacks(const Bitboard pawns) {
    return c == WHITE ? (pawns << 9) & ~MASK_FILE[FILE_A] : (pawns >> 9) & ~MASK_FILE[FILE_H];
}

/********************
 * Creates the checkmask.
 * A checkmask is the path from the enemy checker to our king.
 * Knight and pawns get themselves added to the checkmask, otherwise the path is added.
 * When there is no check at all all bits are set (DEFAULT_CHECKMASK)
 *******************/
template <Color c>
[[nodiscard]] Bitboard checkMask(const Board &board, Square sq, int &double_check) {
    Bitboard mask = 0;
    double_check = 0;

    const auto opp_knight = board.pieces(PieceType::KNIGHT, ~c);
    const auto opp_bishop = board.pieces(PieceType::BISHOP, ~c);
    const auto opp_rook = board.pieces(PieceType::ROOK, ~c);
    const auto opp_queen = board.pieces(PieceType::QUEEN, ~c);

    const auto opp_pawns = board.pieces(PieceType::PAWN, ~c);

    // check for knight checks
    Bitboard knight_attacks = attacks::knight(sq) & opp_knight;
    double_check += bool(knight_attacks);

    mask |= knight_attacks;

    // check for pawn checks
    Bitboard pawn_attacks = attacks::pawn(sq, board.sideToMove()) & opp_pawns;
    mask |= pawn_attacks;
    double_check += bool(pawn_attacks);

    // check for bishop checks
    Bitboard bishop_attacks = attacks::bishop(sq, board.all()) & (opp_bishop | opp_queen);

    if (bishop_attacks) {
        const auto index = builtin::lsb(bishop_attacks);

        mask |= SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        double_check++;
    }

    Bitboard rook_attacks = attacks::rook(sq, board.all()) & (opp_rook | opp_queen);
    if (rook_attacks) {
        /********************
         * 3nk3/4P3/8/8/8/8/8/2K1R3 w - - 0 1, pawn promotes to queen or rook and
         * suddenly the same piecetype gives check two times
         * in that case we have a double check and can return early
         * because king moves dont require the checkmask.
         *******************/
        if (builtin::popcount(rook_attacks) > 1) {
            double_check = 2;
            return mask;
        }

        const auto index = builtin::lsb(rook_attacks);

        mask |= SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        double_check++;
    }

    if (!mask) {
        return DEFAULT_CHECKMASK;
    }

    return mask;
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
[[nodiscard]] Bitboard pinMaskRooks(const Board &board, Square sq, Bitboard occ_us,
                                    Bitboard occ_enemy) {
    Bitboard pin_hv = 0;

    const auto opp_rook = board.pieces(PieceType::ROOK, ~c);
    const auto opp_queen = board.pieces(PieceType::QUEEN, ~c);

    Bitboard rook_attacks = attacks::rook(sq, occ_enemy) & (opp_rook | opp_queen);

    while (rook_attacks) {
        const auto index = builtin::poplsb(rook_attacks);

        const Bitboard possible_pin = SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        if (builtin::popcount(possible_pin & occ_us) == 1) pin_hv |= possible_pin;
    }

    return pin_hv;
}

template <Color c>
[[nodiscard]] Bitboard pinMaskBishops(const Board &board, Square sq, Bitboard occ_us,
                                      Bitboard occ_enemy) {
    Bitboard pin_diag = 0;

    const auto opp_bishop = board.pieces(PieceType::BISHOP, ~c);
    const auto opp_queen = board.pieces(PieceType::QUEEN, ~c);

    Bitboard bishop_attacks = attacks::bishop(sq, occ_enemy) & (opp_bishop | opp_queen);

    while (bishop_attacks) {
        const auto index = builtin::poplsb(bishop_attacks);

        const Bitboard possible_pin = SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        if (builtin::popcount(possible_pin & occ_us) == 1) pin_diag |= possible_pin;
    }

    return pin_diag;
}

/********************
 * Seen squares
 * We keep track of all attacked squares by the enemy
 * this is used for king move generation.
 *******************/
template <Color c>
[[nodiscard]] Bitboard seenSquares(const Board &board, Bitboard enemy_empty) {
    const Square king_sq = board.kingSQ(~c);

    Bitboard pawns = board.pieces<PAWN, c>();
    Bitboard knights = board.pieces<KNIGHT, c>();
    Bitboard queens = board.pieces<QUEEN, c>();
    Bitboard bishops = board.pieces<BISHOP, c>() | queens;
    Bitboard rooks = board.pieces<ROOK, c>() | queens;

    auto occ = board.all();

    Bitboard map_king_atk = attacks::king(king_sq) & enemy_empty;

    if (map_king_atk == 0ull && !board.chess960) {
        return 0ull;
    }

    // Remove our king
    occ &= ~(1ULL << king_sq);

    Bitboard seen = pawnLeftAttacks<c>(pawns) | pawnRightAttacks<c>(pawns);

    while (knights) {
        Square index = builtin::poplsb(knights);
        seen |= attacks::knight(index);
    }

    while (bishops) {
        Square index = builtin::poplsb(bishops);
        seen |= attacks::bishop(index, occ);
    }

    while (rooks) {
        Square index = builtin::poplsb(rooks);
        seen |= attacks::rook(index, occ);
    }

    Square index = builtin::lsb(board.pieces<KING, c>());
    seen |= attacks::king(index);

    return seen;
}

/// @brief shift a mask in a direction
/// @tparam direction
/// @param b
/// @return
template <Direction direction>
[[nodiscard]] constexpr Bitboard shift(const Bitboard b) {
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
void addLegalPawnMoves(const Board &board, Movelist &movelist, Bitboard occ_all, Bitboard occ_enemy,
                       Bitboard check_mask, Bitboard pin_hv, Bitboard pin_d) {
    const Bitboard pawns_mask = board.pieces<PAWN, c>();

    constexpr Direction UP = c == WHITE ? NORTH : SOUTH;
    constexpr Direction DOWN = c == BLACK ? NORTH : SOUTH;
    constexpr Direction DOWN_LEFT = c == BLACK ? NORTH_EAST : SOUTH_WEST;
    constexpr Direction DOWN_RIGHT = c == BLACK ? NORTH_WEST : SOUTH_EAST;
    constexpr Bitboard RANK_BEFORE_PROMO = c == WHITE ? MASK_RANK[RANK_7] : MASK_RANK[RANK_2];
    constexpr Bitboard RANK_PROMO = c == WHITE ? MASK_RANK[RANK_8] : MASK_RANK[RANK_1];
    constexpr Bitboard DOUBLE_PUSH_RANK = c == WHITE ? MASK_RANK[RANK_3] : MASK_RANK[RANK_6];

    // These pawns can maybe take Left or Right
    const Bitboard pawns_lr = pawns_mask & ~pin_hv;

    const Bitboard unpinned_pawns_lr = pawns_lr & ~pin_d;
    const Bitboard pinned_pawns_lr = pawns_lr & pin_d;

    Bitboard l_pawns =
        (pawnLeftAttacks<c>(unpinned_pawns_lr)) | (pawnLeftAttacks<c>(pinned_pawns_lr) & pin_d);
    Bitboard r_pawns =
        (pawnRightAttacks<c>(unpinned_pawns_lr)) | (pawnRightAttacks<c>(pinned_pawns_lr) & pin_d);

    // Prune moves that dont capture a piece and are not on the check_mask.
    l_pawns &= occ_enemy & check_mask;
    r_pawns &= occ_enemy & check_mask;

    // These pawns can walk Forward
    const Bitboard pawns_hv = pawns_mask & ~pin_d;

    const Bitboard pawns_pinned_hv = pawns_hv & pin_hv;
    const Bitboard pawns_unpinned_hv = pawns_hv & ~pin_hv;

    // Prune moves that are blocked by a piece
    const Bitboard single_push_unpinned = shift<UP>(pawns_unpinned_hv) & ~occ_all;
    const Bitboard single_push_pinned = shift<UP>(pawns_pinned_hv) & pin_hv & ~occ_all;

    // Prune moves that are not on the check_mask.
    Bitboard single_push = (single_push_unpinned | single_push_pinned) & check_mask;

    Bitboard double_push = ((shift<UP>(single_push_unpinned & DOUBLE_PUSH_RANK) & ~occ_all) |
                            (shift<UP>(single_push_pinned & DOUBLE_PUSH_RANK) & ~occ_all)) &
                           check_mask;

    /********************
     * Add promotion moves.
     * These are always generated unless we only want quiet moves.
     *******************/
    if ((mt != Movetype::QUIET) && pawns_mask & RANK_BEFORE_PROMO) {
        Bitboard promote_left = l_pawns & RANK_PROMO;
        Bitboard promote_right = r_pawns & RANK_PROMO;
        Bitboard promote_move = single_push & RANK_PROMO;

        while (promote_move) {
            Square to = builtin::poplsb(promote_move);
            movelist.add(make<PROMOTION>(to + DOWN, to, QUEEN));
            movelist.add(make<PROMOTION>(to + DOWN, to, ROOK));
            movelist.add(make<PROMOTION>(to + DOWN, to, KNIGHT));
            movelist.add(make<PROMOTION>(to + DOWN, to, BISHOP));
        }

        while (promote_right) {
            Square to = builtin::poplsb(promote_right);
            movelist.add(make<PROMOTION>(to + DOWN_LEFT, to, QUEEN));
            movelist.add(make<PROMOTION>(to + DOWN_LEFT, to, ROOK));
            movelist.add(make<PROMOTION>(to + DOWN_LEFT, to, KNIGHT));
            movelist.add(make<PROMOTION>(to + DOWN_LEFT, to, BISHOP));
        }

        while (promote_left) {
            Square to = builtin::poplsb(promote_left);
            movelist.add(make<PROMOTION>(to + DOWN_RIGHT, to, QUEEN));
            movelist.add(make<PROMOTION>(to + DOWN_RIGHT, to, ROOK));
            movelist.add(make<PROMOTION>(to + DOWN_RIGHT, to, KNIGHT));
            movelist.add(make<PROMOTION>(to + DOWN_RIGHT, to, BISHOP));
        }
    }

    // Remove the promotion pawns
    single_push &= ~RANK_PROMO;
    l_pawns &= ~RANK_PROMO;
    r_pawns &= ~RANK_PROMO;

    /********************
     * Add single pushs.
     *******************/
    while (mt != Movetype::CAPTURE && single_push) {
        Square to = builtin::poplsb(single_push);
        movelist.add(make(to + DOWN, to));
    }

    /********************
     * Add double pushs.
     *******************/
    while (mt != Movetype::CAPTURE && double_push) {
        Square to = builtin::poplsb(double_push);
        movelist.add(make(to + DOWN + DOWN, to));
    }

    /********************
     * Add right pawn captures.
     *******************/
    while (mt != Movetype::QUIET && r_pawns) {
        Square to = builtin::poplsb(r_pawns);
        movelist.add(make(to + DOWN_LEFT, to));
    }

    /********************
     * Add left pawn captures.
     *******************/
    while (mt != Movetype::QUIET && l_pawns) {
        Square to = builtin::poplsb(l_pawns);
        movelist.add(make(to + DOWN_RIGHT, to));
    }

    /********************
     * Add en passant captures.
     *******************/
    if (mt != Movetype::QUIET && board.enPassant() != NO_SQ) {
        const Square ep = board.enPassant();
        const Square ep_pawn = ep + DOWN;

        Bitboard ep_mask = (1ull << ep_pawn) | (1ull << ep);

        /********************
         * In case the en passant square and the enemy pawn
         * that just moved are not on the check_mask
         * en passant is not available.
         *******************/
        if ((check_mask & ep_mask) == 0) return;

        const Square k_sq = board.kingSQ(c);
        const Bitboard king_mask = (1ull << k_sq) & MASK_RANK[squareRank(ep_pawn)];
        const Bitboard enemy_queen_rook = board.pieces<ROOK, ~c>() | board.pieces<QUEEN, ~c>();

        const bool is_possible_pin = king_mask && enemy_queen_rook;
        Bitboard ep_bb = attacks::pawn(ep, ~c) & pawns_lr;

        /********************
         * For one en passant square two pawns could potentially take there.
         *******************/
        while (ep_bb) {
            Square from = builtin::poplsb(ep_bb);
            Square to = ep;

            /********************
             * If the pawn is pinned but the en passant square is not on the
             * pin mask then the move is illegal.
             *******************/
            if ((1ULL << from) & pin_d && !(pin_d & (1ull << ep))) continue;

            const Bitboard connecting_pawns = (1ull << ep_pawn) | (1ull << from);

            /********************
             * 7k/4p3/8/2KP3r/8/8/8/8 b - - 0 1
             * If e7e5 there will be a potential ep square for us on e6.
             * However we cannot take en passant because that would put our king
             * in check. For this scenario we check if theres an enemy rook/queen
             * that would give check if the two pawns were removed.
             * If thats the case then the move is illegal and we can break immediately.
             *******************/
            if (is_possible_pin &&
                (attacks::rook(k_sq, occ_all & ~connecting_pawns) & enemy_queen_rook) != 0)
                break;

            movelist.add(make<ENPASSANT>(from, to));
        }
    }
}

[[nodiscard]] inline Bitboard generateLegalKnightMoves(Square sq, Bitboard movable_square) {
    return attacks::knight(sq) & movable_square;
}

[[nodiscard]] inline Bitboard generateLegalBishopMoves(Square sq, Bitboard movable_square,
                                                       Bitboard pinMask, Bitboard occ_all) {
    // The Bishop is pinned diagonally thus can only move diagonally.
    if (pinMask & (1ULL << sq)) return attacks::bishop(sq, occ_all) & movable_square & pinMask;
    return attacks::bishop(sq, occ_all) & movable_square;
}

[[nodiscard]] inline Bitboard generateLegalRookMoves(Square sq, Bitboard movable_square,
                                                     Bitboard pinMask, Bitboard occ_all) {
    // The Rook is pinned horizontally thus can only move horizontally.
    if (pinMask & (1ULL << sq)) return attacks::rook(sq, occ_all) & movable_square & pinMask;
    return attacks::rook(sq, occ_all) & movable_square;
}

[[nodiscard]] inline Bitboard generateLegalQueenMoves(Square sq, Bitboard movable_square,
                                                      Bitboard pin_d, Bitboard pin_hv,
                                                      Bitboard occ_all) {
    Bitboard moves = 0ULL;
    if (pin_d & (1ULL << sq))
        moves |= attacks::bishop(sq, occ_all) & movable_square & pin_d;
    else if (pin_hv & (1ULL << sq))
        moves |= attacks::rook(sq, occ_all) & movable_square & pin_hv;
    else {
        moves |= attacks::rook(sq, occ_all) & movable_square;
        moves |= attacks::bishop(sq, occ_all) & movable_square;
    }

    return moves;
}

template <Movetype mt>
[[nodiscard]] Bitboard generateLegalKingMoves(Square sq, Bitboard movable_square, Bitboard seen) {
    return attacks::king(sq) & movable_square & ~seen;
}

[[nodiscard]] constexpr Square relativeSquare(Color c, Square s) {
    return Square(s ^ (static_cast<int>(c) * 56));
}

template <Color c, Movetype mt>
[[nodiscard]] inline Bitboard generateLegalCastlingMoves(const Board &board, Square sq,
                                                         Bitboard seen, Bitboard pin_hv,
                                                         Bitboard occ_all) {
    if constexpr (mt == Movetype::CAPTURE) return 0ull;
    const auto rights = board.castlingRights();

    Bitboard moves = 0ull;

    for (const auto side : {CastleSide::KING_SIDE, CastleSide::QUEEN_SIDE}) {
        if (!rights.hasCastlingRight(c, side)) continue;

        const auto end_king_sq =
            relativeSquare(c, side == CastleSide::KING_SIDE ? Square::SQ_G1 : Square::SQ_C1);
        const auto end_rook_sq =
            relativeSquare(c, side == CastleSide::KING_SIDE ? Square::SQ_F1 : Square::SQ_D1);

        const auto from_rook_sq = fileRankSquare(rights.getRookFile(c, side), squareRank(sq));

        const Bitboard not_occ_path = SQUARES_BETWEEN_BB[sq][from_rook_sq];
        const Bitboard not_attacked_path = SQUARES_BETWEEN_BB[sq][end_king_sq];
        const Bitboard empty_not_attacked = ~seen & ~(occ_all & ~(1ull << from_rook_sq));
        const Bitboard withoutRook = occ_all & ~(1ull << from_rook_sq);
        const Bitboard withoutKing = occ_all & ~(1ull << sq);

        if ((not_attacked_path & empty_not_attacked) == not_attacked_path &&
            ((not_occ_path & ~occ_all) == not_occ_path) &&
            !((1ull << from_rook_sq) & pin_hv & MASK_RANK[static_cast<int>(squareRank(sq))]) &&
            !((1ull << end_rook_sq) & (withoutRook & withoutKing)) &&
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

    int double_check = 0;

    const Square king_sq = board.kingSQ(c);

    const Bitboard occ_us = board.us<c>();
    const Bitboard occ_enemy = board.us<~c>();
    const Bitboard occ_all = occ_us | occ_enemy;
    const Bitboard enemy_empty_bb = ~occ_us;

    const Bitboard seen = seenSquares<~c>(board, enemy_empty_bb);
    const Bitboard check_mask = checkMask<c>(board, king_sq, double_check);
    const Bitboard pin_hv = pinMaskRooks<c>(board, king_sq, occ_us, occ_enemy);
    const Bitboard pin_d = pinMaskBishops<c>(board, king_sq, occ_us, occ_enemy);

    assert(double_check <= 2);

    /********************
     * Moves have to be on the check_mask
     *******************/
    Bitboard movable_square;

    /********************
     * Slider, Knights and King moves can only go to enemy or empty squares.
     *******************/
    if (mt == Movetype::ALL)
        movable_square = enemy_empty_bb;
    else if (mt == Movetype::CAPTURE)
        movable_square = occ_enemy;
    else  // QUIET moves
        movable_square = ~occ_all;

    {
        Bitboard moves = generateLegalKingMoves<mt>(king_sq, movable_square, seen);

        movable_square &= check_mask;

        while (moves) {
            Square to = builtin::poplsb(moves);
            movelist.add(make(king_sq, to));
        }

        if (mt != Movetype::CAPTURE && squareRank(king_sq) == (c == WHITE ? RANK_1 : RANK_8) &&
            board.castlingRights().hasCastlingRight(c) && check_mask == DEFAULT_CHECKMASK) {
            moves = generateLegalCastlingMoves<c, mt>(board, king_sq, seen, pin_hv, occ_all);

            while (moves) {
                Square to = builtin::poplsb(moves);
                movelist.add(make<CASTLING>(king_sq, to));
            }
        }
    }

    /********************
     * Early return for double check as described earlier
     *******************/
    if (double_check == 2) return;

    /********************
     * Prune knights that are pinned since these cannot move.
     *******************/
    Bitboard knights_mask = board.pieces<KNIGHT, c>() & ~(pin_d | pin_hv);

    /********************
     * Prune horizontally pinned bishops
     *******************/
    Bitboard bishops_mask = board.pieces<BISHOP, c>() & ~pin_hv;

    /********************
     * Prune diagonally pinned rooks
     *******************/
    Bitboard rooks_mask = board.pieces<ROOK, c>() & ~pin_d;

    /********************
     * Prune double pinned queens
     *******************/
    Bitboard queens_mask = board.pieces<QUEEN, c>() & ~(pin_d & pin_hv);

    /********************
     * Add the moves to the movelist.
     *******************/
    addLegalPawnMoves<c, mt>(board, movelist, occ_all, occ_enemy, check_mask, pin_hv, pin_d);

    while (knights_mask) {
        Square from = builtin::poplsb(knights_mask);
        Bitboard moves = generateLegalKnightMoves(from, movable_square);
        while (moves) {
            Square to = builtin::poplsb(moves);
            movelist.add(make(from, to));
        }
    }

    while (bishops_mask) {
        Square from = builtin::poplsb(bishops_mask);
        Bitboard moves = generateLegalBishopMoves(from, movable_square, pin_d, occ_all);
        while (moves) {
            Square to = builtin::poplsb(moves);
            movelist.add(make(from, to));
        }
    }

    while (rooks_mask) {
        Square from = builtin::poplsb(rooks_mask);
        Bitboard moves = generateLegalRookMoves(from, movable_square, pin_hv, occ_all);
        while (moves) {
            Square to = builtin::poplsb(moves);
            movelist.add(make(from, to));
        }
    }

    while (queens_mask) {
        Square from = builtin::poplsb(queens_mask);
        Bitboard moves = generateLegalQueenMoves(from, movable_square, pin_d, pin_hv, occ_all);
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
    if (board.sideToMove() == WHITE)
        legalmoves<WHITE, mt>(board, movelist);
    else
        legalmoves<BLACK, mt>(board, movelist);
}

}  // namespace movegen

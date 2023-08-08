#pragma once

#include "board.h"
#include "builtin.h"

namespace see {

[[nodiscard]] inline Bitboard attackersForSide(const Board &board, Color attacker_color, Square sq,
                                               Bitboard occupied_bb) {
    const Bitboard attacking_bishops = board.pieces(BISHOP, attacker_color);
    const Bitboard attacking_rooks = board.pieces(ROOK, attacker_color);
    const Bitboard attacking_queens = board.pieces(QUEEN, attacker_color);
    const Bitboard attacking_knights = board.pieces(KNIGHT, attacker_color);
    const Bitboard attacking_king = board.pieces(KING, attacker_color);
    const Bitboard attacking_pawns = board.pieces(PAWN, attacker_color);

    const Bitboard inter_cardinal_rays = attacks::bishop(sq, occupied_bb);
    const Bitboard cardinal_rays_rays = attacks::rook(sq, occupied_bb);

    Bitboard attackers = inter_cardinal_rays & (attacking_bishops | attacking_queens);
    attackers |= cardinal_rays_rays & (attacking_rooks | attacking_queens);
    attackers |= attacks::knight(sq) & attacking_knights;
    attackers |= attacks::king(sq) & attacking_king;
    attackers |= attacks::pawn(sq, ~attacker_color) & attacking_pawns;
    return attackers;
}

[[nodiscard]] inline Bitboard allAttackers(const Board &board, Square sq, Bitboard occupied_bb) {
    return attackersForSide(board, WHITE, sq, occupied_bb) |
           attackersForSide(board, BLACK, sq, occupied_bb);
}

/********************
 * Static Exchange Evaluation, logical based on Weiss
 * (https://github.com/TerjeKir/weiss/blob/6772250059e82310c913a0d77a4c94b05d1e18e6/src/board.c#L310)
 * licensed under GPL-3.0
 *******************/
[[nodiscard]] inline bool see(const Board &board, Move move, int threshold) {
    Square from_sq = from(move);
    Square to_sq = to(move);
    auto attacker = board.at<PieceType>(from_sq);
    auto victim = board.at<PieceType>(to_sq);
    int swap = PIECE_VALUES_CLASSICAL[victim] - threshold;
    if (swap < 0) return false;
    swap -= PIECE_VALUES_CLASSICAL[attacker];
    if (swap >= 0) return true;
    Bitboard occ = (board.all() ^ (1ULL << from_sq)) | (1ULL << to_sq);
    Bitboard attackers = allAttackers(board, to_sq, occ) & occ;

    Bitboard queens = board.pieces(WHITEQUEEN) | board.pieces(BLACKQUEEN);

    Bitboard bishops = board.pieces(WHITEBISHOP) | board.pieces(BLACKBISHOP) | queens;
    Bitboard rooks = board.pieces(WHITEROOK) | board.pieces(BLACKROOK) | queens;

    Color sT = ~board.colorOf(from_sq);

    while (true) {
        attackers &= occ;
        Bitboard my_attackers = attackers & board.us(sT);
        if (!my_attackers) break;

        int pt;
        for (pt = 0; pt <= 5; pt++) {
            if (my_attackers &
                (board.pieces(static_cast<Piece>(pt)) | board.pieces(static_cast<Piece>(pt + 6))))
                break;
        }
        sT = ~sT;
        if ((swap = -swap - 1 - PIECE_VALUES_TUNED[0][pt]) >= 0) {
            if (pt == KING && (attackers & board.us(sT))) sT = ~sT;
            break;
        }

        occ ^= (1ULL << (builtin::lsb(my_attackers & (board.pieces(static_cast<Piece>(pt)) |
                                                      board.pieces(static_cast<Piece>(pt + 6))))));

        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= attacks::bishop(to_sq, occ) & bishops;
        if (pt == ROOK || pt == QUEEN) attackers |= attacks::rook(to_sq, occ) & rooks;
    }
    return sT != board.colorOf(from_sq);
}

}  // namespace see
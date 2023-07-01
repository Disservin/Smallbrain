#pragma once

#include "board.h"

namespace see {

[[nodiscard]] inline Bitboard attackersForSide(const Board &board, Color attacker_color, Square sq,
                                               Bitboard occupied_bb) {
    Bitboard attacking_bishops = board.pieces(BISHOP, attacker_color);
    Bitboard attacking_rooks = board.pieces(ROOK, attacker_color);
    Bitboard attacking_queens = board.pieces(QUEEN, attacker_color);
    Bitboard attacking_knights = board.pieces(KNIGHT, attacker_color);
    Bitboard attacking_king = board.pieces(KING, attacker_color);
    Bitboard attacking_pawns = board.pieces(PAWN, attacker_color);

    Bitboard inter_cardinal_rays = attacks::Bishop(sq, occupied_bb);
    Bitboard cardinal_rays_rays = attacks::Rook(sq, occupied_bb);

    Bitboard attackers = inter_cardinal_rays & (attacking_bishops | attacking_queens);
    attackers |= cardinal_rays_rays & (attacking_rooks | attacking_queens);
    attackers |= attacks::Knight(sq) & attacking_knights;
    attackers |= attacks::King(sq) & attacking_king;
    attackers |= attacks::Pawn(sq, ~attacker_color) & attacking_pawns;
    return attackers;
}

[[nodiscard]] inline Bitboard allAttackers(const Board &board, Square sq, Bitboard occupied_bb) {
    return attackersForSide(board, WHITE, sq, occupied_bb) |
           attackersForSide(board, BLACK, sq, occupied_bb);
}

/********************
 * Static Exchange Evaluation, logical based on Weiss (https://github.com/TerjeKir/weiss)
 *licensed under GPL-3.0
 *******************/
[[nodiscard]] inline bool see(const Board &board, Move move, int threshold) {
    Square from_sq = from(move);
    Square to_sq = to(move);
    PieceType attacker = board.at<PieceType>(from_sq);
    PieceType victim = board.at<PieceType>(to_sq);
    int swap = pieceValuesDefault[victim] - threshold;
    if (swap < 0) return false;
    swap -= pieceValuesDefault[attacker];
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
        if ((swap = -swap - 1 - piece_values[MG][pt]) >= 0) {
            if (pt == KING && (attackers & board.us(sT))) sT = ~sT;
            break;
        }

        occ ^= (1ULL << (builtin::lsb(my_attackers & (board.pieces(static_cast<Piece>(pt)) |
                                                      board.pieces(static_cast<Piece>(pt + 6))))));

        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= attacks::Bishop(to_sq, occ) & bishops;
        if (pt == ROOK || pt == QUEEN) attackers |= attacks::Rook(to_sq, occ) & rooks;
    }
    return sT != board.colorOf(from_sq);
}

}  // namespace see
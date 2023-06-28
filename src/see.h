#pragma once

#include "board.h"

namespace see {

inline Bitboard attackersForSide(const Board &board, Color attacker_color, Square sq,
                                 Bitboard occupied_bb) {
    Bitboard attackingBishops = board.pieces(BISHOP, attacker_color);
    Bitboard attackingRooks = board.pieces(ROOK, attacker_color);
    Bitboard attackingQueens = board.pieces(QUEEN, attacker_color);
    Bitboard attackingKnights = board.pieces(KNIGHT, attacker_color);
    Bitboard attackingKing = board.pieces(KING, attacker_color);
    Bitboard attackingPawns = board.pieces(PAWN, attacker_color);

    Bitboard interCardinalRays = attacks::Bishop(sq, occupied_bb);
    Bitboard cardinalRaysRays = attacks::Rook(sq, occupied_bb);

    Bitboard attackers = interCardinalRays & (attackingBishops | attackingQueens);
    attackers |= cardinalRaysRays & (attackingRooks | attackingQueens);
    attackers |= attacks::Knight(sq) & attackingKnights;
    attackers |= attacks::King(sq) & attackingKing;
    attackers |= attacks::Pawn(sq, ~attacker_color) & attackingPawns;
    return attackers;
}

inline Bitboard allAttackers(const Board &board, Square sq, Bitboard occupied_bb) {
    return attackersForSide(board, White, sq, occupied_bb) |
           attackersForSide(board, Black, sq, occupied_bb);
}

/********************
 * Static Exchange Evaluation, logical based on Weiss (https://github.com/TerjeKir/weiss)
 *licensed under GPL-3.0
 *******************/
inline bool see(const Board &board, Move move, int threshold) {
    Square from_sq = from(move);
    Square to_sq = to(move);
    PieceType attacker = typeOfPiece(board.at(from_sq));
    PieceType victim = typeOfPiece(board.at(to_sq));
    int swap = pieceValuesDefault[victim] - threshold;
    if (swap < 0) return false;
    swap -= pieceValuesDefault[attacker];
    if (swap >= 0) return true;
    Bitboard occ = (board.all() ^ (1ULL << from_sq)) | (1ULL << to_sq);
    Bitboard attackers = allAttackers(board, to_sq, occ) & occ;

    Bitboard queens = board.pieces(WhiteQueen) | board.pieces(BlackQueen);

    Bitboard bishops = board.pieces(WhiteBishop) | board.pieces(BlackBishop) | queens;
    Bitboard rooks = board.pieces(WhiteRook) | board.pieces(BlackRook) | queens;

    Color sT = ~board.colorOf(from_sq);

    while (true) {
        attackers &= occ;
        Bitboard myAttackers = attackers & board.us(sT);
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

}  // namespace see
#pragma once

#include "board.h"

namespace see {

inline U64 attackersForSide(const Board &board, Color attacker_color, Square sq, U64 occupied_bb) {
    U64 attackingBishops = board.pieces(BISHOP, attacker_color);
    U64 attackingRooks = board.pieces(ROOK, attacker_color);
    U64 attackingQueens = board.pieces(QUEEN, attacker_color);
    U64 attackingKnights = board.pieces(KNIGHT, attacker_color);
    U64 attackingKing = board.pieces(KING, attacker_color);
    U64 attackingPawns = board.pieces(PAWN, attacker_color);

    U64 interCardinalRays = attacks::Bishop(sq, occupied_bb);
    U64 cardinalRaysRays = attacks::Rook(sq, occupied_bb);

    U64 attackers = interCardinalRays & (attackingBishops | attackingQueens);
    attackers |= cardinalRaysRays & (attackingRooks | attackingQueens);
    attackers |= attacks::Knight(sq) & attackingKnights;
    attackers |= attacks::King(sq) & attackingKing;
    attackers |= attacks::Pawn(sq, ~attacker_color) & attackingPawns;
    return attackers;
}

inline U64 allAttackers(const Board &board, Square sq, U64 occupied_bb) {
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
    U64 occ = (board.all() ^ (1ULL << from_sq)) | (1ULL << to_sq);
    U64 attackers = allAttackers(board, to_sq, occ) & occ;

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

}  // namespace see
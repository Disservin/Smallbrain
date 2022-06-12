#include "evaluation.h"

// Values were taken from Stockfish https://github.com/official-stockfish/Stockfish/blob/master/src/psqt.cpp
// Released under GNU General Public License v3.0 https://github.com/official-stockfish/Stockfish/blob/master/Copying.txt

int evaluation(Board& board) {
	nnue.accumulate(board);
	return nnue.output() * (board.sideToMove * -2 + 1);
}

// int evaluation(Board& board) {
// 	int eval_mg = 0;
// 	int eval_eg = 0;
// 	int phase = 0;

// 	int wpawns  = popcount(board.Bitboards[WhitePawn]);
// 	int wknight = popcount(board.Bitboards[WhiteKnight]);
// 	int wbishop = popcount(board.Bitboards[WhiteBishop]);
// 	int wrook   = popcount(board.Bitboards[WhiteRook]);
// 	int wqueen  = popcount(board.Bitboards[WhiteQueen]);

// 	int bpawns  = popcount(board.Bitboards[BlackPawn]);
// 	int bknight = popcount(board.Bitboards[BlackKnight]);
// 	int bbishop = popcount(board.Bitboards[BlackBishop]);
// 	int brook   = popcount(board.Bitboards[BlackRook]);
// 	int bqueen  = popcount(board.Bitboards[BlackQueen]);

// 	phase += wknight + bknight;
// 	phase += wbishop + bbishop;
// 	phase += (wrook + brook) * 2;
// 	phase += (wqueen + bqueen) * 4;

// 	eval_mg += (wpawns - bpawns)  * piece_values[MG][PAWN]
// 		    + (wknight - bknight) * piece_values[MG][KNIGHT]
// 		    + (wbishop - bbishop) * piece_values[MG][BISHOP]
// 		    + (wrook - brook)     * piece_values[MG][ROOK]
// 		    + (wqueen - bqueen)   * piece_values[MG][QUEEN];

// 	eval_eg += (wpawns - bpawns)  * piece_values[EG][PAWN]
// 		    + (wknight - bknight) * piece_values[EG][KNIGHT]
// 		    + (wbishop - bbishop) * piece_values[EG][BISHOP]
// 		    + (wrook - brook)     * piece_values[EG][ROOK]
// 		    + (wqueen - bqueen)   * piece_values[EG][QUEEN];

// 	eval_mg += board.psqt_mg;
// 	eval_eg += board.psqt_eg;

// 	if (board.Pawns(White) & RANK_7) {
// 		eval_mg += 20;
// 		eval_eg += 30;
// 	}
// 	if (board.Pawns(Black) & RANK_2) {
// 		eval_mg -= 20;
// 		eval_eg -= 30;
// 	}

// 	phase = 24 - phase;
// 	phase = (phase * 256 + (24 / 2)) / 24;
// 	return (((eval_mg * (256 - phase)) + (eval_eg * phase)) / 256) * (board.sideToMove * -2 + 1);
// }
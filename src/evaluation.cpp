#include <map>

#include "evaluation.h"
#include "psqt.h"

// Values were taken from Stockfish https://github.com/official-stockfish/Stockfish/blob/master/src/psqt.cpp
// Released under GNU General Public License v3.0 https://github.com/official-stockfish/Stockfish/blob/master/Copying.txt
const int pawnValue = 100;
const int knightValue = 320;
const int bishopValue = 330;
const int rookValue = 500;
const int queenValue = 900;
constexpr int piece_values[2][6] = { { 126, 781, 825, 1276, 2538, 0}, { 208, 854, 915, 1380,  2682, 0} };

int evaluation(Board& board) {
	int eval_mg = 0;
	int eval_eg = 0;
	int phase = 0;

	int wpawns  = popcount(board.Bitboards[WhitePawn]);
	int wknight = popcount(board.Bitboards[WhiteKnight]);
	int wbishop = popcount(board.Bitboards[WhiteBishop]);
	int wrook   = popcount(board.Bitboards[WhiteRook]);
	int wqueen  = popcount(board.Bitboards[WhiteQueen]);

	int bpawns  = popcount(board.Bitboards[BlackPawn]);
	int bknight = popcount(board.Bitboards[BlackKnight]);
	int bbishop = popcount(board.Bitboards[BlackBishop]);
	int brook   = popcount(board.Bitboards[BlackRook]);
	int bqueen  = popcount(board.Bitboards[BlackQueen]);

	phase += wknight + bknight;
	phase += wbishop + bbishop;
	phase += (wrook + brook) * 2;
	phase += (wqueen + bqueen) * 4;

	eval_mg += (wpawns - bpawns)   * piece_values[0][0];
	eval_eg += (wpawns - bpawns)   * piece_values[1][0];

	eval_mg += (wknight - bknight) * piece_values[0][1];
	eval_eg += (wknight - bknight) * piece_values[1][1];

	eval_mg += (wbishop - bbishop) * piece_values[0][2];
	eval_eg += (wbishop - bbishop) * piece_values[1][2];

	eval_mg += (wrook - brook)     * piece_values[0][3];
	eval_eg += (wrook - brook) 	   * piece_values[1][3];

	eval_mg += (wqueen - bqueen)   * piece_values[0][4];
	eval_eg += (wqueen - bqueen)   * piece_values[1][4];

	eval_mg += board.psqt_mg;
	eval_eg += board.psqt_eg;

	if (board.Pawns(White) & RANK_7) {
		eval_mg += 20;
		eval_eg += 30;
	}
	if (board.Pawns(Black) & RANK_2) {
		eval_mg -= 20;
		eval_eg -= 30;
	}

	phase = 24 - phase;
	phase = (phase * 256 + (24 / 2)) / 24;
	return ((eval_mg * (256 - phase)) + (eval_eg * phase)) / 256;
}
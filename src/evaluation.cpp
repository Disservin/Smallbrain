#include <map>

#include "evaluation.h"
#include "psqt.h"

// Values were taken from Stockfish https://github.com/official-stockfish/Stockfish/blob/master/src/psqt.cpp
// Released under GNU General Public License v3.0 https://github.com/official-stockfish/Stockfish/blob/master/Copying.txt

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

	eval_mg +=   (wpawns  - bpawns)  * piece_values[0][PAWN] 
	           + (wknight - bknight) * piece_values[0][KNIGHT] 
			   + (wbishop - bbishop) * piece_values[0][BISHOP] 
			   + (wrook   - brook)   * piece_values[0][ROOK] 
			   + (wqueen  - bqueen)  * piece_values[0][QUEEN];

	eval_eg +=   (wpawns  - bpawns)  * piece_values[1][PAWN] 
	           + (wknight - bknight) * piece_values[1][KNIGHT]
			   + (wbishop - bbishop) * piece_values[1][BISHOP]
			   + (wrook   - brook)   * piece_values[1][ROOK]
			   + (wqueen  - bqueen)  * piece_values[1][QUEEN];

	eval_mg += board.psqt_mg;
	eval_eg += board.psqt_eg;

	Square WK = board.KingSQ(White);
	Square BK = board.KingSQ(Black);
	if (manhatten_distance(WK, BK) == 2) {
		if (board.sideToMove == White) {
			eval_mg -= 10;
			eval_eg -= 20;
		}
		if (board.sideToMove == Black) {
			eval_mg += 10;
			eval_eg += 20;
		}
	}

	eval_eg -= arrCenterManhattanDistance[WK];
	eval_eg += arrCenterManhattanDistance[BK];
	
	U64 WP = board.Bitboards[WhitePawn];
	U64 BP = board.Bitboards[BlackPawn];
	if (WP << 8 & BP) {
		eval_mg -= 10;
		eval_eg -= 20;
	}
	if (BP << 8 & WP) {
		eval_mg += 10;
		eval_eg += 20;
	}
	
	phase = 24 - phase;
	
	phase = (phase * 256 + (24 / 2)) / 24;
	return ((eval_mg * (256 - phase)) + (eval_eg * phase)) / 256;
}
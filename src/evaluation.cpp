#include "evaluation.h"

// Values were taken from Stockfish https://github.com/official-stockfish/Stockfish/blob/master/src/psqt.cpp
// Released under GNU General Public License v3.0 https://github.com/official-stockfish/Stockfish/blob/master/Copying.txt

int evaluation(Board& board) {
	return nnue.output() * (board.sideToMove * -2 + 1);
}

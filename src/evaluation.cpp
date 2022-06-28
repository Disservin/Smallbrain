#include "evaluation.h"

int evaluation(Board& board) {
	return nnue.output(board.accumulator) * (board.sideToMove * -2 + 1);
}
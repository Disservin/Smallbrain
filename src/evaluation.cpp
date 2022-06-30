#include "evaluation.h"

Score evaluation(Board& board) {
	return nnue.output(board.accumulator) * (board.sideToMove * -2 + 1);
}
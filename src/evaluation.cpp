#include "evaluation.h"

int evaluation(Board& board) {
	return nnue.output() * (board.sideToMove * -2 + 1);
}
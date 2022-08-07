#include "evaluation.h"

Score evaluation(Board &board)
{
    int32_t v = NNUE::output(board.accumulator) * (board.sideToMove * -2 + 1);
    Score score = std::clamp(v, (int32_t)(VALUE_MATED_IN_PLY + 1), (int32_t)(VALUE_MATE_IN_PLY - 1));
    return score;
}
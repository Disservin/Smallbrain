#include "evaluation.h"

#include <algorithm> // clamp

namespace Eval
{
Score evaluation(Board &board)
{
    int32_t v = NNUE::output(board.getAccumulator()) * (board.sideToMove * -2 + 1);

    v = static_cast<double>(v) * (1.0 - (board.halfMoveClock / 1000.0));
    Score score = std::clamp(static_cast<int>(v), (int32_t)(VALUE_MATED_IN_PLY + 1), (int32_t)(VALUE_MATE_IN_PLY - 1));
    return score;
}
} // namespace Eval
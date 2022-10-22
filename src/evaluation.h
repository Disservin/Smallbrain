#pragma once

#include "board.h"
#include "nnue.h"

namespace Eval
{
/// @brief nnue evaluation
/// @param board
/// @return
Score evaluation(Board &board);
} // namespace Eval
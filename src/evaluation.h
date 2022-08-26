#pragma once
#include <map>

#include "board.h"
#include "nnue.h"

namespace Eval
{
Score evaluation(Board &board);
} // namespace Eval
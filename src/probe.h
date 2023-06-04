#pragma once

#include "types.h"
#include "board.h"

namespace syzygy {
// check TB WDL during search
Score probeWDL(const Board& board);

/// @brief check DTZ at root node
/// @return return DTZ Move
Move probeDTZ(const Board& board);
}  // namespace syzygy

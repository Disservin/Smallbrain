#pragma once

#include "types.h"
#include "board.h"

namespace syzygy {
// check TB WDL during search
[[nodiscard]] Score probeWDL(const Board& board);

/// @brief check DTZ at root node
/// @return return DTZ Move
[[nodiscard]] Move probeDTZ(const Board& board);
}  // namespace syzygy

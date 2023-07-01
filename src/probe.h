#pragma once

#include "types.h"
#include "board.h"

namespace syzygy {
/// @brief check TB WDL during search
/// @param board
/// @return
[[nodiscard]] Score probeWDL(const Board& board);

/// @brief check DTZ at root node
/// @return return DTZ Move
[[nodiscard]] Move probeDTZ(const Board& board);
}  // namespace syzygy

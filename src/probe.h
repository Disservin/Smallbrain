#pragma once

#include "board.h"
#include "types.h"


namespace syzygy {
/// @brief check TB WDL during search
/// @param board
/// @return
[[nodiscard]] Score probeWDL(const Board& board);

/// @brief check DTZ at root node
/// @return return DTZ Move
[[nodiscard]] std::pair<int, Move> probeDTZ(const Board& board);
}  // namespace syzygy

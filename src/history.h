#pragma once

#include "search.h"

enum class HistoryType { HH, COUNTER, CONST };

namespace history {
// update all histories + other move ordering
void update(Search &search, Move bestmove, int depth, Move *quiets, int quiet_count, Stack *ss);

/// @brief return the history of the move
/// @tparam type
/// @param move
/// @return
template <HistoryType type, typename T = int16_t>
[[nodiscard]] T get(Move move, Move secondmove, const Search &search) {
    if constexpr (type == HistoryType::HH)
        return search.history[search.board.sideToMove()][move.from()][move.to()];
    else if constexpr (type == HistoryType::COUNTER)
        return search.counters[move.from()][move.to()];
    else if constexpr (type == HistoryType::CONST)
        return search.consthist[search.board.at(secondmove.from())][secondmove.to()]
                               [search.board.at(move.from())][move.to()];
}
}  // namespace history
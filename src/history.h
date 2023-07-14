#pragma once

#include "search.h"

enum class History {
    HH, COUNTER, CONST
};

// update history for one move
template<History type>
void updateHistoryBonus(Search &search, Move move, Move secondmove, int bonus);

// update history a movelist
template<History type>
void updateHistory(Search &search, Move bestmove, int bonus, int depth, const Move *moves, int move_count,
                   Stack *ss);

// update all histories + other move ordering
void updateAllHistories(Search &search, Move bestmove, int depth, Move *quiets, int quiet_count,
                        Stack *ss);

/// @brief return the history of the move
/// @tparam type
/// @param move
/// @return
template<History type>
[[nodiscard]] int getHistory(Move move, Move secondmove, const Search &search) {
    if constexpr (type == History::HH)
        return search.history[search.board.sideToMove()][from(move)][to(move)];
    else if constexpr (type == History::COUNTER)
        return search.counters[from(move)][to(move)];
    else if constexpr (type == History::CONST)
        return search.consthist[search.board.at(from(secondmove))][to(secondmove)]
        [search.board.at(from(move))][to(move)];
}
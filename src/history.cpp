#include "history.h"

int bonus(int depth) { return std::min(2000, depth * 155); }

template <History type>
void updateHistoryBonus(Search &search, Move move, Move secondmove, int bonus) {
    int hh_bonus = bonus - getHistory<type>(move, secondmove, search) * std::abs(bonus) / 16384;

    if constexpr (type == History::HH)
        search.history[search.board.sideToMove()][from(move)][to(move)] += hh_bonus;
    else if constexpr (type == History::CONST)
        search.consthist[search.board.at(from(secondmove))][to(secondmove)]
                        [search.board.at(from(move))][to(move)] += hh_bonus;
}

template <History type>
void updateHistory(Search &search, Move bestmove, int bonus, int depth, Move *moves, int move_count,
                   Stack *ss) {
    if constexpr (type == History::HH) {
        if (depth > 1) updateHistoryBonus<type>(search, bestmove, NO_MOVE, bonus);
    }

    if constexpr (type == History::CONST) {
        if (ss->ply > 0) {
            updateHistoryBonus<type>(search, bestmove, (ss - 1)->currentmove, bonus);
            if (ss->ply > 1)
                updateHistoryBonus<type>(search, bestmove, (ss - 2)->currentmove, bonus);
        }
    }

    for (int i = 0; i < move_count; i++) {
        const Move move = moves[i];

        if constexpr (type == History::CONST) {
            if (ss->ply > 0) {
                updateHistoryBonus<type>(search, move, (ss - 1)->currentmove, -bonus);
                if (ss->ply > 1)
                    updateHistoryBonus<type>(search, move, (ss - 2)->currentmove, -bonus);
            }
        } else
            updateHistoryBonus<type>(search, move, NO_MOVE, -bonus);
    }
}

void updateAllHistories(Search &search, Move bestmove, int depth, Move *quiets, int quiet_count,
                        Stack *ss) {
    int depth_bonus = bonus(depth);

    search.counters[from((ss - 1)->currentmove)][to((ss - 1)->currentmove)] = bestmove;

    /********************
     * Update Quiet Moves
     *******************/
    if (search.board.at(to(bestmove)) == NONE) {
        // update Killer Moves
        search.killers[1][ss->ply] = search.killers[0][ss->ply];
        search.killers[0][ss->ply] = bestmove;

        updateHistory<History::HH>(search, bestmove, depth_bonus, depth, quiets, quiet_count, ss);

        int constbonus = std::min(4 * depth * depth * depth, 1500);
        updateHistory<History::CONST>(search, bestmove, constbonus, depth, quiets, quiet_count, ss);
    }
}
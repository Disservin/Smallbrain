#include "history.h"

namespace history {
int bonus(int depth) { return std::min(2000, depth * 155); }

template <HistoryType type>
void updateBonus(Search &search, Move move, Move secondmove, int bonus) {
    int hh_bonus = bonus - get<type>(move, secondmove, search) * std::abs(bonus) / 16384;

    if constexpr (type == HistoryType::HH)
        search.history[search.board.sideToMove()][from(move)][to(move)] += hh_bonus;
    else if constexpr (type == HistoryType::CONST)
        search.consthist[search.board.at(from(secondmove))][to(secondmove)]
                        [search.board.at(from(move))][to(move)] += hh_bonus;
}

template <HistoryType type>
void updateSingle(Search &search, Move bestmove, int bonus, int depth, const Move *moves,
                  int move_count, Stack *ss) {
    if constexpr (type == HistoryType::HH) {
        if (depth > 1) updateBonus<type>(search, bestmove, NO_MOVE, bonus);
    }

    if constexpr (type == HistoryType::CONST) {
        if (ss->ply > 0) {
            updateBonus<type>(search, bestmove, (ss - 1)->currentmove, bonus);
            if (ss->ply > 1) updateBonus<type>(search, bestmove, (ss - 2)->currentmove, bonus);
        }
    }

    for (int i = 0; i < move_count; i++) {
        const Move move = moves[i];

        if constexpr (type == HistoryType::CONST) {
            if (ss->ply > 0) {
                updateBonus<type>(search, move, (ss - 1)->currentmove, -bonus);
                if (ss->ply > 1) updateBonus<type>(search, move, (ss - 2)->currentmove, -bonus);
            }
        } else
            updateBonus<type>(search, move, NO_MOVE, -bonus);
    }
}

void update(Search &search, Move bestmove, int depth, Move *quiets, int quiet_count, Stack *ss) {
    int depth_bonus = bonus(depth);

    search.counters[from((ss - 1)->currentmove)][to((ss - 1)->currentmove)] = bestmove;

    /********************
     * Update Quiet Moves
     *******************/
    if (search.board.at(to(bestmove)) == NONE) {
        // update Killer Moves
        search.killers[1][ss->ply] = search.killers[0][ss->ply];
        search.killers[0][ss->ply] = bestmove;

        updateSingle<HistoryType::HH>(search, bestmove, depth_bonus, depth, quiets, quiet_count,
                                      ss);

        int constbonus = std::min(4 * depth * depth * depth, 1500);
        updateSingle<HistoryType::CONST>(search, bestmove, constbonus, depth, quiets, quiet_count,
                                         ss);
    }
}
}  // namespace history
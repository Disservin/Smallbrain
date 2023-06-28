#pragma once

#include <thread>

#include "board.h"
#include "movegen.h"
#include "timemanager.h"
#include "types/table.h"

struct Stack {
    int eval;
    Move currentmove;
    Move excludedMove;
    uint16_t ply;
};

enum class History { HH, COUNTER, CONST };

class Search {
   public:
    void startThinking();

    // data generation entry function
    std::pair<Move, Score> iterativeDeepening();

    void reset();

    Board board = Board();

    Table<int16_t, N_PIECES + 1, 64, N_PIECES + 1, 64> consthist;

    // history heuristic for quiet move ordering
    Table<int16_t, 2, MAX_SQ, MAX_SQ> history = {};

    // Counter moves for quiet move ordering
    Table<Move, MAX_SQ, MAX_SQ> counters = {};

    // node count logic
    Table<U64, MAX_SQ, MAX_SQ> spent_effort = {};

    // Killer moves for quiet move ordering
    Table<Move, 2, MAX_PLY + 1> killer_moves = {};

    // GUI might send
    // go searchmoves e2e4
    // in which case the root movelist should only include this move
    Movelist searchmoves = {};

    // Limits parsed from UCI
    Limits limit = {};

    // nodes searched
    U64 nodes = 0;
    U64 tbhits = 0;

    // thread id, Mainthread = 0
    int id = 0;

    // data generation is not allowed to print to the console
    // and sets this to false
    bool silent = false;

    bool use_tb = false;

   private:
    // update history for one move
    template <History type>
    void updateHistoryBonus(Move move, Move secondmove, int bonus);

    // update history a movelist
    template <History type>
    void updateHistory(Move bestmove, int bonus, int depth, Move *moves, int move_count, Stack *ss);

    // update all histories + other move ordering
    void updateAllHistories(Move bestmove, int depth, Move *quiets, int quiet_count, Stack *ss);

    // main search functions

    template <Node node>
    [[nodiscard]] Score qsearch(Score alpha, Score beta, Stack *ss);

    template <Node node>
    [[nodiscard]] Score absearch(int depth, Score alpha, Score beta, Stack *ss);

    [[nodiscard]] Score aspirationSearch(int depth, Score prev_eval, Stack *ss);

    // check limits
    [[nodiscard]] bool limitReached();

    [[nodiscard]] std::string getPV() const;
    [[nodiscard]] int64_t getTime() const;

    // pv collection
    Table<uint8_t, MAX_PLY> pv_length_ = {};
    Table<Move, MAX_PLY, MAX_PLY> pv_table_ = {};

    // timepoint when we entered search
    TimePoint::time_point t0_;

    // time will be check if == 0
    int check_time_ = 0;

    // selective depth
    uint8_t seldepth_ = 0;
};

/// @brief return the history of the move
/// @tparam type
/// @param move
/// @return
template <History type>
[[nodiscard]] int getHistory(Move move, Move secondmove, const Search &search) {
    if constexpr (type == History::HH)
        return search.history[search.board.side_to_move][from(move)][to(move)];
    else if constexpr (type == History::COUNTER)
        return search.counters[from(move)][to(move)];
    else if constexpr (type == History::CONST)
        return search.consthist[search.board.at(from(secondmove))][to(secondmove)]
                               [search.board.at(from(move))][to(move)];
}

// fill reductions array
void init_reductions();

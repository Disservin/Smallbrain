#pragma once

#include <thread>

#include "board.h"
#include "movegen.h"
#include "timemanager.h"
#include "types/table.h"

struct Stack {
    int eval;
    int move_count;
    Move currentmove;
    Move excluded_move;
    uint16_t ply;
};

struct SearchResult {
    Move bestmove = NO_MOVE;
    Score score = -VALUE_INFINITE;
};

class Search {
   public:
    void startThinking();

    // data generation entry function
    SearchResult iterativeDeepening();

    void reset();

    Board board = Board();

    Table<int16_t, N_PIECES + 1, 64, N_PIECES + 1, 64> consthist;

    // history heuristic for quiet move ordering
    Table<int16_t, 2, MAX_SQ, MAX_SQ> history = {};

    // Counter moves for quiet move ordering
    Table<Move, MAX_SQ, MAX_SQ> counters = {};

    // node count logic
    Table<U64, MAX_SQ, MAX_SQ> node_effort = {};

    // Killer moves for quiet move ordering
    Table<Move, 2, MAX_PLY + 1> killers = {};

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
    Table<uint8_t, MAX_PLY + 1> pv_length_ = {};
    Table<Move, MAX_PLY + 1, MAX_PLY + 1> pv_table_ = {};

    // timepoint when we entered search
    TimePoint::time_point t0_;

    // time will be check if == 0
    int check_time_ = 0;

    // selective depth
    uint8_t seldepth_ = 0;
};

// fill reductions array
void init_reductions();

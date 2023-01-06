#pragma once

#include <thread>

#include "syzygy/Fathom/src/tbprobe.h"

#include "board.h"
#include "movegen.h"
#include "timemanager.h"

extern std::atomic_bool stopped;

using historyTable = std::array<std::array<std::array<int, MAX_SQ>, MAX_SQ>, 2>;
using killerTable = std::array<std::array<Move, MAX_PLY + 1>, 2>;
using nodeTable = std::array<std::array<U64, MAX_SQ>, MAX_SQ>;

struct Stack
{
    Movelist moves;
    Movelist quietMoves;
    int eval;
    Move currentmove;
    uint16_t ply;
};

struct SearchResult
{
    Move move;
    Score score;
};

class Search
{
  public:
    Board board = Board();

    // thread id, Mainthread = 0
    int id = 0;

    bool useTB = 0;

    // [sideToMove][from][to]
    historyTable history = {};

    // [sideToMove][ply]
    killerTable killerMoves = {};

    // node count logic
    nodeTable spentEffort = {};

    // pv collection
    std::array<uint8_t, MAX_PLY> pvLength = {};
    std::array<std::array<Move, MAX_PLY>, MAX_PLY> pvTable = {};

    // selective depth
    uint8_t seldepth = 0;

    // data generation is not allowed to print to the console
    bool allowPrint = true;

    // Mainthread limits
    Limits limit = {};

    int checkTime = 0;

    // timepoint when we entered search
    TimePoint::time_point t0 = TimePoint::now();

    // nodes searched
    uint64_t nodes = 0;
    uint64_t tbhits = 0;

    void startThinking();

    // data generation entry function
    SearchResult iterativeDeepening();

  private:
    /// @brief update move history
    /// @tparam type
    /// @param move
    /// @param bonus
    template <Movetype type> void updateHistoryBonus(Move move, int bonus);

    /// @brief update history for all moves
    /// @tparam type
    /// @param bestmove
    /// @param bonus
    /// @param depth
    /// @param movelist movelist of moves to update
    template <Movetype type> void updateHistory(Move bestmove, int bonus, int depth, Movelist &movelist);

    /// @brief  update all history + other move ordering
    /// @param bestMove
    /// @param best best score
    /// @param beta
    /// @param depth
    /// @param quietMoves
    /// @param ss
    void updateAllHistories(Move bestMove, Score best, Score beta, int depth, Movelist &quietMoves, Stack *ss);

    // main search functions

    template <Node node> Score qsearch(Score alpha, Score beta, Stack *ss);
    template <Node node> Score absearch(int depth, Score alpha, Score beta, Stack *ss);
    Score aspirationSearch(int depth, Score prev_eval, Stack *ss);

    // check limits
    bool exitEarly();

    std::string getPV();
    long long getTime();

    /// @brief check TB evaluation
    Score probeTB();

    /// @brief find the DTZ Move
    /// @param b Board
    /// @return return DTZ Move
    Move probeDTZ(Board &board);
};

/// @brief return the history of the move
/// @tparam type
/// @param move
/// @return
template <Movetype type> int getHistory(Move move, Search &search)
{
    if constexpr (type == Movetype::QUIET)
        return search.history[search.board.sideToMove][from(move)][to(move)];
}

static constexpr int mvvlvaArray[8][8] = {{0, 0, 0, 0, 0, 0, 0, 0},
                                          {0, 205, 204, 203, 202, 201, 200, 0},
                                          {0, 305, 304, 303, 302, 301, 300, 0},
                                          {0, 405, 404, 403, 402, 401, 400, 0},
                                          {0, 505, 504, 503, 502, 501, 500, 0},
                                          {0, 605, 604, 603, 602, 601, 600, 0},
                                          {0, 705, 704, 703, 702, 701, 700, 0}};

// fill reductions array
void init_reductions();

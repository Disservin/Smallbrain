#pragma once

#include <thread>

#include "syzygy/Fathom/src/tbprobe.h"

#include "board.h"
#include "movegen.h"
#include "timemanager.h"

template <typename T, size_t N, size_t... Dims> struct Table
{
    std::array<Table<T, Dims...>, N> data;
    Table()
    {
        data.fill({});
    }
    Table<T, Dims...> &operator[](size_t index)
    {
        return data[index];
    }
    const Table<T, Dims...> &operator[](size_t index) const
    {
        return data[index];
    }

    void reset()
    {
        data.fill({});
    }
};

template <typename T, size_t N> struct Table<T, N>
{
    std::array<T, N> data;
    Table()
    {
        data.fill({});
    }
    T &operator[](size_t index)
    {
        return data[index];
    }
    const T &operator[](size_t index) const
    {
        return data[index];
    }
};

struct Stack
{
    int eval;
    Move currentmove;
    Move excludedMove;
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
    // timepoint when we entered search
    TimePoint::time_point t0;

    Board board = Board();

    // Counter moves for quiet move ordering
    Table<Move, MAX_SQ, MAX_SQ> counters = {};

    // history heuristic for quiet move ordering
    Table<int16_t, 2, MAX_SQ, MAX_SQ> history = {};

    // Killer moves for quiet move ordering
    Table<Move, 2, MAX_PLY + 1> killerMoves = {};

    // node count logic
    Table<U64, MAX_SQ, MAX_SQ> spentEffort = {};

    // pv collection
    Table<uint8_t, MAX_PLY> pvLength = {};
    Table<Move, MAX_PLY, MAX_PLY> pvTable = {};

    Movelist searchmoves = {};

    // Mainthread limits
    Limits limit = {};

    // nodes searched
    uint64_t nodes = 0;
    uint64_t tbhits = 0;

    // thread id, Mainthread = 0
    int id = 0;

    // time will be check if == 0
    int checkTime = 0;

    // selective depth
    uint8_t seldepth = 0;

    // data generation is not allowed to print to the console
    // and sets this to false
    bool normalSearch = true;

    bool useTB = false;

    void startThinking();

    // data generation entry function
    SearchResult iterativeDeepening();

  private:
    // update move history
    template <Movetype type> void updateHistoryBonus(Move move, int bonus);

    /// @brief update history for all moves
    /// @tparam type
    /// @param bestmove
    /// @param bonus
    /// @param depth
    /// @param movelist movelist of moves to update
    template <Movetype type> void updateHistory(Move bestmove, int bonus, int depth, Move *quiets, int quietCount);

    // update all history + other move ordering
    void updateAllHistories(Move prevMove, Move bestMove, int depth, Move *quiets, int quietCount, Stack *ss);

    // main search functions

    template <Node node> Score qsearch(Score alpha, Score beta, Stack *ss);
    template <Node node> Score absearch(int depth, Score alpha, Score beta, Stack *ss);
    Score aspirationSearch(int depth, Score prevEval, Stack *ss);

    // check limits
    bool limitReached();

    std::string getPV();
    int64_t getTime();

    // check TB WDL during search
    Score probeWDL();

    /// @brief check DTZ at root node
    /// @return return DTZ Move
    Move probeDTZ();
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

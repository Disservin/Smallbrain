#pragma once

#include <thread>

#include "syzygy/Fathom/src/tbprobe.h"

#include "attacks.h"
#include "board.h"
#include "evaluation.h"
#include "movegen.h"
#include "timemanager.h"
#include "tt.h"

extern std::atomic<bool> stopped;
extern std::atomic<bool> useTB;
extern TEntry *TTable;
extern U64 TT_SIZE;

using historyTable = std::array<std::array<std::array<int, MAX_SQ>, MAX_SQ>, 2>;
using killerTable = std::array<std::array<Move, MAX_PLY + 1>, 2>;
using nodeTable = std::array<std::array<U64, MAX_SQ>, MAX_SQ>;

struct Movepicker
{
    int ttMoveIndex = -1;
    int i = 0;
    Staging stage = TT_MOVE;
};

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

struct ThreadData
{
    Board board;

    // thread id, Mainthread = 0
    int id;

    // [sideToMove][from][to]
    historyTable history = {};

    // [sideToMove][ply]
    killerTable killerMoves = {};

    // pv collection
    uint8_t pvLength[MAX_PLY]{};
    Move pvTable[MAX_PLY][MAX_PLY]{};

    // selective depth
    uint8_t seldepth{};

    // nodes searched
    uint64_t nodes{};
    uint64_t tbhits{};

    // data generation is not allowed to print to the console
    bool allowPrint = true;
};

class Search
{
  public:
    // data generation entry function
    SearchResult iterativeDeepening(int searchDepth, uint64_t maxN, Time time, int threadId);

    // search entry function
    void startThinking(Board board, int workers, int searchDepth, uint64_t maxN, Time time);

    std::vector<ThreadData> tds;
    std::vector<std::thread> threads;

  private:
    // Mainthread limits
    uint64_t maxNodes{};
    int64_t optimumTime{};
    int64_t maxTime{};
    int checkTime;

    // node count logic
    nodeTable spentEffort;

    // timepoint when we entered search
    TimePoint::time_point t0;

    /// @brief return the history of the move
    /// @tparam type
    /// @param move
    /// @param td
    /// @return
    template <Movetype type> int getHistory(Move move, ThreadData *td);

    /// @brief update move history
    /// @tparam type
    /// @param move
    /// @param bonus
    /// @param td
    template <Movetype type> void updateHistoryBonus(Move move, int bonus, ThreadData *td);

    /// @brief update history for all moves
    /// @tparam type
    /// @param bestmove
    /// @param bonus
    /// @param depth
    /// @param movelist movelist of moves to update
    /// @param td
    template <Movetype type>
    void updateHistory(Move bestmove, int bonus, int depth, Movelist &movelist, ThreadData *td);

    /// @brief  update all history + other move ordering
    /// @param bestMove
    /// @param best best score
    /// @param beta
    /// @param depth
    /// @param quietMoves
    /// @param td
    /// @param ss
    void updateAllHistories(Move bestMove, Score best, Score beta, int depth, Movelist &quietMoves, ThreadData *td,
                            Stack *ss);

    // main search functions

    template <Node node> Score qsearch(Score alpha, Score beta, int depth, Stack *ss, ThreadData *td);
    template <Node node> Score absearch(int depth, Score alpha, Score beta, Stack *ss, ThreadData *td);
    Score aspirationSearch(int depth, Score prev_eval, Stack *ss, ThreadData *td);

    /// @brief Static Exchange Evaluation
    /// @param move
    /// @param threshold
    /// @param board
    /// @return
    bool see(Move move, int threshold, Board &board);

    /// @brief Most Valuable Victim - Least Valuable Aggressor
    /// @param move
    /// @param board
    /// @return
    int mvvlva(Move move, Board &board);

    /// @brief score moves from qsearch
    /// @param move
    /// @param ply
    /// @param ttMove
    /// @param td
    /// @return
    int scoreqMove(Move move, int ply, Move ttMove, ThreadData *td);

    /// @brief score main search moves
    /// @param move
    /// @param ply
    /// @param ttMove
    /// @param td
    /// @return
    int scoreMove(Move move, int ply, Move ttMove, ThreadData *td);

    // utility functions

    /// @brief requires a clean Movepicker to start with
    /// @param moves legal movelist
    /// @param mp Movepicker object
    /// @param ttHit
    /// @param td ThreadData
    /// @param ss Stack
    /// @return next move
    Move nextMove(Movelist &moves, Movepicker &mp, Move ttMove, ThreadData *td, Stack *ss);

    // sorts the next bestmove to the front
    void sortMoves(Movelist &moves, int sorted);

    // check limits
    bool exitEarly(uint64_t nodes, int ThreadId);

    std::string getPV();
    long long getTime();
    uint64_t getNodes();
    uint64_t getTbHits();

    /// @brief check TB evaluation
    /// @param td ThreadData
    /// @return TB Score
    Score probeTB(ThreadData *td);

    /// @brief find the DTZ Move
    /// @param td ThreadData
    /// @return return DTZ Move
    Move probeDTZ(ThreadData *td);
};

static constexpr int mvvlvaArray[7][7] = {{0, 0, 0, 0, 0, 0, 0},
                                          {0, 205, 204, 203, 202, 201, 200},
                                          {0, 305, 304, 303, 302, 301, 300},
                                          {0, 405, 404, 403, 402, 401, 400},
                                          {0, 505, 504, 503, 502, 501, 500},
                                          {0, 605, 604, 603, 602, 601, 600},
                                          {0, 705, 704, 703, 702, 701, 700}};

static constexpr int piece_values[2][7] = {{98, 337, 365, 477, 1025, 0, 0}, {114, 281, 297, 512, 936, 0, 0}};
static constexpr int pieceValuesDefault[7] = {100, 320, 330, 500, 900, 0, 0};

// fill reductions array
void init_reductions();

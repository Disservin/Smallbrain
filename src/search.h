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

enum Movetype : uint8_t
{
    QUIET,
    CAPTURE
};

enum Node
{
    NonPV,
    PV,
    Root
};

struct Stack
{
    uint16_t currentmove;
    uint16_t ply;
    int eval;
};

enum Staging
{
    TT_MOVE,
    EVAL_OTHER,
    OTHER
};

inline void operator++(Staging &s, int)
{
    s = Staging(static_cast<int>(s) + 1);
}

struct Movepicker
{
    int ttMoveIndex;
    int i;
    Staging stage;
};

struct ThreadData
{
    Board board;

    // thread id, Mainthread = 0
    int id;

    // [sideToMove][from][to]
    int historyTable[2][MAX_SQ][MAX_SQ]{};

    // [sideToMove][ply]
    Move killerMoves[2][MAX_PLY + 1]{};

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

struct SearchResult
{
    Move move;
    Score score;
};

class Search
{
  public:
    // data generation entry function
    SearchResult iterativeDeepening(int search_depth, uint64_t maxN, Time time, int threadId);

    // search entry function
    void startThinking(Board board, int workers, int search_depth, uint64_t maxN, Time time);

    std::vector<ThreadData> tds;
    std::vector<std::thread> threads;

  private:
    // Mainthread limits
    uint64_t maxNodes{};
    int64_t searchTime{};
    int64_t maxTime{};
    int checkTime;

    // node count logic
    U64 spentEffort[MAX_SQ][MAX_SQ];

    // number of legal moves at the root
    int rootSize;

    // timepoint when we entered search
    TimePoint::time_point t0;

    // return the history of the move
    template <Movetype type> int getHistory(Move move, ThreadData *td);

    // update move history
    template <Movetype type> void updateHistoryBonus(Move move, int bonus, ThreadData *td);

    // update history for all moves
    template <Movetype type>
    void updateHistory(Move bestmove, int bonus, int depth, Movelist &movelist, ThreadData *td);

    // update all history + other move ordering
    void updateAllHistories(Move bestMove, Score best, Score beta, int depth, Movelist &quietMoves, ThreadData *td,
                            Stack *ss);

    // main search functions

    template <Node node> Score qsearch(Score alpha, Score beta, Stack *ss, ThreadData *td);
    template <Node node> Score absearch(int depth, Score alpha, Score beta, Stack *ss, ThreadData *td);
    Score aspirationSearch(int depth, Score prev_eval, Stack *ss, ThreadData *td);

    // Static Exchange Evaluation
    bool see(Move move, int threshold, Board &board);

    // Most Valuable Victim - Least Valuable Aggressor
    int mvvlva(Move move, Board &board);

    // score moves from qsearch
    int scoreqMove(Move move, int ply, bool ttMove, ThreadData *td);
    // score main search moves
    int scoreMove(Move move, int ply, bool ttMove, ThreadData *td);

    // utility functions

    /// @brief requires a clean Movepicker to start with
    /// @param moves
    /// @param mp
    /// @param ttHit
    /// @param td
    /// @param ss
    /// @return next move
    Move nextMove(Movelist &moves, Movepicker &mp, bool ttHit, ThreadData *td, Stack *ss);

    // sorts the next bestmove to the front
    void sortMoves(Movelist &moves, int sorted);

    // check limits
    bool exitEarly(uint64_t nodes, int ThreadId);

    std::string getPV();
    long long getTime();
    uint64_t getNodes();
    uint64_t getTbHits();

    Score probeTB(ThreadData *td);
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
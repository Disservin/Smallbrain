#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

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

struct Movepicker
{
    Staging stage;
    int i;
    int ttMoveIndex;
};

inline void operator++(Staging &s, int)
{
    s = Staging((int)s + 1);
}

struct ThreadData
{
    Board board;

    int id;
    // move ordering
    // [sideToMove][from][to]
    int historyTable[2][MAX_SQ][MAX_SQ]{};

    Move killerMoves[2][MAX_PLY + 1]{};

    // pv collection
    uint8_t pvLength[MAX_PLY]{};
    Move pvTable[MAX_PLY][MAX_PLY]{};

    // selective depth
    uint8_t seldepth{};

    // nodes searched
    uint64_t nodes{};
    uint64_t tbhits{};

    bool allowPrint = true;
};

struct SearchResult
{
    Move move;
    Score score;
};

static constexpr int mvvlva[7][7] = {{0, 0, 0, 0, 0, 0, 0},
                                     {0, 205, 204, 203, 202, 201, 200},
                                     {0, 305, 304, 303, 302, 301, 300},
                                     {0, 405, 404, 403, 402, 401, 400},
                                     {0, 505, 504, 503, 502, 501, 500},
                                     {0, 605, 604, 603, 602, 601, 600},
                                     {0, 705, 704, 703, 702, 701, 700}};

class Search
{
  public:
    // Mainthread limits
    uint64_t maxNodes{};
    int64_t searchTime{};
    int64_t maxTime{};
    int checkTime;

    U64 spentEffort[MAX_SQ][MAX_SQ];
    int rootSize;
    TimePoint::time_point t0;

    std::vector<ThreadData> tds;
    std::vector<std::thread> threads;

    // move ordering

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
    SearchResult iterativeDeepening(int search_depth, uint64_t maxN, Time time, int threadId);

    // search entry function
    void startThinking(Board board, int workers, int search_depth, uint64_t maxN, Time time);

    // capture functions

    bool see(Move move, int threshold, Board &board);
    int mmlva(Move move, Board &board);

    // scoring functions

    int scoreqMove(Move move, int ply, bool ttMove, ThreadData *td);
    int scoreMove(Move move, int ply, bool ttMove, ThreadData *td);

    // utility functions
    Move Nextmove(Movelist &moves, Movepicker &mp, bool ttHit, ThreadData *td, Stack *ss);
    void sortMoves(Movelist &moves, int sorted);

    bool exitEarly(uint64_t nodes, int ThreadId);

    std::string getPV();
    long long getTime();
    uint64_t getNodes();
    uint64_t getTbHits();

    Score probeTB(ThreadData *td);
    Move probeDTZ(ThreadData *td);
};

std::string outputScore(int score, Score beta);
void uciOutput(int score, int depth, uint8_t seldepth, U64 nodes, U64 tbHits, int time, std::string pv);
#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "board.h"
#include "evaluation.h"
#include "timemanager.h"
#include "tt.h"

extern std::atomic<bool> stopped;
extern TEntry *TTable;
extern U64 TT_SIZE;

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

struct ThreadData
{
    Board board;

    int id;
    // move ordering
    // [sideToMove][from][to]
    int history_table[2][MAX_SQ][MAX_SQ]{};

    Move killerMoves[2][MAX_PLY + 1]{};

    // pv collection
    uint8_t pv_length[MAX_PLY]{};
    Move pv_table[MAX_PLY][MAX_PLY]{};

    // selective depth
    uint8_t seldepth{};

    // nodes searched
    uint64_t nodes{};

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
    int getHistory(Move move, ThreadData *td);
    void updateHistory(Move bestmove, int bonus, int depth, Movelist &movelist, ThreadData *td);
    void updateHistoryBonus(Move move, int b, ThreadData *td);
    void UpdateHH(Move bestMove, Score best, Score beta, int depth, Movelist &quietMoves, ThreadData *td);

    // main search functions
    template <Node node> Score qsearch(Score alpha, Score beta, Stack *ss, ThreadData *td);
    template <Node node> Score absearch(int depth, Score alpha, Score beta, Stack *ss, ThreadData *td);
    Score aspirationSearch(int depth, Score prev_eval, Stack *ss, ThreadData *td);
    SearchResult iterativeDeepening(int search_depth, uint64_t maxN, Time time, int threadId);
    void startThinking(Board board, int workers, int search_depth, uint64_t maxN, Time time);

    // capture functions
    bool see(Move &move, int threshold, Board &board);
    int mmlva(Move &move, Board &board);

    // scoring functions
    int scoreMove(Move &move, int ply, bool ttMove, ThreadData *td);
    int scoreQmove(Move &move, Board &board);

    // utility functions
    std::string get_pv();
    long long elapsed();
    bool exitEarly(uint64_t nodes, int ThreadId);
    uint64_t getNodes();

    void sortMoves(Movelist &moves, int sorted);
};

std::string outputScore(int score, Score alpha, Score beta);
void uciOutput(int score, Score alpha, Score beta, int depth, uint8_t seldepth, U64 nodes, int time, std::string pv);
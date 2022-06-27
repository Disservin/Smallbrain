#pragma once
#include <atomic>
#include <chrono>
#include <algorithm>

#include "board.h"
#include "tt.h"
#include "timemanager.h"

extern std::atomic<bool> stopped;
extern TEntry* TTable;
extern U64 TT_SIZE;

struct Stack{
    uint16_t currentmove;
    uint16_t ply;
    int eval;
};

static constexpr int mvvlva[7][7] = { {0, 0, 0, 0, 0, 0, 0},
                                    {0, 205, 204, 203, 202, 201, 200},
                                    {0, 305, 304, 303, 302, 301, 300},
                                    {0, 405, 404, 403, 402, 401, 400},
                                    {0, 505, 504, 503, 502, 501, 500},
                                    {0, 605, 604, 603, 602, 601, 600},
                                    {0, 705, 704, 703, 702, 701, 700} };

class Search {
public:
    // limits
    uint64_t maxNodes{};
    int64_t searchTime{};
    int64_t maxTime{};

    // nodes
    uint64_t nodes{};
    
    // pv collection
    uint8_t pv_length[MAX_PLY]{};
    Move pv_table[MAX_PLY][MAX_PLY]{};
    Move pv[MAX_PLY]{};

    // current board
    Board board{};

    // bestmove
    Move bestMove{};

    // startAge and seldepth
    uint16_t startAge{};
    uint8_t seldepth{};

    // move ordering and time usage
    int history_table[2][MAX_SQ][MAX_SQ]{};
    Move killerMoves[2][MAX_PLY + 1]{};
    U64 spentEffort[MAX_SQ][MAX_SQ];
    int rootSize;
    TimePoint::time_point t0;

    // constructor
    Search(Board brd) {
        board = brd;
    }

    // testing
    U64 perft(int depth, int max);
    void perf_Test(int depth, int max);
    void testAllPos();
    
    // move ordering
    void UpdateHH(Move bestMove, int depth, Movelist quietMoves);

    // main search functions
    int qsearch(int depth, int alpha, int beta, int ply);
    int absearch(int depth, int alpha, int beta, int ply, Stack *ss);
    int aspiration_search(int depth, int prev_eval, Stack *ss);
    int iterative_deepening(int search_depth, uint64_t maxN, Time time);
    
    // capture functions
    bool see(Move& move, int threshold);
    int mmlva(Move& move);

    // scoring functions
    int score_move(Move& move, int ply, bool ttMove);
    int score_qmove(Move& move);
    
    // utility functions
    std::string get_pv();
    long long elapsed();
    bool exit_early();

    void sortMoves(Movelist& moves);
    void sortMoves(Movelist& moves, int sorted);
};

std::string output_score(int score);
void uci_output(int score, int depth, uint8_t seldepth, U64 nodes, int time, std::string pv);
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

class Search {
public:
    uint64_t nodes{};
    uint64_t maxNodes{};
    Board board{};
    Move bestMove{};
    uint16_t startAge{};
    int64_t searchTime{};
    int64_t maxTime{};
    uint8_t seldepth{};
    uint8_t pv_length[MAX_PLY]{};
    Move pv_table[MAX_PLY][MAX_PLY]{};
    Move pv[MAX_PLY]{};
    int history_table[2][6][MAX_SQ][MAX_SQ]{};
    Move killerMoves[2][MAX_PLY + 1]{};
    U64 spentEffort[MAX_SQ][MAX_SQ]{};
    int rootSize{};
    std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();

    Search(Board brd) {
        board = brd;
        t0 = std::chrono::high_resolution_clock::now();
    }
    void perf_Test(int depth, int max);
    void testAllPos();
    U64 perft(int depth, int max);
    void UpdateHH(Move bestMove, int depth, Movelist quietMoves);
    int qsearch(int depth, int alpha, int beta, int ply);
    int absearch(int depth, int alpha, int beta, int ply, Stack *ss);
    int aspiration_search(int depth, int prev_eval, Stack *ss);
    int iterative_deepening(int search_depth, uint64_t maxN, Time time);
    bool exit_early();
    int mmlva(Move& move);
    int score_move(Move& move, int ply, bool ttMove);
    std::string get_pv();
    bool store_entry(U64 index, int depth, int bestvalue, int old_alpha, int beta, U64 key, uint8_t ply);
    void uci_output(int score, int depth, int time);
    long long elapsed();
};
void sortMoves(Movelist& moves);
void sortMoves(Movelist& moves, int sorted);
std::string output_score(int score);

inline bool operator==(Move& m, Move& m2);

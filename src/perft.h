#pragma once

#include "board.h"
#include "movegen.h"

class PerftTesting {
   public:
    U64 perftFunction(int depth, int max);

    void perfTest(int depth, int max);

    /// @brief perfs a test on all test positions
    void testAllPos(int n = 1);

    Board board;
    int depth;
    U64 nodes;
    Movelist movelists[MAX_PLY];
};
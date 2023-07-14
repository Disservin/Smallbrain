#pragma once

#include "board.h"
#include "movegen.h"

class PerftTesting {
public:
    U64 perftFunction(int depth, int max_depth);

    void perfTest(int depth, int max_depth);

    /// @brief perfs a test on all test positions
    void testAllPos(int n = 1);

    Board board;
    Movelist movelists[MAX_PLY];

    U64 nodes;
};
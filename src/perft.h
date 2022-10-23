#pragma once

#include "board.h"
#include "movegen.h"

class Perft
{
  public:
    Board board;
    int depth;
    uint64_t nodes;

    U64 perftFunction(int depth, int max);

    void perfTest(int depth, int max);

    /// @brief perfs a test on all test positions
    void testAllPos(int n = 1);
};
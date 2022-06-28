#include <chrono>

#include "board.h"

class Perft {
    public:
    Board board;
    int depth;
    uint64_t nodes;

    U64 perft_function(int depth, int max);
    void perf_Test(int depth, int max);
    void testAllPos();
};
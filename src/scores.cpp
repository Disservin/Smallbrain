#include "scores.h"
#include "types.h"

#include <cmath>

int piece_values[2][6] = { { 98, 337, 365, 477, 1025, 0}, { 114, 281, 297, 512,  936, 0} };

int killerscore1 = 900000;
int killerscore2 = 800000;

int reductions[256][120];

// Initialize reduction table
void init_reductions(int threads) {
    for (int moves = 1; moves < 257; moves++){
        for (int depth = 1; depth < MAX_PLY + 1; depth++) {
            reductions[moves-1][depth-1] = 1 + log(moves * (std::max(threads % 3, 1)))  * log(depth) / 1.75;
        }
    }
}

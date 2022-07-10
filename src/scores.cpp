#include "scores.h"

int piece_values[2][6] = { { 98, 337, 365, 477, 1025, 0}, { 114, 281, 297, 512,  936, 0} };

int killerscore1 = 6'000'000;
int killerscore2 = 5'000'000;

int reductions[MAX_MOVES][MAX_PLY];

// Initialize reduction table
void init_reductions() {
    for (int moves = 0; moves < MAX_MOVES; moves++){
        for (int depth = 0; depth < MAX_PLY; depth++){
            reductions[moves][depth] = 1 + log(moves) * log(depth)  / 1.75;
        }
    }    
}

#include "scores.h"

int piece_values[2][7] = {{98, 337, 365, 477, 1025, 0, 0}, {114, 281, 297, 512, 936, 0, 0}};
int piece_values_default[7] = {100, 320, 330, 500, 900, 0, 0};

int killerscore1 = 6'000'000;
int killerscore2 = 5'000'000;

int reductions[MAX_PLY][MAX_MOVES];

// Initialize reduction table
void init_reductions()
{
    for (int depth = 0; depth < MAX_PLY; depth++)
    {
        for (int moves = 0; moves < MAX_MOVES; moves++)
            reductions[depth][moves] = 1 + log(depth) * log(moves) / 1.75;
    }
}

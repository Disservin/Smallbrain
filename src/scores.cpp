#include "scores.h"

int piece_values[2][7] = { { 98, 337, 365, 477, 1025, 0, 0}, { 114, 281, 297, 512,  936, 0, 0} };

int razor_margin = 166;

int fut_margin_1 = 127;
int fut_margin_2 = 107;

int nmp_depth = 4;
int nmp_depth_divisor = 210;

int late_move_pruning_depth = 4;
int late_move_pruning_th = 6;

int seeDepth = 8;
int seeThreshold = 268;

int lmr_margin = 3;
int lmr_margin_2 = 2;

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

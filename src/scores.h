#pragma once
#include "types.h"

#include <cmath>

extern int piece_values[2][7];

extern int razor_margin;

extern int fut_margin_1;
extern int fut_margin_2;

extern int nmp_depth;
extern int nmp_depth_divisor;

extern int late_move_pruning_depth;
extern int late_move_pruning_th;

extern int seeDepth;
extern int seeThreshold;

extern int lmr_margin;
extern int lmr_margin_2; 

extern int killerscore1;
extern int killerscore2;

extern int reductions[MAX_MOVES][MAX_PLY];

void init_reductions();
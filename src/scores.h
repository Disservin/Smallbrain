#pragma once
#include "types.h"

#include <cmath>

extern int piece_values[2][7];
extern int piece_values_default[7];

extern int killerscore1;
extern int killerscore2;

extern int reductions[MAX_PLY][MAX_MOVES];

void init_reductions();
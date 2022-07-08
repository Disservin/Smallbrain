#pragma once
#include "types.h"

#include <cmath>

extern int piece_values[2][6];

extern int killerscore1;
extern int killerscore2;

extern int reductions[MAX_MOVES][120];

void init_reductions();
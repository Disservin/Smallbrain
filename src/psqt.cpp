#include "psqt.h"

int piece_values[2][6] = { { 98, 337, 365, 477, 1025, 0}, { 114, 281, 297, 512,  936, 0} };

int killerscore1 = 900000;
int killerscore2 = 800000;

int reductions[256][120];
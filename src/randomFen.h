#pragma once

#include <algorithm>
#include <bitset>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "attacks.h"
#include "helper.h"
#include "sliders.hpp"
#include "types.h"

extern std::random_device rd;
extern std::mt19937 e;

inline constexpr char piece_to_char[] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k', '-'};
inline constexpr int max_pieces[] = {1, 2, 2, 2, 1, 1, 1, 2, 2, 2, 1, 1};

inline constexpr int piece_values_random[] = {100, 320, 330, 500, 900, 20000, -100, -320, -330, -500, -900, -20000, 0};

static std::unordered_map<char, int> charToNum({{'P', 0},
                                                {'N', 1},
                                                {'B', 2},
                                                {'R', 3},
                                                {'Q', 4},
                                                {'K', 5},
                                                {'p', 6},
                                                {'n', 7},
                                                {'b', 8},
                                                {'r', 9},
                                                {'q', 10},
                                                {'k', 11},
                                                {'.', 12}});

std::string getRandomfen();
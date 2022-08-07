#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>

// #include "board.h"

#define INPUT_WEIGHTS 64 * 12
#define HIDDEN_BIAS 64 * 2 * 2
#define HIDDEN_WEIGHTS 64 * 2 * 2
#define OUTPUT_BIAS 1

/// HIDDEN_BIAS/HIDDEN_WEIGHTS is basically the width of the hidden layer.
extern uint8_t inputValues[INPUT_WEIGHTS];
extern int16_t inputWeights[INPUT_WEIGHTS * HIDDEN_WEIGHTS];
extern int16_t hiddenBias[HIDDEN_BIAS];
extern int16_t hiddenWeights[HIDDEN_WEIGHTS];
extern int32_t outputBias[OUTPUT_BIAS];

namespace NNUE
{
int16_t relu(int16_t x);
void init(const char *filename);
// void accumulate(Board& b);
void activate(std::array<int16_t, HIDDEN_BIAS> &accumulator, int inputNum);
void deactivate(std::array<int16_t, HIDDEN_BIAS> &accumulator, int inputNum);
int32_t output(std::array<int16_t, HIDDEN_BIAS> &accumulator);
// std::vector<int16_t> accumulator;
} // namespace NNUE
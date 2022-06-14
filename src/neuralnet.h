#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdio.h>

// #include "board.h"

#define INPUT_WEIGHTS 64 * 12
#define HIDDEN_BIAS 64 * 2
#define HIDDEN_WEIGHTS 64 * 2
#define OUTPUT_BIAS 1

extern bool NNUE_initialized;

class NNUE {
    public:
    int relu(int x);
    void init(const char* filename);
    // void accumulate(Board& b);
    void activate(int inputNum);
    void deactivate(int inputNum);
    int32_t output();
    void Clear();
    std::vector<int16_t> accumulator;
    uint8_t inputValues[INPUT_WEIGHTS];
    int16_t inputWeights[INPUT_WEIGHTS * HIDDEN_WEIGHTS];
    int16_t hiddenBias[HIDDEN_BIAS];
    int16_t hiddenWeights[HIDDEN_WEIGHTS];
    int32_t outputBias[OUTPUT_BIAS];
};
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

#include "board.h"

class NNUE {
    public:
    int relu(int x);
    void init(const char* filename);
    void accumulate(Board& b);
    void activate(int inputNum);
    void deactivate(int inputNum);
    int32_t output();
    void Clear();
    private:
    std::vector<int16_t> accumulator;
    uint8_t inputValues[768];
    int16_t inputWeights[768 * 64];
    int16_t hiddenBias[64];
    int16_t hiddenWeights[64];
    int32_t outputBias[1];
};
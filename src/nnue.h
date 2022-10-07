#pragma once

#include <array>
#include <cstdint> // int32_t
#include <cstdio>  // file reading
#include <cstring> // memcpy

#define INPUT_WEIGHTS 64 * 12
#define HIDDEN_BIAS 512
#define HIDDEN_WEIGHTS 512
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

// load the weights and bias
void init(const char *filename);

// activate a certain input and update the accumulator
void activate(std::array<int16_t, HIDDEN_BIAS> &accumulator, int inputNum);

// deactivate a certain input and update the accumulator
void deactivate(std::array<int16_t, HIDDEN_BIAS> &accumulator, int inputNum);

// return the nnue evaluation
int32_t output(std::array<int16_t, HIDDEN_BIAS> &accumulator);
} // namespace NNUE
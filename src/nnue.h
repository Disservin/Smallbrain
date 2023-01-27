#pragma once

#include <array>
#include <cstdint> // int32_t
#include <cstdio>  // file reading
#include <cstring> // memcpy

#include "types.h"

#define FEATURE_SIZE 64 * 12
#define N_HIDDEN_SIZE 512
#define OUTPUT_BIAS 1

/// N_HIDDEN_SIZE/N_HIDDEN_SIZE is basically the width of the hidden layer.
extern int16_t inputWeights[FEATURE_SIZE * N_HIDDEN_SIZE];
extern int16_t hiddenBias[N_HIDDEN_SIZE];
extern int16_t hiddenWeights[N_HIDDEN_SIZE * 2];
extern int32_t outputBias[OUTPUT_BIAS];

namespace NNUE
{
using accumulator = std::array<std::array<int16_t, N_HIDDEN_SIZE>, 2>;

int16_t relu(int16_t x);

// load the weights and bias
void init(const char *filename);

// activate a certain input and update the accumulator
void activate(NNUE::accumulator &accumulator, Square sq, Piece p);

// deactivate a certain input and update the accumulator
void deactivate(NNUE::accumulator &accumulator, Square sq, Piece p);

// activate and deactivate, mirrors the logic of a move
void move(NNUE::accumulator &accumulator, Square from_sq, Square to_sq, Piece p);

// return the nnue evaluation
int32_t output(const NNUE::accumulator &accumulator, Color side);
} // namespace NNUE
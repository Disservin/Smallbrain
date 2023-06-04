#pragma once

#include <array>
#include <cstdint>  // int32_t
#include <cstdio>   // file reading
#include <cstring>  // memcpy

#include "types.h"

#define BUCKETS 4
#define FEATURE_SIZE 64 * 12
#define N_HIDDEN_SIZE 512
#define OUTPUTS 1

extern int16_t INPUT_WEIGHTS[BUCKETS * FEATURE_SIZE * N_HIDDEN_SIZE];
extern int16_t HIDDEN_BIAS[N_HIDDEN_SIZE];
extern int16_t HIDDEN_WEIGHTS[N_HIDDEN_SIZE * 2];
extern int32_t OUTPUT_BIAS[OUTPUTS];

namespace NNUE {

// clang-format off
constexpr int KING_BUCKET[64] {
    0, 0, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 0, 1, 1, 1, 1,
    2, 2, 2, 2, 3, 3, 3, 3,
    2, 2, 2, 2, 3, 3, 3, 3,
    2, 2, 2, 2, 3, 3, 3, 3,
    2, 2, 2, 2, 3, 3, 3, 3,
};

// clang-format on

using accumulator = std::array<std::array<int16_t, N_HIDDEN_SIZE>, 2>;

[[nodiscard]] int16_t relu(int16_t x);

// load the weights and bias
void init(const char *filename);

// activate a certain input and update the accumulator
void activate(NNUE::accumulator &accumulator, Square sq, Piece p, Square kSQ_White,
              Square kSq_Black);

// deactivate a certain input and update the accumulator
void deactivate(NNUE::accumulator &accumulator, Square sq, Piece p, Square kSQ_White,
                Square kSq_Black);

// activate and deactivate, mirrors the logic of a move
void move(NNUE::accumulator &accumulator, Square from_sq, Square to_sq, Piece p, Square kSQ_White,
          Square kSq_Black);

// return the nnue evaluation
[[nodiscard]] int32_t output(const NNUE::accumulator &accumulator, Color side);
}  // namespace NNUE
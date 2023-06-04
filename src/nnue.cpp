
#include <iostream>

#include "nnue.h"

#define INCBIN_STYLE INCBIN_STYLE_CAMEL
#include "incbin/incbin.h"

INCBIN(Eval, EVALFILE);

int16_t inputWeights[BUCKETS * FEATURE_SIZE * N_HIDDEN_SIZE];
int16_t hiddenBias[N_HIDDEN_SIZE];
int16_t hiddenWeights[N_HIDDEN_SIZE * 2];
int32_t outputBias[OUTPUT_BIAS];

namespace NNUE {

int idx(Color side, Square sq, Piece p, int ksq) {
    if (side == White) {
        return sq + p * 64 + KING_BUCKET[ksq] * FEATURE_SIZE;
    } else {
        const Square mirrorSquare = Square(sq ^ 56);
        const Piece mirrorPiece = flippedPiece[p];
        const int input = mirrorSquare + mirrorPiece * 64;

        return input + KING_BUCKET[ksq ^ 56] * FEATURE_SIZE;
    }
}

void activate(NNUE::accumulator &accumulator, Square sq, Piece p, Square kSQ_White,
              Square kSq_Black) {
    for (auto side : {White, Black}) {
        const int input = idx(side, sq, p, side == White ? kSQ_White : kSq_Black);
        for (int chunks = 0; chunks < N_HIDDEN_SIZE / 256; chunks++) {
            const int offset = chunks * 256;
            for (int i = offset; i < 256 + offset; i++) {
                accumulator[side][i] += inputWeights[input * N_HIDDEN_SIZE + i];
            }
        }
    }
}

void deactivate(NNUE::accumulator &accumulator, Square sq, Piece p, Square kSQ_White,
                Square kSq_Black) {
    for (auto side : {White, Black}) {
        const int input = idx(side, sq, p, side == White ? kSQ_White : kSq_Black);
        for (int chunks = 0; chunks < N_HIDDEN_SIZE / 256; chunks++) {
            const int offset = chunks * 256;
            for (int i = offset; i < 256 + offset; i++) {
                accumulator[side][i] -= inputWeights[input * N_HIDDEN_SIZE + i];
            }
        }
    }
}

void move(NNUE::accumulator &accumulator, Square from_sq, Square to_sq, Piece p, Square kSQ_White,
          Square kSq_Black) {
    for (auto side : {White, Black}) {
        const int inputClear = idx(side, from_sq, p, side == White ? kSQ_White : kSq_Black);
        const int inputAdd = idx(side, to_sq, p, side == White ? kSQ_White : kSq_Black);

        for (int chunks = 0; chunks < N_HIDDEN_SIZE / 256; chunks++) {
            const int offset = chunks * 256;
            for (int i = offset; i < 256 + offset; i++) {
                accumulator[side][i] += -inputWeights[inputClear * N_HIDDEN_SIZE + i] +
                                        inputWeights[inputAdd * N_HIDDEN_SIZE + i];
            }
        }
    }
}

int16_t relu(int16_t x) { return std::max(static_cast<int16_t>(0), x); }

int32_t output(const NNUE::accumulator &accumulator, Color side) {
    int32_t output = outputBias[0];

    for (int chunks = 0; chunks < N_HIDDEN_SIZE / 256; chunks++) {
        const int offset = chunks * 256;
        for (int i = 0; i < 256; i++) {
            output += relu(accumulator[side][i + offset]) * hiddenWeights[i + offset];
        }
    }

    // seperate loop seems to be faster
    for (int chunks = 0; chunks < N_HIDDEN_SIZE / 256; chunks++) {
        const int offset = chunks * 256;
        for (int i = 0; i < 256; i++) {
            output +=
                relu(accumulator[!side][i + offset]) * hiddenWeights[N_HIDDEN_SIZE + i + offset];
        }
    }

    return output / (16 * 512);
}

void init(const char *filename) {
    FILE *f = fopen(filename, "rb");

    // obtain file size
    int fileSize = FEATURE_SIZE * N_HIDDEN_SIZE + N_HIDDEN_SIZE + 2 * N_HIDDEN_SIZE + OUTPUT_BIAS;

    int readElements = 0;
    if (f != NULL) {
        readElements +=
            fread(inputWeights, sizeof(int16_t), BUCKETS * FEATURE_SIZE * N_HIDDEN_SIZE, f);
        readElements += fread(hiddenBias, sizeof(int16_t), N_HIDDEN_SIZE, f);
        readElements += fread(hiddenWeights, sizeof(int16_t), 2 * N_HIDDEN_SIZE, f);
        readElements += fread(outputBias, sizeof(int32_t), OUTPUT_BIAS, f);

        if (readElements != fileSize) {
            std::cout << "The network was not fully loaded"
                      << " " << readElements << " " << fileSize << std::endl;
            exit(2);
        }

        fclose(f);
    } else {
        int memoryIndex = 0;
        std::memcpy(inputWeights, &gEvalData[memoryIndex],
                    BUCKETS * FEATURE_SIZE * N_HIDDEN_SIZE * sizeof(int16_t));
        memoryIndex += BUCKETS * FEATURE_SIZE * N_HIDDEN_SIZE * sizeof(int16_t);
        std::memcpy(hiddenBias, &gEvalData[memoryIndex], N_HIDDEN_SIZE * sizeof(int16_t));
        memoryIndex += N_HIDDEN_SIZE * sizeof(int16_t);

        std::memcpy(hiddenWeights, &gEvalData[memoryIndex], 2 * N_HIDDEN_SIZE * sizeof(int16_t));
        memoryIndex += 2 * N_HIDDEN_SIZE * sizeof(int16_t);
        std::memcpy(outputBias, &gEvalData[memoryIndex], 1 * sizeof(int32_t));
        memoryIndex += OUTPUT_BIAS * sizeof(int32_t);
    }
}
}  // namespace NNUE
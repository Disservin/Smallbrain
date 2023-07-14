
#include <iostream>

#include "nnue.h"

#define INCBIN_STYLE INCBIN_STYLE_CAMEL

#include "incbin/incbin.h"

INCBIN(Eval, EVALFILE);

alignas(32) int16_t INPUT_WEIGHTS[BUCKETS * FEATURE_SIZE * N_HIDDEN_SIZE];
alignas(32) int16_t HIDDEN_BIAS[N_HIDDEN_SIZE];
alignas(32) int16_t HIDDEN_WEIGHTS[N_HIDDEN_SIZE * 2];
alignas(32) int32_t OUTPUT_BIAS[OUTPUTS];

namespace nnue {

    template<Color side>
    int idx(Square sq, Piece p, int ksq) {
        if constexpr (side == WHITE) {
            return sq + static_cast<int>(p) * 64 + KING_BUCKET[ksq] * FEATURE_SIZE;
        } else {
            const Square mirrorSquare = Square(sq ^ 56);
            const int mirrorPiece = static_cast<int>(FLIPPED_PIECE[static_cast<int>(p)]);
            const int input = mirrorSquare + mirrorPiece * 64;

            return input + KING_BUCKET[ksq ^ 56] * FEATURE_SIZE;
        }
    }

    void activate(nnue::accumulator &accumulator, Square sq, Piece p, Square ksq_white,
                  Square ksq_black) {
        const int input_white = idx<WHITE>(sq, p, ksq_white);
        const int input_black = idx<BLACK>(sq, p, ksq_black);

        for (int chunks = 0; chunks < N_HIDDEN_SIZE / 256; chunks++) {
            const int offset = chunks * 256;

            for (int i = offset; i < 256 + offset; i++) {
                accumulator[0][i] += INPUT_WEIGHTS[input_white * N_HIDDEN_SIZE + i];
            }

            for (int i = offset; i < 256 + offset; i++) {
                accumulator[1][i] += INPUT_WEIGHTS[input_black * N_HIDDEN_SIZE + i];
            }
        }
    }

    void deactivate(nnue::accumulator &accumulator, Square sq, Piece p, Square ksq_white,
                    Square ksq_black) {
        const int input_white = idx<WHITE>(sq, p, ksq_white);
        const int input_black = idx<BLACK>(sq, p, ksq_black);
        for (int chunks = 0; chunks < N_HIDDEN_SIZE / 256; chunks++) {
            const int offset = chunks * 256;

            for (int i = offset; i < 256 + offset; i++) {
                accumulator[0][i] -= INPUT_WEIGHTS[input_white * N_HIDDEN_SIZE + i];
            }

            for (int i = offset; i < 256 + offset; i++) {
                accumulator[1][i] -= INPUT_WEIGHTS[input_black * N_HIDDEN_SIZE + i];
            }
        }
    }

    void move(nnue::accumulator &accumulator, Square from_sq, Square to_sq, Piece p, Square ksq_white,
              Square ksq_black) {
        const int input_clear_white = idx<WHITE>(from_sq, p, ksq_white);
        const int input_add_white = idx<WHITE>(to_sq, p, ksq_white);

        const int input_clear_black = idx<BLACK>(from_sq, p, ksq_black);
        const int input_add_black = idx<BLACK>(to_sq, p, ksq_black);

        for (int chunks = 0; chunks < N_HIDDEN_SIZE / 256; chunks++) {
            const int offset = chunks * 256;

            for (int i = offset; i < 256 + offset; i++) {
                accumulator[0][i] += -INPUT_WEIGHTS[input_clear_white * N_HIDDEN_SIZE + i] +
                                     INPUT_WEIGHTS[input_add_white * N_HIDDEN_SIZE + i];
            }

            for (int i = offset; i < 256 + offset; i++) {
                accumulator[1][i] += -INPUT_WEIGHTS[input_clear_black * N_HIDDEN_SIZE + i] +
                                     INPUT_WEIGHTS[input_add_black * N_HIDDEN_SIZE + i];
            }
        }
    }

    int16_t relu(int16_t x) { return std::max(static_cast<int16_t>(0), x); }

    int32_t output(const nnue::accumulator &accumulator, Color side) {
        int32_t output = OUTPUT_BIAS[0];

        for (int chunks = 0; chunks < N_HIDDEN_SIZE / 256; chunks++) {
            const int offset = chunks * 256;

            // two individual loops seem to be faster for auto vectorization
            for (int i = 0; i < 256; i++) {
                output +=
                        relu(accumulator[static_cast<int>(side)][i + offset]) * HIDDEN_WEIGHTS[i + offset];
            }

            for (int i = 0; i < 256; i++) {
                output += relu(accumulator[static_cast<int>(~side)][i + offset]) *
                          HIDDEN_WEIGHTS[N_HIDDEN_SIZE + i + offset];
            }
        }

        return output / (16 * 512);
    }

    void init(const char *filename) {
        FILE *f = fopen(filename, "rb");

        // obtain file size
        std::size_t fileSize = FEATURE_SIZE * N_HIDDEN_SIZE + N_HIDDEN_SIZE + 2 * N_HIDDEN_SIZE + OUTPUTS;

        std::size_t readElements = 0;
        if (f != nullptr) {
            readElements +=
                    fread(INPUT_WEIGHTS, sizeof(int16_t), BUCKETS * FEATURE_SIZE * N_HIDDEN_SIZE, f);
            readElements += fread(HIDDEN_BIAS, sizeof(int16_t), N_HIDDEN_SIZE, f);
            readElements += fread(HIDDEN_WEIGHTS, sizeof(int16_t), 2 * N_HIDDEN_SIZE, f);
            readElements += fread(OUTPUT_BIAS, sizeof(int32_t), OUTPUTS, f);

            if (readElements != fileSize) {
                std::cout << "The network was not fully loaded"
                          << " " << readElements << " " << fileSize << std::endl;
                exit(2);
            }

            fclose(f);
        } else {
            int memoryIndex = 0;
            std::memcpy(INPUT_WEIGHTS, &gEvalData[memoryIndex],
                        BUCKETS * FEATURE_SIZE * N_HIDDEN_SIZE * sizeof(int16_t));
            memoryIndex += BUCKETS * FEATURE_SIZE * N_HIDDEN_SIZE * sizeof(int16_t);
            std::memcpy(HIDDEN_BIAS, &gEvalData[memoryIndex], N_HIDDEN_SIZE * sizeof(int16_t));
            memoryIndex += N_HIDDEN_SIZE * sizeof(int16_t);

            std::memcpy(HIDDEN_WEIGHTS, &gEvalData[memoryIndex], 2 * N_HIDDEN_SIZE * sizeof(int16_t));
            memoryIndex += 2 * N_HIDDEN_SIZE * sizeof(int16_t);
            std::memcpy(OUTPUT_BIAS, &gEvalData[memoryIndex], 1 * sizeof(int32_t));
            memoryIndex += OUTPUTS * sizeof(int32_t);
        }

        std::cout << "Loaded NNUE network" << std::endl;
    }
}  // namespace nnue
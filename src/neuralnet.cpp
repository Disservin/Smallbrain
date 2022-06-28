#include "neuralnet.h"


#define INCBIN_STYLE INCBIN_STYLE_CAMEL
#include "incbin/incbin.h"

INCBIN(Eval, EVALFILE);

uint8_t inputValues[INPUT_WEIGHTS];
int16_t inputWeights[INPUT_WEIGHTS * HIDDEN_WEIGHTS];
int16_t hiddenBias[HIDDEN_BIAS];
int16_t hiddenWeights[HIDDEN_WEIGHTS];
int32_t outputBias[OUTPUT_BIAS];

void NNUE::init(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f != NULL) {    
        fread(inputWeights  , sizeof(int16_t), INPUT_WEIGHTS * HIDDEN_WEIGHTS, f);
        fread(hiddenBias    , sizeof(int16_t), HIDDEN_BIAS, f);
        fread(hiddenWeights , sizeof(int16_t), HIDDEN_WEIGHTS, f);
        fread(outputBias    , sizeof(int32_t), OUTPUT_BIAS, f);

        fclose(f);
    } else {
        int memoryIndex = 0;
        std::memcpy(inputWeights, &gEvalData[memoryIndex], INPUT_WEIGHTS * HIDDEN_WEIGHTS * sizeof(int16_t));
        memoryIndex += INPUT_WEIGHTS * HIDDEN_WEIGHTS * sizeof(int16_t);
        std::memcpy(hiddenBias, &gEvalData[memoryIndex], HIDDEN_BIAS * sizeof(int16_t));
        memoryIndex += HIDDEN_BIAS * sizeof(int16_t);

        std::memcpy(hiddenWeights, &gEvalData[memoryIndex], HIDDEN_WEIGHTS * sizeof(int16_t));
        memoryIndex += HIDDEN_WEIGHTS * sizeof(int16_t);
        std::memcpy(outputBias, &gEvalData[memoryIndex], 1 * sizeof(int32_t));
        memoryIndex += OUTPUT_BIAS * sizeof(int32_t);   
    }
}

// This is no longer used.
// applyFen resets the accumulator and activates the input neurons
// void NNUE::accumulate(Board& b) {
//     for (int i = 0; i < HIDDEN_BIAS; i++) {
//         accumulator[i] = hiddenBias[i];
//     }
    
//     for (int i = 0; i < 64; i++) {
//         bool input = b.pieceAtB(Square(i)) != None;
//         if (!input) continue;
//         int j = Square(i) + (b.pieceAtB(Square(i))) * 64;
//         inputValues[j] = 1;
//         activate(j);
//     }
// }

// void NNUE::activate(int inputNum) {
//     for (int i = 0; i < HIDDEN_BIAS; i++) {
//         accumulator[i] += inputWeights[inputNum * HIDDEN_BIAS + i];
//     }
// }

// void NNUE::deactivate(int inputNum) {
//     for (int i = 0; i < HIDDEN_BIAS; i++) {
//         accumulator[i] -= inputWeights[inputNum * HIDDEN_BIAS + i];
//     }
// }

int NNUE::relu(int x) {
    if (x < 0) {
        return 0;
    }
    return x;
}

int32_t NNUE::output(int16_t accumulator[HIDDEN_BIAS]) {
    int32_t output = 0;
    for (int i = 0; i < HIDDEN_BIAS; i++) {
        output += relu(accumulator[i]) * hiddenWeights[i];
    }
    output += outputBias[0];
    return output / (64 * 256);
}

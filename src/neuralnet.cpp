#include "neuralnet.h"


#define INCBIN_STYLE INCBIN_STYLE_CAMEL
#include "incbin/incbin.h"

INCBIN(Eval, EVALFILE);

void NNUE::init(const char* filename) {
    for (int i = 0; i < 64 * 2; i++) {
        accumulator.push_back(0);
    }
    FILE* f = fopen(filename, "rb");
    if (f != NULL) {    
        fread(inputWeights  , sizeof(int16_t), 768 * 64 * 2, f);
        fread(hiddenBias    , sizeof(int16_t),   2 * 64, f);
        fread(hiddenWeights , sizeof(int16_t),   2 * 64, f);
        fread(outputBias    , sizeof(int32_t),        1, f);

        fclose(f);
    } else {
        int memoryIndex = 0;
        std::memcpy(inputWeights, &gEvalData[memoryIndex], 768 * 64 * 2 * sizeof(int16_t));
        memoryIndex += 768 * 64 * 2 * sizeof(int16_t);
        std::memcpy(hiddenBias, &gEvalData[memoryIndex], 2 * 64 * sizeof(int16_t));
        memoryIndex += 2 * 64 * sizeof(int16_t);

        std::memcpy(hiddenWeights, &gEvalData[memoryIndex], 2 * 64 * sizeof(int16_t));
        memoryIndex += 2 * 64 * sizeof(int16_t);
        std::memcpy(outputBias, &gEvalData[memoryIndex], 1 * sizeof(int32_t));
        memoryIndex += 1 * sizeof(int32_t);   
    }
}

void NNUE::accumulate(Board& b) {
    for (int i = 0; i < 128; i++) {
        accumulator[i] = hiddenBias[i];
    }
    
    for (int i = 0; i < 64; i++) {
        bool input = b.pieceAtB(Square(i)) != None;
        if (!input) continue;
        int j = Square(i) + (b.pieceAtB(Square(i))) * 64;
        inputValues[j] = 1;
        activate(j);
    }
}

void NNUE::activate(int inputNum) {
    for (int i = 0; i < 128; i++) {
        accumulator[i] += inputWeights[inputNum * 128 + i];
    }
}

void NNUE::deactivate(int inputNum) {
    for (int i = 0; i < 128; i++) {
        accumulator[i] -= inputWeights[inputNum * 128 + i];
    }
}

int NNUE::relu(int x) {
    if (x < 0) {
        return 0;
    }
    return x;
}

int32_t NNUE::output() {
    int32_t output = 0;
    for (int i = 0; i < 128; i++) {
        output += relu(accumulator[i]) * hiddenWeights[i];
    }
    output += outputBias[0];
    return output / (64 * 256);
}

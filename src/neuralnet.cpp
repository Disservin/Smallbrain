#include "neuralnet.h"

void NNUE::init(const char* filename) {
    for (int i = 0; i < 64 * 2; i++) {
        accumulator.push_back(0);
    }
    FILE* f = fopen(filename, "rb");

    fread(inputWeights  , sizeof(int16_t), 768 * 64, f);
    fread(hiddenBias    , sizeof(int16_t),   2 * 64, f);
    fread(hiddenWeights , sizeof(int16_t),   2 * 64, f);
    fread(outputBias    , sizeof(int32_t),        1, f);

    fclose(f);
}

void NNUE::accumulate(Board& b) {
    for (int i = 0; i < 64 * 2; i++) {
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
    for (int i = 0; i < 64 * 2; i++) {
        accumulator[i] += inputWeights[inputNum * 64 + i];
    }
}

void NNUE::deactivate(int inputNum) {
    for (int i = 0; i < 64; i++) {
        accumulator[i] -= inputWeights[inputNum * 64 + i];
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
    for (int i = 0; i < 64 * 2; i++) {
        output += relu(accumulator[i]) * hiddenWeights[i];
    }
    output += outputBias[0];
    return output / (64 * 256);
}

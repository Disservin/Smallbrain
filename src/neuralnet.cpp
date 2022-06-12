#include "neuralnet.h"

void NNUE::init(std::string filename) {
    // std::ifstream file;
    // file.open(filename, std::ios::binary | std::ios::in);
    // if (!file) {
    //     std::cout << "Error opening file: " << filename << std::endl;
    //     return;
    // }

    for (int i = 0; i < 64; i++) {
        accumulator.push_back(0);
    }
    FILE* f = fopen("20.net", "rb");

    fread(inputWeights  , sizeof(int16_t), 768 * 64, f);
    fread(hiddenBias    , sizeof(int16_t),       64, f);
    fread(hiddenWeights , sizeof(int16_t),       64, f);
    fread(outputBias    , sizeof(int32_t),        1, f);

    fclose(f);
    // // Read input weights, we multiply by 2 because tempWeights is char (8 bits)
    // // 768 rows and 64 columns
    // char tempWeights[768 * 64 * 2];
    // file.read(tempWeights, 768 * 64 * 2);
    // // for (int i = 0; i < 768 * 64 * 2; i += 2) {
    // //     inputWeights[i / 2] = (tempWeights[i] << 8) | tempWeights[i + 1];
    // // }
    // for (int i = 0; i < 768 * 2; i += 2) {
    //     inputWeights[i / 2][(i/2 % 64)] = (tempWeights[i] << 8) | tempWeights[i + 1];
    // }

    // // Read bias for first hidden layer
    // char temphiddenBias[64 * 4];
    // file.read(tempWeights, 64 * 4);
    // for (int i = 0; i < 64 * 4; i += 4) {
    //     std::cout << temphiddenBias[i] << std::endl;
    //     hiddenBias[i / 4] = (temphiddenBias[i] << 24) | temphiddenBias[i + 1] << 16 | temphiddenBias[i + 2] << 8 | temphiddenBias[i + 3];
    // }

    // // Read weights for first hidden layer (aka hidden layer -> output)
    // char temphiddenWeights[64 * 2];
    // file.read(temphiddenWeights, 64 * 2);
    // for (int i = 0; i < 64 * 2; i += 2) {
    //     hiddenWeights[i / 2] = (temphiddenWeights[i] << 8) | temphiddenWeights[i + 1];
    // }

    // // Read bias for output layer
    // char tempoutputBias[4];
    // file.read(tempoutputBias, 4);
    // outputBias = (tempoutputBias[0] << 24) | tempoutputBias[1] << 16 | tempoutputBias[2] << 8 | tempoutputBias[3];
}

void NNUE::accumulate(Board& b) {
    for (int i = 0; i < 64; i++) {
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
    for (int i = 0; i < 64; i++) {
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
    for (int i = 0; i < 64; i++) {
        output += relu(accumulator[i]) * hiddenWeights[i];
    }
    output += outputBias[0];
    return output / (128 * 256);
}

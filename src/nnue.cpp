#include "nnue.h"

#include <iostream>

#define INCBIN_STYLE INCBIN_STYLE_CAMEL
#include "incbin/incbin.h"

INCBIN(Eval, EVALFILE);

uint8_t inputValues[INPUT_WEIGHTS];
int16_t inputWeights[INPUT_WEIGHTS * HIDDEN_WEIGHTS];
int16_t hiddenBias[HIDDEN_BIAS];
int16_t hiddenWeights[HIDDEN_WEIGHTS];
int32_t outputBias[OUTPUT_BIAS];

namespace NNUE
{
void init(const char *filename)
{
    FILE *f = fopen(filename, "rb");

    // obtain file size
    int fileSize = INPUT_WEIGHTS * HIDDEN_WEIGHTS + HIDDEN_BIAS + HIDDEN_WEIGHTS + OUTPUT_BIAS;

    int readElements = 0;
    if (f != NULL)
    {
        readElements += fread(inputWeights, sizeof(int16_t), INPUT_WEIGHTS * HIDDEN_WEIGHTS, f);
        readElements += fread(hiddenBias, sizeof(int16_t), HIDDEN_BIAS, f);
        readElements += fread(hiddenWeights, sizeof(int16_t), HIDDEN_WEIGHTS, f);
        readElements += fread(outputBias, sizeof(int32_t), OUTPUT_BIAS, f);

        if (readElements != fileSize)
        {
            std::cout << "The network was not fully loaded"
                      << " " << readElements << " " << fileSize << std::endl;
            exit(2);
        }

        fclose(f);
    }
    else
    {
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

void activate(std::array<int16_t, HIDDEN_BIAS> &accumulator, Square sq, Piece p)
{
    const int inputUs = sq + p * 64;

    for (int chunks = 0; chunks < 2; chunks++)
    {
        const int offset = chunks * 256;
        for (int i = offset; i < 256 + offset; i++)
        {
            accumulator[i] += inputWeights[inputUs * HIDDEN_BIAS + i];
        }
    }
}

void deactivate(std::array<int16_t, HIDDEN_BIAS> &accumulator, Square sq, Piece p)
{
    const int inputUs = sq + p * 64;

    for (int chunks = 0; chunks < 2; chunks++)
    {
        const int offset = chunks * 256;
        for (int i = offset; i < 256 + offset; i++)
        {
            accumulator[i] -= inputWeights[inputUs * HIDDEN_BIAS + i];
        }
    }
}

void move(std::array<int16_t, HIDDEN_BIAS> &accumulator, Square from_sq, Square to_sq, Piece p)
{
    const int ClearInputUs = from_sq + p * 64;

    const int AddInputUs = to_sq + p * 64;

    for (int chunks = 0; chunks < 2; chunks++)
    {
        const int offset = chunks * 256;
        for (int i = offset; i < 256 + offset; i++)
        {
            accumulator[i] +=
                -inputWeights[ClearInputUs * HIDDEN_BIAS + i] + inputWeights[AddInputUs * HIDDEN_BIAS + i];
        }
    }
}

int16_t relu(int16_t x)
{
    return std::max((int16_t)0, x);
}

int32_t output(const std::array<int16_t, HIDDEN_BIAS> &accumulator)
{
    int32_t output = outputBias[0];

    for (int chunks = 0; chunks < 2; chunks++)
    {
        const int offset = chunks * 256;
        for (int i = 0; i < 256; i++)
        {
            output += relu(accumulator[i + offset]) * hiddenWeights[i + offset];
        }
    }

    return output / (16 * 512);
}
} // namespace NNUE
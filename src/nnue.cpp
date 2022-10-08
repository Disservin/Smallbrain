#include "nnue.h"

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

    uint64_t readElements = 0;
    if (f != NULL)
    {
        readElements += fread(inputWeights, sizeof(int16_t), INPUT_WEIGHTS * HIDDEN_WEIGHTS, f);
        readElements += fread(hiddenBias, sizeof(int16_t), HIDDEN_BIAS, f);
        readElements += fread(hiddenWeights, sizeof(int16_t), HIDDEN_WEIGHTS, f);
        readElements += fread(outputBias, sizeof(int32_t), OUTPUT_BIAS, f);

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

void activate(std::array<int16_t, HIDDEN_BIAS> &accumulator, int inputNum)
{
    for (int i = 0; i < HIDDEN_BIAS; i++)
    {
        accumulator[i] += inputWeights[inputNum * HIDDEN_BIAS + i];
    }
}

void deactivate(std::array<int16_t, HIDDEN_BIAS> &accumulator, int inputNum)
{
    for (int i = 0; i < HIDDEN_BIAS; i++)
    {
        accumulator[i] -= inputWeights[inputNum * HIDDEN_BIAS + i];
    }
}

int16_t relu(int16_t x)
{
    return std::max((int16_t)0, x);
}

int32_t output(const std::array<int16_t, HIDDEN_BIAS> &accumulator)
{
    int32_t output = outputBias[0];
    for (int i = 0; i < HIDDEN_BIAS; i++)
    {
        output += relu(accumulator[i]) * hiddenWeights[i];
    }
    return output / (16 * 512);
}
} // namespace NNUE
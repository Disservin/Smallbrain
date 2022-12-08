#include "nnue.h"

#include <immintrin.h> // Include AVX2 header
#include <iostream>
#include <x86intrin.h>

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

void activate(NNUE::accumulator &accumulator, Square sq, Piece p)
{
    // Compute the input index
    const int inputUs = sq + p * 64;

    // Compute the base offset for accessing the input weights array
    const int baseOffset = inputUs * HIDDEN_BIAS;

    // Create a vector of 16-bit integers with all elements set to 0
    // __m256i zero = _mm256_setzero_si256();

    // Loop over the full accumulator array
    for (int i = 0; i < 512; i += 16)
    {
        // Load the next 16 values from the input weights array using _mm256_loadu_si256()
        __m256i input = _mm256_loadu_si256((__m256i *)&inputWeights[baseOffset + i]);

        // Load the next 16 values from the accumulator array using _mm256_loadu_si256()
        __m256i acc = _mm256_loadu_si256((__m256i *)&accumulator[i]);

        // Add the input and acc vectors using _mm256_add_epi16()
        __m256i result = _mm256_add_epi16(input, acc);

        // Store the result vector to the accumulator array using _mm256_storeu_si256()
        _mm256_storeu_si256((__m256i *)&accumulator[i], result);
    }
}

void deactivate(NNUE::accumulator &accumulator, Square sq, Piece p)
{
    // Compute the input index
    const int inputUs = sq + p * 64;

    // Compute the base offset for accessing the input weights array
    const int baseOffset = inputUs * HIDDEN_BIAS;

    // Loop over the full accumulator array
    for (int i = 0; i < 512; i += 16)
    {
        // Load the next 16 values from the input weights array using _mm256_loadu_si256()
        __m256i input = _mm256_loadu_si256((__m256i *)&inputWeights[baseOffset + i]);

        // Load the next 16 values from the accumulator array using _mm256_loadu_si256()
        __m256i acc = _mm256_loadu_si256((__m256i *)&accumulator[i]);

        // Subtract the input from the acc vector using _mm256_sub_epi16()
        __m256i result = _mm256_sub_epi16(acc, input);

        // Store the result vector to the accumulator array using _mm256_storeu_si256()
        _mm256_storeu_si256((__m256i *)&accumulator[i], result);
    }
}

void move(NNUE::accumulator &accumulator, Square from_sq, Square to_sq, Piece p)
{
    // Compute the input index for the 'from' square
    const int clearInputUs = from_sq + p * 64;

    // Compute the input index for the 'to' square
    const int addInputUs = to_sq + p * 64;

    // Compute the base offsets for accessing the input weights array
    const int clearInputOffset = clearInputUs * HIDDEN_BIAS;
    const int addInputOffset = addInputUs * HIDDEN_BIAS;

    // Loop over the full accumulator array
    for (int i = 0; i < 512; i += 16)
    {
        // Load the next 16 values from the 'clear' input weights array using _mm256_loadu_si256()
        __m256i clearInput = _mm256_loadu_si256((__m256i *)&inputWeights[clearInputOffset + i]);

        // Load the next 16 values from the 'add' input weights array using _m256_loadu_si256()
        __m256i addInput = _mm256_loadu_si256((__m256i *)&inputWeights[addInputOffset + i]);

        // Load the next 16 values from the accumulator array using _mm256_loadu_si256()
        __m256i acc = _mm256_loadu_si256((__m256i *)&accumulator[i]);

        // Subtract the clearInput from the acc vector using _mm256_sub_epi16()
        __m256i temp = _mm256_sub_epi16(acc, clearInput);

        // Add the addInput to the temp vector using _mm256_add_epi16()
        __m256i result = _mm256_add_epi16(temp, addInput);

        // Store the result vector to the accumulator array using _mm256_storeu_si256()
        _mm256_storeu_si256((__m256i *)&accumulator[i], result);
    }
}

int16_t relu(int16_t x)
{
    return std::max((int16_t)0, x);
}

int32_t output(const NNUE::accumulator &accumulator)
{
    int32_t output = outputBias[0];

    for (int chunks = 0; chunks < HIDDEN_BIAS / 256; chunks++)
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
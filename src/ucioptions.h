#pragma once

#include <algorithm>

#include "board.h"
#include "types.h"

// 57344 MiB = 2^32 * 14B / (1024 * 1024)
static constexpr int MAXHASH = (1ull << 32) * sizeof(TEntry) / (1024 * 1024);

struct optionType
{
    std::string name;
    std::string type;
    std::string defaultValue;
    std::string min;
    std::string max;
    // constructor
    optionType(std::string name, std::string type, std::string defaultValue, std::string min, std::string max)
    {
        this->name = name;
        this->type = type;
        this->defaultValue = defaultValue;
        this->min = min;
        this->max = max;
    }
};

class uciOptions
{
  public:
    void printOptions();

    /// @brief resize hash
    /// @param value MB
    void uciHash(int value);

    /// @brief load nnue from file or binary
    /// @param name
    void uciEvalFile(std::string name);

    /// @brief max thread number
    /// @param value
    /// @return
    int uciThreads(int value);

    /// @brief
    /// @param input
    /// @return
    bool uciSyzygy(std::string input);

    /// @brief
    /// @param board
    /// @param v
    void uciChess960(Board &board, std::string_view v);

    bool addIntTuneOption(std::string name, std::string type, int defaultValue, int min, int max);
    bool addDoubleTuneOption(std::string name, std::string type, double defaultValue, double min, double max);
};
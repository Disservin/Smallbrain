#pragma once

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iostream>
#include <vector>

#include "board.h"
#include "nnue.h"
#include "syzygy/Fathom/src/tbprobe.h"
#include "tt.h"
#include "types.h"

extern std::atomic<bool> useTB;
extern TEntry *TTable;
extern U64 TT_SIZE;

// 57344 MB = 2^32 * 14B / (1024 * 1024)
#define MAXHASH 57344

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

struct optionTune
{
    std::string name;
    // constructor
    optionTune(std::string name)
    {
        this->name = name;
    }
};

class uciOptions
{
  public:
    void printOptions();
    void uciHash(int value);
    void uciEvalFile(std::string name);
    int uciThreads(int value);
    void uciSyzygy(std::string path);
    void uciPosition(Board &board, std::string fen = DEFAULT_POS, bool update = true);
    void uciMoves(Board &board, std::vector<std::string> &tokens);
    void addIntTuneOption(std::string name, std::string type, int defaultValue, int min, int max);
    void addDoubleTuneOption(std::string name, std::string type, double defaultValue, double min, double max);
};

void allocateTT();

void reallocateTT(U64 elements);

Move convertUciToMove(Board &board, std::string input);
#pragma once

#include <algorithm>

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

    /// @brief Setup Syzygy
    /// @param input
    void uciSyzygy(std::string input);

    /// @brief Setup a Position
    /// @param board
    /// @param fen
    /// @param update
    void uciPosition(Board &board, std::string fen = DEFAULT_POS, bool update = true);

    /// @brief plays all moves in tokens
    /// @param board
    /// @param tokens
    void uciMoves(Board &board, std::vector<std::string> &tokens);

    void addIntTuneOption(std::string name, std::string type, int defaultValue, int min, int max);
    void addDoubleTuneOption(std::string name, std::string type, double defaultValue, double min, double max);
};

/// @brief allocate Transposition Table and initialize entries
void allocateTT();

/// @brief resize Transposition Table
/// @param elements
void reallocateTT(U64 elements);

/// @brief clear the TT
/// @param elements
void clearTT(U64 elements);

/// @brief convert console input to move
/// @param board
/// @param input
/// @return
Move convertUciToMove(Board &board, std::string input);
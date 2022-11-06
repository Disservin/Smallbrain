#pragma once

#include <type_traits> // for is_same_v

#include "benchmark.h"
#include "board.h"
#include "datagen.h"
#include "evaluation.h"
#include "nnue.h"
#include "perft.h"

#include "search.h"
#include "syzygy/Fathom/src/tbprobe.h"
#include "timemanager.h"
#include "tt.h"
#include "ucioptions.h"

extern std::atomic<bool> stopped;
extern TEntry *TTable;
extern U64 TT_SIZE;

namespace uciCommand
{

void uciInput(uciOptions options);
void isreadyInput();
void ucinewgameInput(uciOptions &options, Board &board, Search &searcher, Datagen::TrainingData &dg);

/// @brief we parse the uci input and call the corresponding function
/// @param input
/// @param searcher
/// @param board
void parseInput(std::string input, Board &board);

/// @brief stop all threads
/// @param searcher
/// @param dg
void stopThreads(Search &searcher, Datagen::TrainingData &dg);

/// @brief quit program
/// @param searcher
/// @param dg
void quit(Search &searcher, Datagen::TrainingData &dg);

/// @brief elementInVector searches el in the tokens
/// @param el
/// @param tokens
/// @return returns false if not found
bool elementInVector(std::string el, std::vector<std::string> tokens);

/// @brief findElement returns the next value after a param
/// @param param
/// @param tokens
/// @return
template <typename T> T findElement(std::string param, std::vector<std::string> tokens)
{
    int index = find(tokens.begin(), tokens.end(), param) - tokens.begin();
    if constexpr (std::is_same_v<T, int>)
        return std::stoi(tokens[index + 1]);
    else
        return tokens[index + 1];
}

/// @brief test if string contains string
/// @param s test
/// @param origin
/// @return
bool stringContain(std::string s, std::string origin);

// Get current date/time
const std::string getVersion();

} // namespace uciCommand
#pragma once

#include "benchmark.h"
#include "board.h"
#include "datagen.h"
#include "evaluation.h"
#include "helper.h"
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

// Get current date/time
const std::string getVersion();

} // namespace uciCommand
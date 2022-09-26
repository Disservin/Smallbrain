#pragma once

#include "benchmark.h"
#include "board.h"
#include "datagen.h"
#include "evaluation.h"
#include "nnue.h"
#include "perft.h"
#include "scores.h"
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
void ucinewgameInput(uciOptions &options, Board &board, Search &searcher, Datagen &dg);

// we parse the uci input and call the corresponding function
void parseInput(std::string input, Search &searcher, Board &board, Datagen &dg);

void stopThreads(Search &searcher, Datagen &dg);

void signalCallbackHandler(Search &searcher, Datagen &dg, int signum);

void quit(Search &searcher, Datagen &dg);

// elementInVector searches el in the tokens and returns false if not found
bool elementInVector(std::string el, std::vector<std::string> tokens);

// findElement returns the next (int) value after a param
int findElement(std::string param, std::vector<std::string> tokens);

// findElementString returns the next (string) value after a param
std::string findElementString(std::string param, std::vector<std::string> tokens);

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime();

} // namespace uciCommand
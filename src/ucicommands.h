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

extern std::atomic<bool> stopped;
extern TEntry *TTable;
extern U64 TT_SIZE;

namespace uciCommand
{

void parseInput(std::string input, Search &searcher, Board &board, Datagen &dg);

void stopThreads(Search &searcher, Datagen &dg);

void signalCallbackHandler(Search &searcher, Datagen &dg, int signum);

void quit(Search &searcher, Datagen &dg);

bool elementInVector(std::string el, std::vector<std::string> tokens);
int findElement(std::string param, std::vector<std::string> tokens);
std::string findElementString(std::string param, std::vector<std::string> tokens);

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime();

} // namespace uciCommand
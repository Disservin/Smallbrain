#pragma once

#include <algorithm>
#include <atomic>
#include <cstring>
#include <fstream>
#include <map>
#include <math.h>
#include <signal.h>
#include <sstream>
#include <thread>

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
#include "ucicommands.h"
#include "ucioptions.h"

extern std::atomic<bool> stopped;
extern std::atomic<bool> UCI_FORCE_STOP;
extern std::atomic<bool> useTB;
extern TEntry *TTable;

#define TUNE_INT(x, min, max)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        extern int x;                                                                                                  \
        options.addIntTuneOption(#x, "spin", x, min, max);                                                             \
    } while (0);

#define TUNE_DOUBLE(x, min, max)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        extern double x;                                                                                               \
        options.addDoubleTuneOption(#x, "spin", x, min, max);                                                          \
    } while (0);

int uciLoop(int argc, char **argv);

void signalCallbackHandler(int signum);

std::vector<std::string> splitInput(std::string fen);

void stopThreads();

void quit();

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime();

void parseArgs(int argc, char **argv, uciOptions options, Board board);
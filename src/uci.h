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

class UCI
{
  public:
    UCI();

    int uciLoop(int argc, char **argv);

  private:
    Search searcher;
    Board board;
    uciOptions options;
    Datagen::TrainingData genData;

    int threadCount;

    void stopThreads();

    void quit();

    // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
    const std::string currentDateTime();

    void parseArgs(int argc, char **argv, uciOptions options, Board board);
};

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

extern std::atomic_bool stopped;
extern std::atomic_bool UCI_FORCE_STOP;
extern TranspositionTable TTable;

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
    Datagen::TrainingData datagen;
    bool useTB;

    int threadCount;

    /// @brief parse custom engine commands
    /// @param argc
    /// @param argv
    /// @param options
    /// @return true if we should terminate after processing the command
    bool parseArgs(int argc, char **argv, uciOptions options);

    void processCommand(std::string command);

    void uciInput();

    void isreadyInput();

    void ucinewgameInput();

    void stopThreads();

    void quit();

    const std::string getVersion();

    void uciMoves(std::vector<std::string> &tokens);

    Square extractSquare(std::string_view squareStr);

    /// @brief convert console input to move
    /// @param board
    /// @param input
    /// @return
    Move convertUciToMove(std::string input);
};

#pragma once

#include "benchmark.h"
#include "board.h"
#include "datagen.h"
#include "timemanager.h"
#include "ucioptions.h"

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
    Board board = Board();
    uciOptions options = uciOptions();
    Datagen::TrainingData datagen = Datagen::TrainingData();

    int threadCount;
    bool useTB;

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

    void quit();

    const std::string getVersion();

    void uciMoves(std::vector<std::string> &tokens);
};

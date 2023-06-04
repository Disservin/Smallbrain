#pragma once

#include "board.h"
#include "datagen.h"
#include "timemanager.h"
#include "ucioptions.h"

#define CONCAT_(prefix, suffix) prefix##suffix
/// Concatenate `prefix, suffix` into `prefixsuffix`
#define CONCAT(prefix, suffix) CONCAT_(prefix, suffix)

#define UNIQUE_VAR(prefix) CONCAT(prefix##_, __COUNTER__)

#define TUNE_INT(x) \
    extern int x;   \
    bool UNIQUE_VAR(Unique) = options_.addIntTuneOption(#x, "spin", x, 0, x * 2);

#define TUNE_DOUBLE(x) \
    extern double x;   \
    bool UNIQUE_VAR(Unique) = options_.addDoubleTuneOption(#x, "spin", x, 0, x * 2);

#define ASSIGN_VALUE(option, var, value) \
    if (option == #var) {                \
        var = std::stoi(value);          \
    }

class UCI {
   public:
    UCI();

    int uciLoop(int argc, char **argv);

   private:
    Board board_ = Board();
    uciOptions options_ = uciOptions();
    Datagen::TrainingData datagen_ = Datagen::TrainingData();

    Movelist searchmoves_;

    int thread_count_;
    bool use_tb_;

    /// @brief parse custom engine commands
    /// @param argc
    /// @param argv
    /// @param options_
    /// @return true if we should terminate after processing the command
    bool parseArgs(int argc, char **argv, uciOptions options_);

    void processCommand(std::string command);

    void uciInput();

    void isreadyInput();

    void ucinewgameInput();

    void quit();

    const std::string getVersion();

    void uciMoves(const std::vector<std::string> &tokens);

    void startSearch(const std::vector<std::string> &tokens, const std::string &command);

    void setPosition(const std::vector<std::string> &tokens, const std::string &command);
};

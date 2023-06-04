#pragma once

#include "board.h"
#include "datagen.h"
#include "timemanager.h"
#include "ucioptions.h"

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

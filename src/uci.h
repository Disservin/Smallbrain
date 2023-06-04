#pragma once

#include "board.h"
#include "datagen.h"
#include "timemanager.h"
#include "ucioptions.h"

class UCI {
   public:
    int uciLoop();

   private:
    Board board_ = Board(DEFAULT_POS);
    uciOptions options_ = uciOptions();

    Movelist searchmoves_;

    int thread_count_ = 1;
    bool use_tb_ = false;

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

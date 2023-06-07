#pragma once

#include "board.h"
#include "options.h"
#include "timemanager.h"
#include "movegen.h"

namespace uci {
class Uci {
   public:
    Uci();

    void uciLoop();

    void processLine(const std::string& line);

    void uci();

    void setOption(const std::string& line);
    void applyOptions();

    void isReady();

    void uciNewGame();
    void position(const std::string& line);

    void go(const std::string& line);

    void stop();
    void quit();

   private:
    uci::Options options_;
    Board board_;

    Movelist searchmoves_;

    std::size_t worker_threads_ = 1;
    uint64_t hash_size_ = 16;

    bool use_tb_ = false;
};

Move uciToMove(const Board& board, const std::string& input);
std::string moveToUci(Move move, bool chess960);
}  // namespace uci

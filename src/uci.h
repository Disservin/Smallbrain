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
    U64 hash_size_ = 16;

    bool use_tb_ = false;
};

Move uciToMove(const Board& board, const std::string& input);
std::string moveToUci(Move move, bool chess960);

/// @brief adjust the outputted score
/// @param score
/// @return a new score used for uci output
std::string convertScore(int score);

/// @brief prints the new uci info
/// @param score
/// @param depth
/// @param seldepth
/// @param nodes
/// @param tbHits
/// @param time
/// @param pv
void output(int score, int depth, uint8_t seldepth, U64 nodes, U64 tbHits, int time,
            const std::string& pv, int hashfull);
}  // namespace uci

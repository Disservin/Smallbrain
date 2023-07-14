#pragma once

#include "board.h"
#include "movegen.h"
#include "options.h"
#include "timemanager.h"

namespace uci {

class Uci {
   public:
    Uci();

    void uciLoop();

    void processLine(const std::string& line);

    void uci();

    void setOption(const std::string& line);
    void applyOptions();

    static void isReady();

    void uciNewGame();
    void position(const std::string& line);

    void go(const std::string& line);

    static void stop();
    static void quit();

   private:
    Board board_;

    Movelist searchmoves_;

    int worker_threads_ = 1;

    bool use_tb_ = false;
};

[[nodiscard]] int modelWinRate(int v, int ply);

[[nodiscard]] Move uciToMove(const Board& board, const std::string& input);
[[nodiscard]] std::string moveToUci(Move move, bool chess960);

[[nodiscard]] std::string convertScore(int score);

void output(int score, int ply, int depth, uint8_t seldepth, U64 nodes, U64 tbHits, int time,
            const std::string& pv, int hashfull);
}  // namespace uci

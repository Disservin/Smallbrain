#pragma once

#include <cstring>
#include <iomanip>  // std::setprecision
#include <memory>   // unique_ptr

#include "board.h"
#include "search.h"

namespace datagen {

struct fenData {
    std::string fen;
    Score score;
    Move move;
};

enum class Side { WHITE, BLACK, DRAW, NONE, TB_CHECK };

std::string stringFenData(const fenData &fen_data, double score);

class TrainingData {
    std::vector<std::string> opening_book_;

    std::atomic_bool stop_ = false;

    bool use_tb_ = false;
    Limits limit_ = {};

   public:
    ~TrainingData() {
        stop_ = true;
        for (auto &thread : threads) {
            thread.join();
        }
    }

    /// @brief entry function
    /// @param workers
    /// @param book
    /// @param depth
    void generate(int workers = 4, std::string book = "", int depth = 7, int nodes = 0,
                  bool use_tb = false);

    /// @brief repeats infinite random playouts
    /// @param threadId
    /// @param book
    /// @param depth
    void infinitePlay(int threadId, int depth, int nodes);

    Board randomStart();

    Side makeMove(Search &search, std::vector<fenData> &fens, int &win_count, int &draw_count,
                  int ply);

    /// @brief starts one selfplay game
    /// @param file
    /// @param depth
    /// @param board
    /// @param Movelist
    /// @param search
    void randomPlayout(std::ofstream &file);

    std::vector<std::thread> threads;
};

}  // namespace datagen
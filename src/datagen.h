#pragma once

#include <cstring>
#include <iomanip>  // std::setprecision
#include <memory>   // unique_ptr

#include "board.h"
#include "search.h"

extern std::atomic_bool UCI_FORCE_STOP;

namespace datagen {

struct fenData {
    std::string fen;
    Score score;
    Move move;
};

std::string stringFenData(const fenData &fen_data, double score);

class TrainingData {
    std::vector<std::string> opening_book_;

   public:
    /// @brief entry function
    /// @param workers
    /// @param book
    /// @param depth
    void generate(int workers = 4, std::string book = "", int depth = 7, int nodes = 0,
                  bool use_tb = false, int rand_limit = 0);

    /// @brief repeats infinite random playouts
    /// @param threadId
    /// @param book
    /// @param depth
    void infinitePlay(int threadId, int depth, int nodes, int rand_limit, bool use_tb);

    /// @brief starts one selfplay game
    /// @param file
    /// @param depth
    /// @param board
    /// @param Movelist
    /// @param search
    void randomPlayout(std::ofstream &file, Board &board, Movelist &movelist,
                       std::unique_ptr<Search> &search, int rand_limit, bool use_tb);
    std::vector<std::thread> threads;
};

}  // namespace datagen
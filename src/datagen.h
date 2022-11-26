#pragma once

#include "board.h"
#include "movegen.h"
#include "randomFen.h"
#include "search.h"
#include "syzygy/Fathom/src/tbprobe.h"
#include "types.h"

#include <cstring>
#include <iomanip> // std::setprecision
#include <random>

extern std::atomic<bool> useTB;
extern std::atomic<bool> UCI_FORCE_STOP;

namespace Datagen
{

struct fenData
{
    std::string fen;
    Score score;
    Move move;
    bool use;
};

std::string stringFenData(fenData fenData, double score);

class TrainingData
{
    std::vector<std::string> openingBook;

  public:
    /// @brief entry function
    /// @param workers
    /// @param book
    /// @param depth
    void generate(int workers = 4, std::string book = "", int depth = 7);

    /// @brief repeats infinite random playouts
    /// @param threadId
    /// @param book
    /// @param depth
    void infinitePlay(int threadId, int depth, int number_of_lines);

    /// @brief starts one selfplay game
    /// @param file
    /// @param depth
    /// @param board
    /// @param Movelist
    /// @param search
    /// @param td
    void randomPlayout(std::ofstream &file, Board &board, Movelist &movelist, Search &search, ThreadData &td, int depth,
                       int number_of_lines);
    std::vector<std::thread> threads;
};

} // namespace Datagen
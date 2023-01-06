#pragma once

#include <cstring>
#include <iomanip> // std::setprecision
#include <random>

#include "board.h"
#include "movegen.h"
#include "randomFen.h"
#include "search.h"
#include "syzygy/Fathom/src/tbprobe.h"
#include "types.h"

extern std::atomic_bool UCI_FORCE_STOP;

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
    void generate(int workers = 4, std::string book = "", int depth = 7, bool useTB = false);

    /// @brief repeats infinite random playouts
    /// @param threadId
    /// @param book
    /// @param depth
    void infinitePlay(int threadId, int depth, bool useTB);

    /// @brief starts one selfplay game
    /// @param file
    /// @param depth
    /// @param board
    /// @param Movelist
    /// @param search
    void randomPlayout(std::ofstream &file, Board &board, Movelist &movelist, Search &search, int depth, bool useTB);
    std::vector<std::thread> threads;
};

} // namespace Datagen
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
  public:
    void generate(int workers = 4, std::string book = "", int depth = 7);
    void infinitePlay(int threadId, std::string book, int depth);
    void randomPlayout(std::ofstream &file, int threadId, std::string &bookm, int depth, int numLines);
    std::vector<std::thread> threads;
};

} // namespace Datagen
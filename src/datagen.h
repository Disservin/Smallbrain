#pragma once

#include "board.h"
#include "movegen.h"
#include "randomFen.h"
#include "search.h"
#include "syzygy/Fathom/src/tbprobe.h"
#include "types.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iomanip> // std::setprecision
#include <random>
#include <thread>
#include <vector>

extern std::atomic<bool> useTB;

struct fenData
{
    std::string fen;
    Score score;
    Move move;
    bool use;
};

extern std::atomic<bool> UCI_FORCE_STOP;

class Datagen
{
  public:
    void generate(int workers = 4, std::string book = "", int depth = 7, bool additionalEndgame = false);
    void infinitePlay(int threadId, std::string book, int depth, bool additionalEndgame);
    void randomPlayout(std::ofstream &file, int threadId, std::string &bookm, int depth, bool additionalEndgame);
    std::vector<std::thread> threads;
};

std::string stringFenData(fenData fenData, double score);
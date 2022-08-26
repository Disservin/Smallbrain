#pragma once

#include "board.h"
#include "search.h"
#include "types.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iomanip> // std::setprecision
#include <random>
#include <thread>
#include <vector>

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
    void generate(int workers = 4);
    void infinitePlay(int threadId);
    void randomPlayout(std::ofstream &file, int threadId);
    std::vector<std::thread> threads;
};

std::string stringFenData(fenData fenData, double score);
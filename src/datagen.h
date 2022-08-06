#include "board.h"
#include "search.h"
#include "types.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iomanip> // std::setprecision
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
    void infinite_play(int threadId);
    void randomPlayout(int threadId);
    std::vector<std::thread> threads;
};

std::string stringFenData(fenData fenData, Color ws, Color stm);

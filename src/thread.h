#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

#include "board.h"
#include "search.h"

// A wrapper class to start the search
class Thread
{
  public:
    void start_thinking();

    Search search;
};

// Holds all currently running threads and their data
class ThreadPool
{
  public:
    std::atomic_bool stop;

    std::vector<Thread> pool;
    std::vector<std::thread> runningThreads;

    uint64_t getNodes();
    uint64_t getTbHits();

    void start_threads(const Board &board, const Limits &limit, const Movelist &searchmoves, int workerCount,
                       bool useTB);

    void stop_threads();
};
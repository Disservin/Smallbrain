#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

#include "search.h"

// A wrapper class to start the search
class Thread
{
  public:
    void start_thinking();
    std::unique_ptr<Search> search;

    Thread()
    {
        search = std::make_unique<Search>();
    }

    Thread(const Thread &other)
    {
        search = std::make_unique<Search>(*other.search);
    }

    Thread &operator=(const Thread &other)
    {
        // protect against invalid self-assignment
        if (this != &other)
        {
            search = std::make_unique<Search>(*other.search);
        }
        // by convention, always return *this
        return *this;
    }

    ~Thread() = default;
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
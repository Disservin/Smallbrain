#include <iostream>

#include "thread.h"

extern std::atomic_bool UCI_FORCE_STOP;

void Thread::start_thinking()
{
    search->startThinking();
}

uint64_t ThreadPool::getNodes()
{
    uint64_t total = 0;

    for (auto &th : pool)
    {
        total += th.search->nodes;
    }

    return total;
}

uint64_t ThreadPool::getTbHits()
{
    uint64_t total = 0;

    for (auto &th : pool)
    {
        total += th.search->tbhits;
    }

    return total;
}

void ThreadPool::start_threads(const Board &board, const Limits &limit, const Movelist &searchmoves, int workerCount,
                               bool use_tb)
{
    assert(runningThreads.size() == 0);

    stop = false;

    Thread mainThread;

    if (pool.size() > 0)
        mainThread = pool[0];

    pool.clear();

    // update with info
    mainThread.search->id = 0;
    mainThread.search->board = board;
    mainThread.search->limit = limit;
    mainThread.search->use_tb = use_tb;
    mainThread.search->nodes = 0;
    mainThread.search->tbhits = 0;
    mainThread.search->spent_effort.reset();
    mainThread.search->searchmoves = searchmoves;

    pool.emplace_back(mainThread);

    // start at index 1 to keep "mainthread" data alive
    mainThread.search->consthist.reset();
    mainThread.search->history.reset();
    mainThread.search->counters.reset();

    for (int i = 1; i < workerCount; i++)
    {
        mainThread.search->id = i;

        pool.emplace_back(mainThread);
    }

    for (int i = 0; i < workerCount; i++)
    {
        runningThreads.emplace_back(&Thread::start_thinking, std::ref(pool[i]));
    }
}

void ThreadPool::stop_threads()
{
    stop = UCI_FORCE_STOP = true;

    for (auto &th : runningThreads)
        if (th.joinable())
            th.join();

    pool.clear();
    runningThreads.clear();
}

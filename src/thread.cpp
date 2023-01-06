#include "thread.h"

#include <iostream>

void Thread::start_thinking()
{
    search.startThinking();
}

uint64_t ThreadPool::getNodes()
{
    uint64_t total = 0;

    for (auto &th : pool)
    {
        total += th.search.nodes;
    }

    return total;
}

uint64_t ThreadPool::getTbHits()
{
    uint64_t total = 0;

    for (auto &th : pool)
    {
        total += th.search.nodes;
    }

    return total;
}

void ThreadPool::start_threads(const Board &board, const Limits &limit, int workerCount, bool useTB)
{
    assert(runningThreads.size() == 0);

    stopped = false;

    Thread mainThread;
    if (pool.size() > 0)
        mainThread = pool[0];

    pool.clear();

    // update with info
    mainThread.search.id = 0;
    mainThread.search.board = board;
    mainThread.search.limit = limit;
    mainThread.search.useTB = useTB;
    mainThread.search.nodes = 0;
    mainThread.search.tbhits = 0;

    pool.push_back(mainThread);

    // start at index 1 to keep "mainthread" data alive
    for (int i = 1; i < workerCount; i++)
    {
        Thread th;
        th.search.id = i;
        th.search.board = board;
        th.search.limit = limit;
        th.search.useTB = useTB;
        th.search.nodes = 0;
        th.search.tbhits = 0;

        pool.push_back(th);
    }

    for (int i = 0; i < workerCount; i++)
    {
        runningThreads.emplace_back(&Thread::start_thinking, std::ref(pool[i]));
    }
}

void ThreadPool::stop_threads()
{
    stopped = true;

    for (auto &th : runningThreads)
        if (th.joinable())
            th.join();
}

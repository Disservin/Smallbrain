#include <iostream>

#include "thread.h"

void SearchInstance::start() const { search->startThinking(); }

U64 ThreadPool::getNodes() const {
    U64 total = 0;

    for (auto &th : pool_) {
        total += th.search->nodes;
    }

    return total;
}

U64 ThreadPool::getTbHits() const {
    U64 total = 0;

    for (auto &th : pool_) {
        total += th.search->tbhits;
    }

    return total;
}

void ThreadPool::start(const Board &board, const Limits &limit, const Movelist &searchmoves,
                       int worker_count, bool use_tb) {
    assert(running_threads_.size() == 0);

    stop = false;

    SearchInstance mainThread;

    if (!pool_.empty()) mainThread = pool_[0];

    pool_.clear();

    // update with info
    mainThread.search->id = 0;
    mainThread.search->board = board;
    mainThread.search->limit = limit;
    mainThread.search->use_tb = use_tb;
    mainThread.search->nodes = 0;
    mainThread.search->tbhits = 0;
    mainThread.search->node_effort.reset();
    mainThread.search->searchmoves = searchmoves;

    pool_.emplace_back(mainThread);

    // start at index 1 to keep "mainthread" data alive
    mainThread.search->consthist.reset();
    mainThread.search->history.reset();
    mainThread.search->counters.reset();

    for (int i = 1; i < worker_count; i++) {
        mainThread.search->id = i;

        pool_.emplace_back(mainThread);
    }

    for (int i = 0; i < worker_count; i++) {
        running_threads_.emplace_back(&SearchInstance::start, std::ref(pool_[i]));
    }
}

void ThreadPool::kill() {
    stop = true;

    for (auto &th : running_threads_)
        if (th.joinable()) th.join();

    pool_.clear();
    running_threads_.clear();
}

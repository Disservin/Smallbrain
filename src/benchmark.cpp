#include "benchmark.h"
#include "thread.h"

extern ThreadPool Threads;

namespace Bench
{

int startBench(int depth)
{
    U64 totalNodes = 0;

    Limits limit;
    limit.depth = depth;
    limit.nodes = 0;
    limit.time = Time();

    int i = 1;

    auto t1 = TimePoint::now();

    for (auto &fen : benchmarkfens)
    {
        std::cout << "\nPosition: " << i++ << "/" << benchmarkfens.size() << " " << fen << std::endl;

        Threads.stop = false;

        std::unique_ptr<Search> searcher = std::make_unique<Search>();

        searcher->id = 0;
        searcher->useTB = false;
        searcher->board.applyFen(fen);
        searcher->limit = limit;
        searcher->id = 0;

        searcher->startThinking();

        totalNodes += searcher->nodes;
    }

    auto t2 = TimePoint::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << "\n" << totalNodes << " nodes " << signed((totalNodes / (ms + 1)) * 1000) << " nps " << std::endl;

    print_mean();

    return 0;
}

} // namespace Bench
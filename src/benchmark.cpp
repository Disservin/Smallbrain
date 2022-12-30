#include "benchmark.h"

namespace Bench
{

int startBench()
{
    U64 totalNodes = 0;

    Limits limit;
    limit.depth = 12;
    limit.nodes = 0;
    limit.time = Time();

    int i = 1;

    auto t1 = TimePoint::now();

    for (auto &fen : benchmarkfens)
    {
        std::cout << "\nPosition: " << i++ << "/" << benchmarkfens.size() << " " << fen << std::endl;
        stopped = false;

        ThreadData td;
        td.id = 0;
        td.useTB = false;
        td.board.applyFen(fen);

        Search searcher = Search();
        searcher.tds.emplace_back(td);
        searcher.iterativeDeepening(limit, 0);

        totalNodes += searcher.tds[0].nodes;
    }

    auto t2 = TimePoint::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << "\n" << totalNodes << " nodes " << signed((totalNodes / (ms + 1)) * 1000) << " nps " << std::endl;

    print_mean();

    return 0;
}

} // namespace Bench
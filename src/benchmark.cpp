#include "benchmark.h"

namespace Bench
{

int startBench()
{
    U64 totalNodes = 0;
    Search searcher = Search();
    Time t;
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

        searcher.tds.emplace_back(td);

        searcher.iterativeDeepening(12, 0, t, 0);

        totalNodes += searcher.tds[0].nodes;
        searcher.tds.clear();
    }

    auto t2 = TimePoint::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\n" << totalNodes << " nodes " << signed((totalNodes / (ms + 1)) * 1000) << " nps " << std::endl;

    print_mean();

    return 0;
}

} // namespace Bench
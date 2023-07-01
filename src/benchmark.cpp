#include "benchmark.h"
#include "search.h"
#include "thread.h"

extern ThreadPool Threads;

namespace bench {

int startBench(int depth) {
    U64 total_nodes = 0;

    Limits limit;
    limit.depth = depth;
    limit.nodes = 0;
    limit.time = Time();

    int i = 1;

    auto t1 = TimePoint::now();

    for (auto &fen : benchmarkfens) {
        std::cout << "\nPosition: " << i++ << "/" << benchmarkfens.size() << " " << fen
                  << std::endl;

        Threads.stop = false;

        std::unique_ptr<Search> searcher = std::make_unique<Search>();

        searcher->id = 0;
        searcher->use_tb = false;
        searcher->board.setFen(fen);
        searcher->limit = limit;
        searcher->id = 0;

        searcher->startThinking();

        total_nodes += searcher->nodes;
    }

    auto t2 = TimePoint::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << "\n"
              << total_nodes << " nodes " << signed((total_nodes / (ms + 1)) * 1000) << " nps "
              << std::endl;

    print_mean();

    return 0;
}

}  // namespace bench
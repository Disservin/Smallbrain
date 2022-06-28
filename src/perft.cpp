#include "perft.h"

void Perft::perf_Test(int depth, int max) {
    auto t1 = std::chrono::high_resolution_clock::now();
    perft_function(depth, max);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\ntime: " << ms << "ms" << std::endl;
    std::cout << "Nodes: " << nodes << " nps " << (nodes / (ms + 1)) * 1000 << std::endl;
}

void Perft::testAllPos() {
    auto t1 = std::chrono::high_resolution_clock::now();
    U64 total = 0;
    int passed = 0;
    board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    nodes = 0;
    perf_Test(6, 6);
    total += nodes;
    if (nodes == 119060324) {
        passed++;
        std::cout << "Startpos: passed" << std::endl;
    }
    else std::cout << "Startpos: failed" << std::endl;
    board.applyFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    nodes = 0;
    perf_Test(5, 5);
    total += nodes;
    if (nodes == 193690690) {
        passed++;
        std::cout << "Kiwipete: passed" << std::endl;
    }
    else std::cout << "Kiwipete: failed" << std::endl;
    board.applyFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ");
    nodes = 0;
    perf_Test(7, 7);
    total += nodes;
    if (nodes == 178633661) {
        passed++;
        std::cout << "Pos 3: passed" << std::endl;
    }
    else std::cout << "Pos 3: failed" << std::endl;
    board.applyFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    nodes = 0;
    perf_Test(6, 6);
    total += nodes;
    if (nodes == 706045033) {
        passed++;
        std::cout << "Pos 4: passed" << std::endl;
    }
    else std::cout << "Pos 4: failed" << std::endl;
    board.applyFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ");
    nodes = 0;
    perf_Test(5, 5);
    total += nodes;
    if (nodes == 89941194) {
        passed++;
        std::cout << "Pos 5: passed" << std::endl;
    }
    else std::cout << "Pos 5: failed" << std::endl;
    board.applyFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ");
    nodes = 0;
    perf_Test(5, 5);
    total += nodes;
    if (nodes == 164075551) {
        passed++;
        std::cout << "Pos 6: passed" << std::endl;
    }
    else std::cout << "Pos 6: failed" << std::endl;
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\n";
    std::cout << "Total time (ms)    : " << ms << std::endl;
    std::cout << "Nodes searched     : " << total << std::endl;
    std::cout << "Nodes/second       : " << (total / (ms + 1)) * 1000 << std::endl;
    std::cout << "Correct Positions  : " << passed << "/" << "6" << std::endl;
}

U64 Perft::perft_function(int depth, int max) {
    Movelist ml = board.legalmoves();
    if (depth == 1) {
        return ml.size;
    }
    U64 nodesIt = 0;
    for (int i = 0; i < ml.size; i++) {
        Move move = ml.list[i];
        board.makeMove(move);
        nodesIt += perft_function(depth - 1, depth);
        board.unmakeMove(move);
        if (depth == max) {
            nodes += nodesIt;
            std::cout << board.printMove(move) << " " << nodesIt << std::endl;
            nodesIt = 0;
        }
    }
    return nodesIt;
}
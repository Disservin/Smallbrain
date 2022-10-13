#include "perft.h"

#include <vector>

void Perft::perfTest(int depth, int max)
{
    auto t1 = TimePoint::now();
    perftFunction(depth, max);
    auto t2 = TimePoint::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\ntime: " << ms << "ms" << std::endl;
    std::cout << "Nodes: " << nodes << " nps " << ((nodes * 1000) / ms) << std::endl;
}

void Perft::testAllPos()
{
    auto t1 = TimePoint::now();
    U64 total = 0;
    size_t passed = 0;
    std::string tests[] = {DEFAULT_POS,
                           "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                           "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                           "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
                           "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                           "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ",
                           "4r3/bpk5/5n2/2P1P3/8/4K3/8/8 w - - 0 1"};
    int depths[] = {6, 5, 7, 6, 5, 5, 7};
    std::vector<U64> expected = {119060324, 193690690, 178633661, 706045033, 89941194, 164075551, 71441619};
    for (size_t i = 0; i < expected.size(); i++)
    {
        board.applyFen(tests[i]);
        nodes = 0;
        perfTest(depths[i], depths[i]);
        total += nodes;
        if (nodes == expected[i])
        {
            passed++;
            std::cout << "Position " << i + 1 << ": passed" << std::endl;
        }
        else
            std::cout << "Position " << i + 1 << ": failed" << std::endl;
    }

    auto t2 = TimePoint::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\n";
    std::cout << "Total time (ms)    : " << ms << std::endl;
    std::cout << "Nodes searched     : " << total << std::endl;
    std::cout << "Nodes/second       : " << ((total * 1000) / ms) << std::endl;
    std::cout << "Correct Positions  : " << passed << "/" << expected.size() << std::endl;
    if (passed != expected.size())
        exit(1);
}

U64 Perft::perftFunction(int depth, int max)
{
    Movelist moves;
    Movegen::legalmoves<ALL>(board, moves);
    if (depth == 0)
        return 1;
    else if (depth == 1 && max != 1)
    {
        return moves.size;
    }
    U64 nodesIt = 0;
    for (int i = 0; i < moves.size; i++)
    {
        Move move = moves[i].move;
        board.makeMove<false>(move);
        nodesIt += perftFunction(depth - 1, depth);
        board.unmakeMove<false>(move);
        if (depth == max)
        {
            nodes += nodesIt;
            std::cout << uciRep(move) << " " << nodesIt << std::endl;
            nodesIt = 0;
        }
    }
    return nodesIt;
}
#include "perft.h"

#include <vector>

void Perft::perfTest(int depth, int max)
{
    auto t1 = TimePoint::now();
    perftFunction(depth, max);
    auto t2 = TimePoint::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\ntime: " << ms << "ms" << std::endl;
    std::cout << "Nodes: " << nodes << " nps " << ((nodes * 1000) / (ms + 1)) << std::endl;
}

void Perft::testAllPos(int n)
{
    U64 mnps = 0;
    for (int runs = 0; runs < n; runs++)
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
                               "4r3/bpk5/5n2/2P1P3/8/4K3/8/8 w - - 0 1",
                               "b2r4/2q3k1/p5p1/P1r1pp2/R1pnP2p/4NP1P/1PP2RPK/Q4B2 w - - 2 29",
                               "3n1k2/8/4P3/8/8/8/8/2K1R3 w - - 0 1"};

        std::string tests960[] = {DEFAULT_POS,
                                  "1rqbkrbn/1ppppp1p/1n6/p1N3p1/8/2P4P/PP1PPPP1/1RQBKRBN w FBfb - 0 9",
                                  "rbbqn1kr/pp2p1pp/6n1/2pp1p2/2P4P/P7/BP1PPPP1/R1BQNNKR w HAha - 0 9",
                                  "rqbbknr1/1ppp2pp/p5n1/4pp2/P7/1PP5/1Q1PPPPP/R1BBKNRN w GAga - 0 9",
                                  "4rrb1/1kp3b1/1p1p4/pP1Pn2p/5p2/1PR2P2/2P1NB1P/2KR1B2 w D - 0 21",
                                  "1rkr3b/1ppn3p/3pB1n1/6q1/R2P4/4N1P1/1P5P/2KRQ1B1 b Dbd - 0 14"};

        int depths[] = {6, 5, 7, 6, 5, 5, 7, 6, 8};
        int depths960[] = {6, 6, 6, 6, 6, 6};

        std::vector<U64> expected = {119060324ull, 193690690ull, 178633661ull,  706045033ull, 89941194ull,
                                     164075551ull, 71441619ull,  2261050076ull, 437319625ull};

        std::vector<U64> expected960 = {119060324ull, 191762235ull, 924181432ull,
                                        308553169ull, 872323796ull, 2678022813ull};
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
            {
                std::cout << "Position " << i + 1 << ": failed" << std::endl;
                exit(4);
            }
        }

        board.chess960 = true;
        for (size_t i = 0; i < expected960.size(); i++)
        {
            board.applyFen(tests960[i]);
            nodes = 0;
            perfTest(depths960[i], depths960[i]);
            total += nodes;
            if (nodes == expected960[i])
            {
                passed++;
                std::cout << "Position960 " << i + 1 << ": passed" << std::endl;
            }
            else
            {

                std::cout << "Position960 " << i + 1 << ": failed" << std::endl;
                exit(4);
            }
        }

        auto t2 = TimePoint::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        mnps += ((total * 1000) / ms);
        std::cout << "\n";
        std::cout << "Total time (ms)    : " << ms << std::endl;
        std::cout << "Nodes searched     : " << total << std::endl;
        std::cout << "Nodes/second       : " << ((total * 1000) / ms) << std::endl;
        std::cout << "Correct Positions  : " << passed << "/" << expected.size() + expected960.size() << std::endl;
    }
    std::cout << "Avg Nodes/second   : " << mnps / n << std::endl;
}

U64 Perft::perftFunction(int depth, int max)
{
    movelists[depth].size = 0;
    Movegen::legalmoves<Gentype::ALL>(board, movelists[depth]);
    if (depth == 0)
        return 1;
    else if (depth == 1 && max != 1)
    {
        return movelists[depth].size;
    }
    U64 nodesIt = 0;
    for (int i = 0; i < movelists[depth].size; i++)
    {
        Move move = movelists[depth][i].move;
        assert(board.isPseudoLegal(move) && board.isLegal(move));
        board.makeMove<false>(move);
        nodesIt += perftFunction(depth - 1, depth);
        board.unmakeMove<false>(move);
        if (depth == max)
        {
            nodes += nodesIt;
            std::cout << uciRep(board, move) << " " << nodesIt << std::endl;
            nodesIt = 0;
        }
    }
    return nodesIt;
}
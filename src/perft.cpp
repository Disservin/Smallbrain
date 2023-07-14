#include <vector>

#include "movegen.h"
#include "perft.h"
#include "uci.h"

U64 PerftTesting::perftFunction(int depth, int max_depth) {
    movelists[depth].size = 0;
    movegen::legalmoves<Movetype::ALL>(board, movelists[depth]);

    if (depth == 0)
        return 1;
    else if (depth == 1 && max_depth != 1) {
        return movelists[depth].size;
    }

    U64 nodes_it = 0;
    for (auto extmove : movelists[depth]) {
        Move move = extmove.move;
        board.makeMove<false>(move);
        nodes_it += perftFunction(depth - 1, depth);
        board.unmakeMove<false>(move);
        if (depth == max_depth) {
            nodes += nodes_it;
            std::cout << uci::moveToUci(move, board.chess960) << " " << nodes_it << std::endl;
            nodes_it = 0;
        }
    }
    return nodes_it;
}

void PerftTesting::perfTest(int depth, int max_depth) {
    auto t1 = TimePoint::now();
    perftFunction(depth, max_depth);
    auto t2 = TimePoint::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\ntime: " << ms << "ms" << std::endl;
    std::cout << "Nodes: " << nodes << " nps " << ((nodes * 1000) / (ms + 1)) << std::endl;
}

struct Test {
    std::string fen;
    uint64_t expected_node_count;
    int depth;
};

void PerftTesting::testAllPos(int n) {
    U64 mnps = 0;
    for (int runs = 0; runs < n; runs++) {
        auto t1 = TimePoint::now();
        U64 total = 0;
        size_t passed = 0;

        const Test test_positions[] = {
            {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3195901860, 7},
            {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", 193690690, 5},
            {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ", 178633661, 7},
            {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 706045033, 6},
            {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 89941194, 5},
            {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 1", 164075551,
             5},
            {"4r3/bpk5/5n2/2P1P3/8/4K3/8/8 w - - 0 1", 71441619ull, 7},
            {"b2r4/2q3k1/p5p1/P1r1pp2/R1pnP2p/4NP1P/1PP2RPK/Q4B2 w - - 2 29", 2261050076ull, 6},
            {"3n1k2/8/4P3/8/8/8/8/2K1R3 w - - 0 1", 437319625ull, 8}};

        const Test test_positions_960[] = {
            {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w AHah - 0 1", 119060324ull, 6},
            {"1rqbkrbn/1ppppp1p/1n6/p1N3p1/8/2P4P/PP1PPPP1/1RQBKRBN w FBfb - 0 9", 191762235ull, 6},
            {"rbbqn1kr/pp2p1pp/6n1/2pp1p2/2P4P/P7/BP1PPPP1/R1BQNNKR w HAha - 0 9", 924181432ull, 6},
            {"rqbbknr1/1ppp2pp/p5n1/4pp2/P7/1PP5/1Q1PPPPP/R1BBKNRN w GAga - 0 9", 308553169ull, 6},
            {"4rrb1/1kp3b1/1p1p4/pP1Pn2p/5p2/1PR2P2/2P1NB1P/2KR1B2 w D - 0 21", 872323796ull, 6},
            {"1rkr3b/1ppn3p/3pB1n1/6q1/R2P4/4N1P1/1P5P/2KRQ1B1 b Dbd - 0 14", 2678022813ull, 6},
            {"qbbnrkr1/p1pppppp/1p4n1/8/2P5/6N1/PPNPPPPP/1BRKBRQ1 b FCge - 1 3", 521301336ull, 6},
            {"rr6/2kpp3/1ppnb1p1/p4q1p/P4P1P/1PNN2P1/2PP2Q1/1K2RR2 w E - 1 19", 2998685421ull, 6}};

        int i = 0;
        for (const auto& test : test_positions) {
            nodes = 0;

            board.setFen(test.fen);

            perfTest(test.depth, test.depth);

            total += nodes;
            if (nodes == test.expected_node_count) {
                passed++;
                std::cout << "Position " << i + 1 << ": passed" << std::endl;
            } else {
                std::cout << "Position " << i + 1 << ": failed" << std::endl;
                exit(4);
            }
            i++;
        }

        board.chess960 = true;
        i = 0;

        for (const auto& test : test_positions_960) {
            nodes = 0;

            board.setFen(test.fen);

            perfTest(test.depth, test.depth);

            total += nodes;
            if (nodes == test.expected_node_count) {
                passed++;
                std::cout << "FRC Position " << i + 1 << ": passed" << std::endl;
            } else {
                std::cout << "FRC Position " << i + 1 << ": failed" << std::endl;
                exit(4);
            }
            i++;
        }

        auto t2 = TimePoint::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        mnps += ((total * 1000) / ms);
        std::cout << "\n";
        std::cout << "Total time (ms)    : " << ms << std::endl;
        std::cout << "Nodes searched     : " << total << std::endl;
        std::cout << "Nodes/second       : " << ((total * 1000) / ms) << std::endl;
        std::cout << "Correct Positions  : " << passed << "/"
                  << sizeof(test_positions) / sizeof(Test) +
                         sizeof(test_positions_960) / sizeof(Test)
                  << std::endl;
    }
    std::cout << "Avg Nodes/second   : " << mnps / n << std::endl;
}

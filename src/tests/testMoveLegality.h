#pragma once

#include "tests.h"

bool testIsPseudoLegalAndIsLegal(const std::string &fen)
{
    Board b;
    b.applyFen(fen);

    Movelist moves;
    Movegen::legalmoves<Movetype::ALL>(b, moves);

    for (size_t i = 0; i < 65536; i++)
    {
        Move m = Move(i);

        if (moves.find(m) == -1 && (b.isPseudoLegal(m) && b.isLegal(m)))
        {
            std::cout << m << uciMove(m, b.chess960) << std::endl;
            return false;
        }
        else if (moves.find(m) >= 0 && !(b.isPseudoLegal(m) && b.isLegal(m)))
        {
            std::cout << m << uciMove(m, b.chess960) << std::endl;
            return false;
        }
    }

    return true;
}

void testAllMoveLegality()
{
    std::vector<std::string> tests = {DEFAULT_POS,
                                      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                                      "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                                      "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
                                      "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                                      "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ",
                                      "4r3/bpk5/5n2/2P1P3/8/4K3/8/8 w - - 0 1",
                                      "b2r4/2q3k1/p5p1/P1r1pp2/R1pnP2p/4NP1P/1PP2RPK/Q4B2 w - - 2 29",
                                      "3n1k2/8/4P3/8/8/8/8/2K1R3 w - - 0 1",
                                      "r1bq1rk1/bpp2ppp/p1np1n2/6B1/PPN1P3/2PB1N1P/5PP1/R2Q1RK1 b - - 4 12",
                                      "r2q1rk1/3nbppp/p2pbn2/4p1P1/1p2P3/1N2BP2/PPPQN2P/2KR1B1R b - - 1 13",
                                      "4kbnr/5ppp/4p3/1R1pnb2/8/6P1/5P1P/2B1K2R b Kk - 0 20",
                                      "3r2k1/2r2ppp/p3p1b1/B2p4/3NnP2/P7/6PP/4QR1K b - - 2 25"
                                      "4nk2/pp1r1pp1/4b2n/1Pp5/P1P3PB/5N1P/8/KB2R3 w - - 3 38",
                                      "3n4/5k2/6p1/1P1b2P1/P7/2K5/8/4R3 b - - 2 52",
                                      "r1bqk1r1/1p1p1n2/p1n2pN1/2p1b2Q/2P1Pp2/1PN5/PB4PP/R4RK1 w q - 0 1"};

    std::vector<std::string> tests960 = {DEFAULT_POS,
                                         "1rqbkrbn/1ppppp1p/1n6/p1N3p1/8/2P4P/PP1PPPP1/1RQBKRBN w FBfb - 0 9",
                                         "rbbqn1kr/pp2p1pp/6n1/2pp1p2/2P4P/P7/BP1PPPP1/R1BQNNKR w HAha - 0 9",
                                         "rqbbknr1/1ppp2pp/p5n1/4pp2/P7/1PP5/1Q1PPPPP/R1BBKNRN w GAga - 0 9",
                                         "4rrb1/1kp3b1/1p1p4/pP1Pn2p/5p2/1PR2P2/2P1NB1P/2KR1B2 w D - 0 21",
                                         "1rkr3b/1ppn3p/3pB1n1/6q1/R2P4/4N1P1/1P5P/2KRQ1B1 b Dbd - 0 14"};

    for (auto &fen : tests)
    {
        expect(testIsPseudoLegalAndIsLegal(fen), true, fen);
    }

    for (auto &fen : tests960)
    {
        expect(testIsPseudoLegalAndIsLegal(fen), true, fen);
    }
}
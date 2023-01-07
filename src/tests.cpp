#include "tests.h"
#include "board.h"

namespace Tests
{

bool testAll()
{
    testZobristHash();
    testRepetition();
    testDraw();

    return true;
}

void testFenRepetition(std::string input)
{
    Board board;

    std::vector<std::string> tokens = splitInput(input);

    bool hasMoves = contains(tokens, "moves");

    if (tokens[1] == "fen")
        board.applyFen(input.substr(input.find("fen") + 4), false);
    else
        board.applyFen(DEFAULT_POS, false);

    if (hasMoves)
    {
        std::size_t index = std::find(tokens.begin(), tokens.end(), "moves") - tokens.begin();
        index++;
        for (; index < tokens.size(); index++)
        {
            Move move = convertUciToMove(board, tokens[index]);
            board.makeMove<false>(move);
        }
    }

    board.accumulate();

    assert(board.isRepetition(2));
}

void testRepetition()
{
    std::cout << "Testing repetions" << std::endl;

    std::string input =
        "position fen r2qk2r/pp3pp1/2nbpn1p/8/6b1/2NP1N2/PPP1BPPP/R1BQ1RK1 w - - 0 1 moves h2h3 g4f5 d3d4 e8h8 e2d3 "
        "f5d3 d1d3 a8c8 a2a3 f8e8 f1e1 e6e5 d4e5 c6e5 f3e5 d6e5 d3d8 c8d8 c3d1 f6e4 d1e3 e5d4 g1f1 d4c5 e1d1 d8d1 e3d1 "
        "c5f2 d1f2 e4g3 f1g1 e8e1 g1h2 g3f1 h2g1 f1g3 g1h2 g3f1 h2g1 f1g3 ";

    testFenRepetition(input);

    input = "position fen 6Q1/1p1k3p/3b2p1/p6r/3P1PR1/1PR2NK1/P3q2P/4r3 b - - 14 37 moves e1f1 f3e5 d6e5 g8f7 d7d8 "
            "f7f8 d8d7 f8f7 d7d8 f7f8 d8d7 f8f7";

    testFenRepetition(input);

    input = "position fen rn2k1nr/pp2bppp/2p5/q3p3/2B5/P1N2b1P/1PPP1PP1/R1BQ1RK1 w ah - 0 1 moves d1f3 g8f6 c3e2 e8h8 "
            "e2g3 b8d7 d2d3 a5c7 g3f5 e7d8 c4a2 a7a5 f3g3 f6h5 g3f3 h5f6 f3g3 f6h5 g3f3 h5f6";

    testFenRepetition(input);

    input = "position fen rnbqk2r/1pp2ppp/p2bpn2/8/2NP1B2/3B1N2/PPP2PPP/R2QK2R w AHah - 0 1 moves c4d6 c7d6 c2c4 e8h8 "
            "e1h1 b7b6 f1e1 c8b7 f4g3 f8e8 a1c1 h7h6 d3b1 b6b5 b2b3 b5c4 b3c4 d8b6 c4c5 d6c5 d4c5 b6c6 b1c2 c6d5 g3d6 "
            "e8c8 c2b3 d5d1 e1d1 a6a5 f3d4 a5a4 b3c4 b7d5 a2a3 b8c6 c4d5 f6d5 d4c6 c8c6 c1c4 d5c7 d1b1 c7e8 d6f4 a8c8 "
            "b1b8 c8b8 f4b8 f7f6 g1f1 g8f7 f2f4 g7g5 g2g3 f7g6 f1e2 c6c8 b8a7 e8c7 e2d3 c8d8 d3e4 f6f5 e4e3 d8a8 a7b6 "
            "c7d5 e3d4 a8a6 b6d8 a6a8 d8b6 a8a6 b6d8 a6a8 d8b6";

    testFenRepetition(input);
}

void testZobristHash()
{
    std::cout << "Testing zobrist hash" << std::endl;

    Board b;

    b.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    assert(b.zobristHash() == 0x463b96181691fc9c);

    b.makeMove<false>(convertUciToMove(b, "e2e4"));
    assert(b.zobristHash() == 0x823c9b50fd114196);

    b.makeMove<false>(convertUciToMove(b, "d7d5"));
    assert(b.zobristHash() == 0x0756b94461c50fb0);

    b.makeMove<false>(convertUciToMove(b, "e4e5"));
    assert(b.zobristHash() == 0x662fafb965db29d4);

    b.makeMove<false>(convertUciToMove(b, "f7f5"));
    assert(b.zobristHash() == 0x22a48b5a8e47ff78);

    b.makeMove<false>(convertUciToMove(b, "e1e2"));
    assert(b.zobristHash() == 0x652a607ca3f242c1);

    b.makeMove<false>(convertUciToMove(b, "e8f7"));

    assert(b.zobristHash() == 0x00fdd303c946bdd9);

    b.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    b.makeMove<false>(convertUciToMove(b, "a2a4"));
    b.makeMove<false>(convertUciToMove(b, "b7b5"));
    b.makeMove<false>(convertUciToMove(b, "h2h4"));
    b.makeMove<false>(convertUciToMove(b, "b5b4"));
    b.makeMove<false>(convertUciToMove(b, "c2c4"));

    assert(b.zobristHash() == 0x3c8123ea7b067637);

    b.makeMove<false>(convertUciToMove(b, "b4c3"));
    b.makeMove<false>(convertUciToMove(b, "a1a3"));

    assert(b.zobristHash() == 0x5c3f9b829b279560);
}

void testDraw()
{
    std::cout << "Testing draw detection" << std::endl;

    Board b;

    // KBvkb same colour
    b.applyFen("8/2k1b3/8/8/8/4B3/2K5/8 w - - 0 1");
    assert(b.isDrawn(b.isSquareAttacked(~b.sideToMove, b.KingSQ(b.sideToMove))) == Result::DRAWN);

    // KBvkb different colour
    b.applyFen("8/2k1b3/8/8/8/5B2/2K5/8 w - - 0 1");
    assert(b.isDrawn(b.isSquareAttacked(~b.sideToMove, b.KingSQ(b.sideToMove))) == Result::NONE);

    // Kvkb
    b.applyFen("8/2k1b3/8/8/8/8/2K5/8 w - - 0 1");
    assert(b.isDrawn(b.isSquareAttacked(~b.sideToMove, b.KingSQ(b.sideToMove))) == Result::DRAWN);

    // KBvk
    b.applyFen("8/2k1B3/8/8/8/8/2K5/8 w - - 0 1");
    assert(b.isDrawn(b.isSquareAttacked(~b.sideToMove, b.KingSQ(b.sideToMove))) == Result::DRAWN);

    // KNvk
    b.applyFen("8/2k1N3/8/8/8/8/2K5/8 w - - 0 1");
    assert(b.isDrawn(b.isSquareAttacked(~b.sideToMove, b.KingSQ(b.sideToMove))) == Result::DRAWN);

    // Kvkn
    b.applyFen("8/2k1n3/8/8/8/8/2K5/8 w - - 0 1");
    assert(b.isDrawn(b.isSquareAttacked(~b.sideToMove, b.KingSQ(b.sideToMove))) == Result::DRAWN);

    // Kvk
    b.applyFen("8/2k5/8/8/8/8/2K5/8 w - - 0 1");
    assert(b.isDrawn(b.isSquareAttacked(~b.sideToMove, b.KingSQ(b.sideToMove))) == Result::DRAWN);
}

} // namespace Tests
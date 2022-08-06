#include "datagen.h"

void Datagen::generate(int workers)
{
    for (std::thread &th : threads)
    {
        if (th.joinable())
            th.join();
    }
    threads.clear();

    for (int i = 0; i < workers; i++)
    {
        threads.emplace_back(&Datagen::infinite_play, this, i);
    }
}

void Datagen::infinite_play(int threadId)
{
    while (!UCI_FORCE_STOP)
    {
        randomPlayout(threadId);
    }
}

void Datagen::randomPlayout(int threadId)
{
    Board board;
    board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::vector<fenData> fens;
    Movelist movelist;
    int ply = 0;

    while (ply++ <= 9)
    {
        movelist = board.legalmoves();
        if (movelist.size == 0 || UCI_FORCE_STOP)
            break;

        int index = rand() % movelist.size;
        Move move = movelist.list[index];

        board.makeMove<true>(move);
    }

    Board randomPlayBoard = board;

    Search search = Search();

    ThreadData td;
    td.board = board;
    td.id = 0;
    td.allowPrint = false;
    search.tds.push_back(td);

    constexpr int nodes = 5000;

    Time t;

    ply = 0;
    int drawCount = 0;
    int winCount = 0;

    Color winningSide = NO_COLOR;
    while (true)
    {
        ply++;
        movelist = board.legalmoves();
        if (movelist.size == 0 || UCI_FORCE_STOP || board.isRepetition(3))
        {
            winningSide = (board.checkMask == DEFAULT_CHECKMASK ? ~board.sideToMove : NO_COLOR);
            break;
        }

        int all = popcount(board.All());
        if (all == 2)
        {
            winningSide = NO_COLOR;
            break;
        }
        else if (all == 3 && (board.Bitboards[WhiteKnight] || board.Bitboards[BlackKnight] ||
                              board.Bitboards[WhiteBishop] || board.Bitboards[BlackBishop]))
        {
            winningSide = NO_COLOR;
            break;
        }

        stopped = false;
        SearchResult result = search.iterative_deepening(MAX_PLY, nodes, t, 0);

        // Shouldnt happen
        if (result.score == VALUE_NONE)
        {
            std::cout << "meh" << std::endl;
            board.print();
            // break;
            exit(1);
        }

        if (ply > 300)
        {
            winningSide = NO_COLOR;
            break;
        }

        const bool capture = board.pieceAtB(to(result.move)) != None;
        const bool inCheck = board.checkMask != DEFAULT_CHECKMASK;

        fenData fn;
        fn.fen = board.getFen();
        fn.score = result.score;
        fn.move = result.move;
        fn.use = true;
        fn.use = !(capture || inCheck);

        Score absScore = std::abs(result.score);
        if (absScore >= 0 && absScore <= 10)
        {
            drawCount++;
            winCount = 0;
            fn.use = false;
        }

        else if (absScore > 1500)
        {
            winCount++;
            drawCount = 0;
            fn.use = false;
        }

        if (winCount > 5 || absScore >= VALUE_MATE_IN_PLY)
        {
            if (result.score > 1500)
                winningSide = board.sideToMove;
            else if (result.score < -1500)
                winningSide = ~board.sideToMove;
            break;
        }
        else if (board.halfMoveClock >= 80 && absScore > 600)
        {
            if (result.score > 600)
                winningSide = board.sideToMove;
            else if (result.score < -600)
                winningSide = ~board.sideToMove;
            break;
        }

        if (drawCount > 5)
        {
            winningSide = NO_COLOR;
            break;
        }

        fens.push_back(fn);

        board.makeMove<true>(result.move);
        search.tds[0].board = board;
    }
    std::ofstream file;
    std::string filename = "data/data" + std::to_string(threadId) + ".txt";
    file.open(filename, std::ios::app);
    for (auto &f : fens)
    {
        if (f.use)
            file << stringFenData(f, winningSide, randomPlayBoard.sideToMove) << std::endl;
        randomPlayBoard.makeMove<false>(f.move);
    }
    file << std::endl;
    file.close();
}

std::string stringFenData(fenData fenData, Color ws, Color stm)
{
    double score;
    if (ws == NO_COLOR)
        score = 0.5;
    else if (ws == stm)
        score = 1.0;
    else if (~ws == stm)
        score = 0.0;

    std::ostringstream sstream;
    sstream << std::fixed << std::setprecision(1) << score;
    return std::string{fenData.fen + " [" + sstream.str() + "] " + std::to_string(fenData.score)};
}

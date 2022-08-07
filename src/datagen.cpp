#include "datagen.h"

// random number generator
std::random_device rd;
std::mt19937 e{rd()}; // or std::default_random_engine e{rd()};

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
    std::ofstream file;
    std::string filename = "data/data" + std::to_string(threadId) + ".txt";
    file.open(filename, std::ios::app);
    U64 games = 0;
    while (!UCI_FORCE_STOP)
    {
        games++;
        randomPlayout(file, threadId);
    }
    file.close();
}

void Datagen::randomPlayout(std::ofstream &file, int threadId)
{
    Board board;
    board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::vector<fenData> fens;
    Movelist movelist;
    int ply = 0;

    while (ply++ <= 7)
    {
        movelist = board.legalmoves();
        if (movelist.size == 0 || UCI_FORCE_STOP)
            break;

        // int index = rand() % movelist.size;
        std::uniform_int_distribution<int> rm{0, movelist.size - 1};
        int index = rm(e);

        Move move = movelist.list[index];
        board.makeMove<true>(move);
    }

    // std::ifstream fenFile;
    // fenFile.open("data/fens.epd");

    // int line = 0;
    // int maxLines = 100'000;
    // std::uniform_int_distribution<int> rm{0, maxLines};
    // int lineNum = rm(e);

    // std::string readLine;
    // std::string fen;
    // while (std::getline(fenFile, readLine))
    // {
    //     line++;
    //     if (line == lineNum)
    //     {
    //         fen = readLine;
    //         break;
    //     }
    // }
    // board.applyFen(fen);
    // fenFile.close();

    Board randomPlayBoard = board;

    Search search = Search();

    ThreadData td;
    td.board = board;
    td.id = 0;
    td.allowPrint = false;
    search.tds.push_back(td);

    constexpr int nodes = 0;
    constexpr int depth = 8;

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
        SearchResult result = search.iterative_deepening(depth, nodes, t, 0);

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
        fn.score = result.score;
        fn.move = result.move;
        fn.use = true;
        fn.use = !(capture || inCheck || ply < 15);

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

        if (winCount >= 4 || absScore >= VALUE_MATE_IN_PLY)
        {
            if (result.score > 1500)
                winningSide = board.sideToMove;
            else if (result.score < -1500)
                winningSide = ~board.sideToMove;
            break;
        }
        else if (board.halfMoveClock >= 80 && absScore > 400)
        {
            if (result.score > 400)
                winningSide = board.sideToMove;
            else if (result.score < -400)
                winningSide = ~board.sideToMove;
            break;
        }

        if (drawCount >= 8)
        {
            winningSide = NO_COLOR;
            break;
        }

        if (fn.use)
            fn.fen = board.getFen();
        else
            fn.fen = "";

        fens.push_back(fn);

        board.makeMove<true>(result.move);
        search.tds[0].board = board;
    }
    // std::ofstream file;
    // std::string filename = "data/data" + std::to_string(threadId) + ".txt";
    // file.open(filename, std::ios::app);
    for (auto &f : fens)
    {
        if (f.use)
            file << stringFenData(f, winningSide, randomPlayBoard.sideToMove) << std::endl;
        randomPlayBoard.makeMove<false>(f.move);
    }
    // file.close();
}

std::string stringFenData(fenData fenData, Color ws, Color stm)
{
    double score = 0.5;
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

#include "uci.h"

UCI::UCI()
{
    searcher = Search();
    board = Board();
    options = uciOptions();
    genData = Datagen::TrainingData();
    useTB = false;

    threadCount = 1;

    // load default position
    options.uciPosition(board);

    // Initialize reductions used in search
    init_reductions();
}

int UCI::uciLoop(int argc, char **argv)
{
    std::vector<std::string> allArgs(argv + 1, argv + argc);

    if (elementInVector("bench", allArgs))
    {
        Bench::startBench();
        uciCommand::quit(searcher, genData);
        return 0;
    }
    else if (elementInVector("perft", allArgs))
    {
        int n = 1;
        if (elementInVector("-n", allArgs))
            n = findElement<int>("-n", allArgs);

        Perft perft = Perft();
        perft.board = board;
        perft.testAllPos(n);
        uciCommand::quit(searcher, genData);
        return 0;
    }
    else if (argc > 1)
    {
        parseArgs(argc, argv, options);
    }

    // START OF TUNE

    // TUNE_INT(razorMargin, -100, 100);

    // END OF TUNE

    while (true)
    {
        // catching inputs
        std::string input;

        // UCI COMMANDS
        if (!std::getline(std::cin, input) && argc == 1)
            input = "quit";

        std::vector<std::string> tokens = splitInput(input);

        if (input == "quit")
        {
            uciCommand::quit(searcher, genData);
            return 0;
        }
        else if (input == "uci")
            uciCommand::uciInput(options);
        else if (input == "isready")
            uciCommand::isreadyInput();
        else if (input == "ucinewgame")
            uciCommand::ucinewgameInput(options, board, searcher, genData);
        else if (input == "stop")
            uciCommand::stopThreads(searcher, genData);
        else if (contains("setoption name", input))
        {
            std::string option = tokens[2];
            std::string value = tokens[4];
            if (option == "Hash")
                options.uciHash(std::stoi(value));
            else if (option == "EvalFile")
                options.uciEvalFile(value);
            else if (option == "Threads")
                threadCount = options.uciThreads(std::stoi(value));
            else if (option == "SyzygyPath")
                useTB = options.uciSyzygy(input);
            else if (option == "UCI_Chess960")
                options.uciChess960(board, value);
            // ADD TUNES BY HAND AND DO `extern int x;` IN uci.h
            // else if (option == "")  = std::stoi(value);
            // double
            // else if (option == "") = std::stod(value);
        }
        else if (contains("position", input))
        {
            bool hasMoves = elementInVector("moves", tokens);
            if (tokens[1] == "fen")
                options.uciPosition(board, input.substr(input.find("fen") + 4), false);
            else
                options.uciPosition(board, DEFAULT_POS, false);

            if (hasMoves)
                options.uciMoves(board, tokens);

            // setup accumulator with the correct board
            board.accumulate();
        }
        else if (contains("go perft", input))
        {
            int depth = findElement<int>("perft", tokens);
            Perft perft = Perft();
            perft.board = board;
            perft.perfTest(depth, depth);
        }
        else if (contains("go", input))
        {
            uciCommand::stopThreads(searcher, genData);

            Limits info;

            std::string limit;
            if (tokens.size() == 1)
                limit = "";
            else
                limit = tokens[1];

            info.depth = (limit == "depth") ? findElement<int>("depth", tokens) : MAX_PLY;
            info.depth = (limit == "infinite" || input == "go") ? MAX_PLY : info.depth;
            info.nodes = (limit == "nodes") ? findElement<int>("nodes", tokens) : 0;
            info.time.maximum = info.time.optimum = (limit == "movetime") ? findElement<int>("movetime", tokens) : 0;

            std::string side = board.sideToMove == White ? "wtime" : "btime";
            if (elementInVector(side, tokens))
            {
                // go wtime 100 btime 100 winc 100 binc 100
                std::string inc_str = board.sideToMove == White ? "winc" : "binc";
                int64_t timegiven = findElement<int>(side, tokens);
                int64_t inc = 0;
                int64_t mtg = 0;

                // Increment
                if (elementInVector(inc_str, tokens))
                    inc = findElement<int>(inc_str, tokens);

                // Moves to next time control
                if (elementInVector("movestogo", tokens))
                    inc = findElement<int>("movestogo", tokens);

                // Calculate search time
                info.time = optimumTime(timegiven, inc, mtg);
            }

            // start search
            searcher.startThinking(board, threadCount, info.depth, info.nodes, info.time, useTB);
        }
        // ENGINE SPECIFIC
        uciCommand::parseInput(input, board);
    }
}

// ./smallbrain bench
void UCI::parseArgs(int argc, char **argv, uciOptions options)
{
    std::vector<std::string> allArgs(argv + 1, argv + argc);

    if (elementInVector("-gen", allArgs))
    {
        int workers = 1;
        int depth = 7;
        std::string bookPath = "";
        std::string tbPath = "";
        bool useTB = false;

        if (elementInVector("-threads", allArgs))
        {
            workers = findElement<int>("-threads", allArgs);
        }

        if (elementInVector("-book", allArgs))
        {
            bookPath = findElement<std::string>("-book", allArgs);
        }

        if (elementInVector("-tb", allArgs))
        {
            std::string s = "setoption name SyzygyPath value " + findElement<std::string>("-tb", allArgs);
            useTB = options.uciSyzygy(s);
        }

        if (elementInVector("-depth", allArgs))
        {
            depth = findElement<int>("-depth", allArgs);
        }

        genData.generate(workers, bookPath, depth, useTB);

        std::cout << "Starting data generation" << std::endl;
    }
}
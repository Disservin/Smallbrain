#include "uci.h"

UCI::UCI()
{
    searcher = Search();
    board = Board();
    options = uciOptions();
    genData = Datagen::TrainingData();

    threadCount = 1;

    // load default position
    options.uciPosition(board);

    // Initialize reductions used in search
    init_reductions();
}

int UCI::uciLoop(int argc, char **argv)
{
    std::vector<std::string> allArgs(argv + 1, argv + argc);

    if (uciCommand::elementInVector("bench", allArgs))
    {
        Bench::startBench();
        uciCommand::quit(searcher, genData);
        return 0;
    }
    else if (uciCommand::elementInVector("perft", allArgs))
    {
        uciCommand::parseInput(allArgs[0], searcher, board, genData);
        uciCommand::quit(searcher, genData);
        return 0;
    }
    else if (argc > 1)
    {
        parseArgs(argc, argv, options, board);
    }

    // START OF TUNE

    // TUNE_INT(razorMargin, -100, 100);

    // END OF TUNE

    while (true)
    {
        // catching inputs
        std::string input;

        // UCI COMMANDS
        if (argc == 1 && !std::getline(std::cin, input))
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
        else if (uciCommand::stringContain("setoption name", input))
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
                options.uciSyzygy(value);

            // ADD TUNES BY HAND AND DO `extern int x;` IN uci.h
            // else if (option == "")  = std::stoi(value);
            // double
            // else if (option == "") = std::stod(value);
        }
        else if (uciCommand::stringContain("position", input))
        {
            bool hasMoves = uciCommand::elementInVector("moves", tokens);
            if (tokens[1] == "fen")
                options.uciPosition(board, input.substr(input.find("fen") + 4), false);
            else
                options.uciPosition(board, DEFAULT_POS, false);

            if (hasMoves)
                options.uciMoves(board, tokens);

            // setup accumulator with the correct board
            board.accumulate();
        }
        else if (uciCommand::stringContain("go perft", input))
        {
            int depth = uciCommand::findElement("perft", tokens);
            Perft perft = Perft();
            perft.board = board;
            perft.perfTest(depth, depth);
        }
        else if (uciCommand::stringContain("go", input))
        {
            uciCommand::stopThreads(searcher, genData);

            Limits info;

            std::string limit;
            if (tokens.size() == 1)
                limit = "";
            else
                limit = tokens[1];

            info.depth = (limit == "depth") ? uciCommand::findElement("depth", tokens) : MAX_PLY;
            info.depth = (limit == "infinite" || input == "go") ? MAX_PLY : info.depth;
            info.nodes = (limit == "nodes") ? uciCommand::findElement("nodes", tokens) : 0;
            info.time.maximum = info.time.optimum =
                (limit == "movetime") ? uciCommand::findElement("movetime", tokens) : 0;

            std::string side = board.sideToMove == White ? "wtime" : "btime";
            if (uciCommand::elementInVector(side, tokens))
            {
                // go wtime 100 btime 100 winc 100 binc 100
                std::string inc_str = board.sideToMove == White ? "winc" : "binc";
                int64_t timegiven = uciCommand::findElement(side, tokens);
                int64_t inc = 0;
                int64_t mtg = 0;

                // Increment
                if (uciCommand::elementInVector(inc_str, tokens))
                    inc = uciCommand::findElement(inc_str, tokens);

                // Moves to next time control
                if (uciCommand::elementInVector("movestogo", tokens))
                    inc = uciCommand::findElement("movestogo", tokens);

                // Calculate search time
                info.time = optimumTime(timegiven, inc, board.fullMoveNumber, mtg);
            }

            // start search
            searcher.startThinking(board, threadCount, info.depth, info.nodes, info.time);
        }
        // ENGINE SPECIFIC
        uciCommand::parseInput(input, searcher, board, genData);
    }
}

// ./smallbrain bench
void UCI::parseArgs(int argc, char **argv, uciOptions options, Board board)
{
    std::vector<std::string> allArgs(argv + 1, argv + argc);

    if (uciCommand::elementInVector("-gen", allArgs))
    {
        int workers = 1;
        int depth = 7;
        std::string bookPath = "";
        std::string tbPath = "";

        if (uciCommand::elementInVector("-threads", allArgs))
        {
            workers = uciCommand::findElement("-threads", allArgs);
        }

        if (uciCommand::elementInVector("-book", allArgs))
        {
            bookPath = uciCommand::findElementString("-book", allArgs);
        }

        if (uciCommand::elementInVector("-tb", allArgs))
        {
            options.uciSyzygy(uciCommand::findElementString("-tb", allArgs));
        }

        if (uciCommand::elementInVector("-depth", allArgs))
        {
            depth = uciCommand::findElement("-depth", allArgs);
        }

        genData.generate(workers, bookPath, depth);
    }
}
#include "uci.h"

// global because of signal
Search searcher = Search();
Datagen dg = Datagen();

int uciLoop(int argc, char **argv)
{
    Board board = Board();
    uciOptions options = uciOptions();
    int threads = 1;

    // load default position
    options.uciPosition(board);

    // making sure threads and tds are really clear
    searcher.threads.clear();
    searcher.tds.clear();

    std::vector<std::string> allArgs(argv + 1, argv + argc);

    if (uciCommand::elementInVector("bench", allArgs))
    {
        startBench();
        uciCommand::quit(searcher, dg);
        return 0;
    }
    else if (uciCommand::elementInVector("perft", allArgs))
    {
        uciCommand::parseInput(allArgs[0], searcher, board, dg);
        uciCommand::quit(searcher, dg);
        return 0;
    }
    else
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
        std::getline(std::cin, input);
        std::vector<std::string> tokens = splitInput(input);

        // UCI COMMANDS
        if (input == "uci")
            uciCommand::uciInput(options);

        if (input == "isready")
            uciCommand::isreadyInput();

        if (input == "ucinewgame")
            uciCommand::ucinewgameInput(options, board, searcher, dg);

        if (input == "quit")
            uciCommand::quit(searcher, dg);

        if (input == "stop")
            uciCommand::stopThreads(searcher, dg);

        if (input.find("setoption name") != std::string::npos)
        {
            std::string option = tokens[2];
            std::string value = tokens[4];
            if (option == "Hash")
                options.uciHash(std::stoi(value));
            else if (option == "EvalFile")
                options.uciEvalFile(value);
            else if (option == "Threads")
                threads = options.uciThreads(std::stoi(value));
            else if (option == "SyzygyPath")
                options.uciSyzygy(value);

            // ADD TUNES BY HAND AND DO `extern int x;` IN uci.h
            // else if (option == "")  = std::stoi(value);
            // double
            // else if (option == "") = std::stod(value);
        }

        if (input.find("position") != std::string::npos)
        {
            bool hasMoves = uciCommand::elementInVector("moves", tokens);
            if (tokens[1] == "fen")
                options.uciPosition(board, input.substr(input.find("fen") + 4), false);
            else
                options.uciPosition(board, DEFAULT_POS, false);

            if (hasMoves)
                options.uciMoves(board, tokens);

            board.prevStates.list.clear();
            board.accumulate();
        }

        if (input.find("go") != std::string::npos)
        {
            uciCommand::stopThreads(searcher, dg);

            stopped = false;
            Limits info;
            info.depth = MAX_PLY;
            info.nodes = 0;

            std::string limit;
            if (tokens.size() == 1)
                limit = "";
            else
                limit = tokens[1];

            if (limit == "perft")
            {
                int depth = uciCommand::findElement("perft", tokens);
                Perft perft = Perft();
                perft.board = board;
                perft.perfTest(depth, depth);
                uciCommand::quit(searcher, dg);
                return 0;
            }

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
            searcher.startThinking(board, threads, info.depth, info.nodes, info.time);
        }
        // ENGINE SPECIFIC
        uciCommand::parseInput(input, searcher, board, dg);
    }
}

void signalCallbackHandler(int signum)
{
    uciCommand::stopThreads(searcher, dg);
    free(TTable);
    exit(signum);
}

// ./smallbrain bench
void parseArgs(int argc, char **argv, uciOptions options, Board board)
{
    if (argc > 1)
    {
        std::vector<std::string> allArgs(argv + 1, argv + argc);

        if (uciCommand::elementInVector("-gen", allArgs))
        {
            int workers = 1;
            int depth = 7;
            bool additionalEndgame = false;
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

            if (uciCommand::elementInVector("-endgame", allArgs))
            {
                additionalEndgame = uciCommand::findElement("-endgame", allArgs);
            }

            dg.generate(workers, bookPath, depth, additionalEndgame);
        }
    }
}

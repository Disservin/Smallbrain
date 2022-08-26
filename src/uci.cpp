#include "uci.h"
#include "benchmark.h"
#include "board.h"
#include "datagen.h"
#include "evaluation.h"
#include "nnue.h"
#include "scores.h"
#include "search.h"
#include "timemanager.h"
#include "tt.h"
#include "ucicommands.h"
#include "ucioptions.h"

std::atomic<bool> stopped;
std::atomic<bool> UCI_FORCE_STOP;

U64 TT_SIZE = 16 * 1024 * 1024 / sizeof(TEntry); // initialise to 16 MB
TEntry *TTable;                                  // TEntry size is 14 bytes

Board board = Board();
Search searcher = Search();
uciOptions options = uciOptions();
Datagen dg = Datagen();

int threads = 1;

int main(int argc, char **argv)
{
    UCI_FORCE_STOP = false;
    stopped = false;

    signal(SIGINT, signalCallbackHandler);
#ifdef SIGBREAK
    signal(SIGBREAK, signalCallbackHandler);
#endif

    // Initialize TT
    allocateTT();

    // Initialize NNUE
    // This either loads the weights from a file or makes use of the weights in the binary file that it was compiled
    // with.
    NNUE::init("default.nnue");

    // Initialize reductions used in search
    init_reductions();

    // load default position
    options.uciPosition(board);

    // making sure threads and tds are really clear
    searcher.threads.clear();
    searcher.tds.clear();

    // START OF TUNE

    // TUNE_INT(razorMargin, -100, 100);

    // END OF TUNE
    while (true)
    {
        // ./smallbrain bench
        if (argc > 1)
        {
            if (argv[1] == std::string("bench"))
            {
                startBench();
                uciCommand::quit(searcher, dg);
                return 0;
            }
            else if (argv[1] == std::string("gen"))
            {
                int workers = argc > 2 ? atoi(argv[2]) : 1;
                dg.generate(workers);
            }
            else if (argv[1] == std::string("perft"))
            {
                uciCommand::parseInput(argv[1], searcher, board, dg);
            }
        }

        // catching inputs
        std::string input;
        std::getline(std::cin, input);
        std::vector<std::string> tokens = splitInput(input);
        // UCI COMMANDS
        if (input == "uci")
        {
            std::cout << "id name Smallbrain dev " << uciCommand::currentDateTime() << std::endl;
            std::cout << "id author Disservin\n" << std::endl;
            options.printOptions();
            std::cout << "uciok" << std::endl;
        }
        if (input == "isready")
            std::cout << "readyok" << std::endl;

        if (input == "ucinewgame")
        {
            options.uciPosition(board);
            uciCommand::stopThreads(searcher, dg);
            searcher.tds.clear();
        }
        if (input == "quit")
        {
            uciCommand::quit(searcher, dg);
            return 0;
        }

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

            // ADD TUNES BY HAND AND DO `extern int x;` IN uci.h
            // else if (option == "")  = std::stoi(value);
            // double
            // else if (option == "") = std::stod(value);
        }
        if (input.find("position") != std::string::npos)
        {
            bool hasMoves = input.find("moves") != std::string::npos;
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
                int depth = std::stoi(tokens[2]);
                Perft perft = Perft();
                perft.board = board;
                perft.perfTest(depth, depth);
                uciCommand::quit(searcher, dg);
                return 0;
            }

            info.depth = (limit == "depth") ? std::stoi(tokens[2]) : MAX_PLY;
            info.depth = (limit == "infinite" || input == "go") ? MAX_PLY : info.depth;
            info.nodes = (limit == "nodes") ? std::stoi(tokens[2]) : 0;
            info.time.maximum = info.time.optimum =
                (limit == "movetime") ? uciCommand::findElement("movetime", tokens) : 0;

            std::string side = board.sideToMove == White ? "wtime" : "btime";
            if (input.find(side) != std::string::npos)
            {
                // go wtime 100 btime 100 winc 100 binc 100
                std::string inc_str = board.sideToMove == White ? "winc" : "binc";
                int64_t timegiven = uciCommand::findElement(side, tokens);
                int64_t inc = 0;
                int64_t mtg = 0;
                // Increment
                if (input.find(inc_str) != std::string::npos)
                {
                    auto index = find(tokens.begin(), tokens.end(), inc_str) - tokens.begin();
                    inc = std::stoi(tokens[index + 1]);
                }
                // Moves to next time control
                if (input.find("movestogo") != std::string::npos)
                {
                    auto index = find(tokens.begin(), tokens.end(), "movestogo") - tokens.begin();
                    mtg = std::stoi(tokens[index + 1]);
                }
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
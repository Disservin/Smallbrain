#include "uci.h"
#include "benchmark.h"
#include "board.h"
#include "evaluation.h"
#include "neuralnet.h"
#include "scores.h"
#include "search.h"
#include "timemanager.h"
#include "tt.h"
#include "ucioptions.h"

std::atomic<bool> stopped;

U64 TT_SIZE = 16 * 1024 * 1024 / sizeof(TEntry); // initialise to 16 MB
TEntry *TTable;                                  // TEntry size is 14 bytes

Board board = Board();
Search searcher = Search();
NNUE nnue = NNUE();
uciOptions options = uciOptions();

int threads = 1;

int main(int argc, char **argv)
{
    stopped = false;
    signal(SIGINT, signal_callback_handler);

    // Initialize TT
    allocate_tt();

    // Initialize NNUE
    // This either loads the weights from a file or makes use of the weights in the binary file that it was compiled
    // with.
    nnue.init("default.net");

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
                start_bench();
                quit();
            }
        }

        // catching inputs
        std::string input;
        std::getline(std::cin, input);
        std::vector<std::string> tokens = split_input(input);
        // UCI COMMANDS
        if (input == "uci")
        {
            std::cout << "id name Smallbrain Version 5.0" << std::endl;
            std::cout << "id author Disservin\n" << std::endl;
            options.printOptions();
            std::cout << "uciok" << std::endl;
        }
        if (input == "isready")
            std::cout << "readyok" << std::endl;

        if (input == "ucinewgame")
        {
            options.uciPosition(board);
            stop_threads();
            searcher.tds.clear();
        }
        if (input == "quit")
            quit();

        if (input == "stop")
            stop_threads();

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
            board.accumulate();
        }
        if (input.find("go") != std::string::npos)
        {
            stop_threads();

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
                perft.perf_Test(depth, depth);
                quit();
            }

            info.depth = (limit == "depth") ? std::stoi(tokens[2]) : MAX_PLY;
            info.depth = (limit == "infinite" || input == "go") ? MAX_PLY : info.depth;
            info.nodes = (limit == "nodes") ? std::stoi(tokens[2]) : 0;
            info.time.maximum = info.time.optimum = (limit == "movetime") ? find_element("movetime", tokens) : 0;

            std::string side = board.sideToMove == White ? "wtime" : "btime";
            if (input.find(side) != std::string::npos)
            {
                // go wtime 100 btime 100 winc 100 binc 100
                std::string inc_str = board.sideToMove == White ? "winc" : "binc";
                int64_t timegiven = find_element(side, tokens);
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
            searcher.start_thinking(board, threads, info.depth, info.nodes, info.time);
        }
        // ENGINE SPECIFIC
        if (input == "print")
            board.print();

        if (input == "captures" || input == "moves")
        {
            Movelist ml = input == "captures" ? board.capturemoves() : board.legalmoves();
            for (int i = 0; i < ml.size; i++)
                std::cout << printMove(ml.list[i]) << std::endl;
            std::cout << "count: " << signed(ml.size) << std::endl;
        }

        if (input == "rep")
            std::cout << board.isRepetition(3) << std::endl;

        if (input == "eval")
            std::cout << evaluation(board) << std::endl;

        if (input.find("perft") != std::string::npos)
        {
            Perft perft = Perft();
            perft.board = board;
            perft.testAllPos();
            return 0;
        }
    }
}

void signal_callback_handler(int signum)
{
    stopped = true;
    stop_threads();
    free(TTable);
    exit(signum);
}

void stop_threads()
{
    stopped = true;
    for (std::thread &th : searcher.threads)
    {
        if (th.joinable())
            th.join();
    }
    searcher.threads.clear();
}

void quit()
{
    stop_threads();
    free(TTable);
    exit(0);
}

int find_element(std::string param, std::vector<std::string> tokens)
{
    int index = find(tokens.begin(), tokens.end(), param) - tokens.begin();
    int value = std::stoi(tokens[index + 1]);
    return value;
}
#include "board.h"
#include "search.h"
#include "threadmanager.h"
#include "scores.h"
#include "uci.h"
#include "tt.h"
#include "evaluation.h"
#include "benchmark.h"
#include "timemanager.h"
#include "neuralnet.h"

#include <signal.h>
#include <math.h> 
#include <fstream>

std::atomic<bool> stopped;
ThreadManager thread;

U64 TT_SIZE = 524287;
TEntry* TTable{};   //TEntry size is 18 bytes

Board board = Board();
Search searcher_class = Search(board);
NNUE nnue = NNUE();

int main(int argc, char** argv) {
    stopped = false;
    signal(SIGINT, signal_callback_handler);
    TEntry* oldbuffer;

    // Initialize TT
    if ((TTable = (TEntry*)malloc(TT_SIZE * sizeof(TEntry))) == NULL) {
        std::cout << "Error: Could not allocate memory for TT" << std::endl;
        exit(1);
    }

    // Initialize NNUE
    // This either loads the weights from a file or makes use of the weights in the binary file that it was compiled with.
    nnue.init("default.net");

    init_reductions();

    // load position
    searcher_class.board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    while (true) {
        // ./smallbrain bench
        if (argc > 1) {
            if (argv[1] == std::string("bench")) {
                start_bench();
                return 0;
            }
        }
        Time t;
        std::string input;
        std::getline(std::cin, input);
        std::vector<std::string> tokens = split_input(input);
        // UCI COMMANDS
        if (input == "uci") {
            std::cout << "id name Smallbrain Version 4.0\n" <<
                         "id author Disservin\n" <<
                         "\noption name Hash type spin default 400 min 1 max 100000\n" << //Hash in mb
                         "option name Threads type spin default 1 min 1 max 1\n" << //Threads
                         "option name EvalFile type string default default.net\n" << //NN file
                         "uciok" << std::endl;
        }
        if (input == "isready") {
            std::cout << "readyok" << std::endl;
        }
        if (input == "ucinewgame") {
            searcher_class.board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        }
        if (input == "quit") {
            thread.stop();
            free(TTable);
            return 0;
        }
        if (input == "stop") {
            thread.stop();
        }
        if (input.find("setoption name") != std::string::npos)
        {
            std::string option = tokens[2];
            std::string value = tokens[4];
            if (option == "Hash")
            {
                U64 elements = (static_cast<unsigned long long>(std::stoi(value)) * 1000000) / sizeof(TEntry);
                oldbuffer = TTable;
                if ((TTable = (TEntry*)realloc(TTable, elements * sizeof(TEntry))) == NULL)
                {
                    std::cout << "Error: Could not allocate memory for TT" << std::endl;
                    free(oldbuffer);
                    exit(1);
                }
                TT_SIZE = elements;  
            }
            else if (option == "EvalFile")
            {
                std::cout << "Loading eval file: " << value << std::endl;            
                nnue.init(value.c_str());    
            }
        }
        if (input.find("position") != std::string::npos) {
            if (tokens[1] == "fen") 
            {
                std::size_t start_index = input.find("fen");
                std::string fen = input.substr(start_index + 4);
                searcher_class.board.applyFen(fen);
            }
            else
            {
                searcher_class.board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            }
            if (input.find("moves") != std::string::npos) {
                std::size_t index = std::find(tokens.begin(), tokens.end(), "moves") - tokens.begin();
                index++;
                for (; index < tokens.size(); index++) {
                    Move move = convert_uci_to_Move(tokens[index]);
                    searcher_class.board.makeMove(move);
                }
            }
        }
        if (input.find("go") != std::string::npos) 
        {
            thread.stop();

            std::string limit = tokens[1];
            int depth = MAX_PLY;
            int nodes = 0;
            Time time = t;
            if (limit == "depth") {
                depth = std::stoi(tokens[2]);
            }
            else if (limit == "perft") {
                depth = std::stoi(tokens[2]);
                std::thread threads = std::thread(&Search::perf_Test, searcher_class, depth, depth);
                threads.join();
                return 0;    
            }
            else if (limit == "nodes") {
                nodes = std::stoi(tokens[2]);
            }
            else if (limit == "infinite" || input == "go") {
                depth = MAX_PLY;
            }
            else if (limit == "movetime") {
                auto indexTime = find(tokens.begin(), tokens.end(), "movetime") - tokens.begin();
                int64_t timegiven = std::stoi(tokens[indexTime + 1]);        
                time.maximum = timegiven;
                time.optimum = timegiven;
            }
            else if (input.find("wtime") != std::string::npos) {
                // go wtime 100 btime 100 winc 100 binc 100
                std::string side = searcher_class.board.sideToMove == White ? "wtime" : "btime";
                auto indexTime = find(tokens.begin(), tokens.end(), side) - tokens.begin();
                int64_t timegiven = std::stoi(tokens[indexTime + 1]);
                int64_t inc = 0;
                int64_t mtg = 0;
                // Increment 
                if (input.find("winc") != std::string::npos && tokens.size() > 4) {
                    std::string inc_str = searcher_class.board.sideToMove == White ? "winc" : "binc";
                    auto it = find(tokens.begin(), tokens.end(), inc_str);
                    int index = it - tokens.begin();
                    inc = std::stoi(tokens[index + 1]);
                }
                // Moves to next time control
                if (input.find("movestogo") != std::string::npos) {
                    auto it = find(tokens.begin(), tokens.end(), "movestogo");
                    int index = it - tokens.begin();
                    mtg = std::stoi(tokens[index + 1]);
                }
                // Calculate search time
                time = optimumTime(timegiven, inc, searcher_class.board.fullMoveNumber, mtg);
            }
            else {
                std::cout << "Error: Invalid limit" << std::endl; // Silent Error
                return 0;
            }
            thread.begin(depth, nodes, time);
        }
        // ENGINE SPECIFIC
        if (input == "print") {
            searcher_class.board.print();
            std::cout << searcher_class.board.getFen() << std::endl;
        }

        if (input == "moves") {
            Movelist ml = searcher_class.board.legalmoves();
            for (int i = 0; i < ml.size; i++) {
                std::cout << searcher_class.board.printMove(ml.list[i]) << std::endl;
            }
        }

        if (input == "captures") {
            Movelist ml = searcher_class.board.capturemoves();
            for (int i = 0; i < ml.size; i++) {
                std::cout << searcher_class.board.printMove(ml.list[i]) << std::endl;
            }
        }

        if (input == "rep") {
            std::cout << searcher_class.board.isRepetition(3) << std::endl;
        }

        if (input == "eval") {
            std::cout << evaluation(searcher_class.board) << std::endl;
        }

        if (input.find("perft") != std::string::npos) {
            std::thread threads = std::thread(&Search::testAllPos, searcher_class);
            threads.join();
            return 0;
        }
    }
}

void signal_callback_handler(int signum) {
    thread.stop();
    free(TTable);
    exit(signum);
}

Move convert_uci_to_Move(std::string input) {
    Move move;
    if (input.length() == 4) {
        std::string from = input.substr(0, 2);
        std::string to = input.substr(2);
        char letter = from[0];
        int file = letter - 96;
        int rank = from[1] - 48;
        int from_index = (rank - 1) * 8 + file - 1;
        Square source = Square(from_index);

        letter = to[0];
        file = letter - 96;
        rank = to[1] - 48;

        int to_index = (rank - 1) * 8 + file - 1;
        Square target = Square(to_index);
        PieceType piece = searcher_class.board.piece_type(searcher_class.board.pieceAtBB(source));
        return Move(piece, source, target, false);
    }
    if (input.length() == 5) {
        std::string from = input.substr(0, 2);
        std::string to = input.substr(2, 2);
        char letter = from[0];
        int file = letter - 96;
        int rank = from[1] - 48;
        int from_index = (rank - 1) * 8 + file - 1;
        Square source = Square(from_index);

        letter = to[0];
        file = letter - 96;
        rank = to[1] - 48;
        
        int to_index = (rank - 1) * 8 + file - 1;
        Square target = Square(to_index);
        std::map<char, int> piece_to_int =
        {
        { 'n', 1 },
        { 'b', 2 },
        { 'r', 3 },
        { 'q', 4 },
        { 'N', 1 },
        { 'B', 2 },
        { 'R', 3 },
        { 'Q', 4 }
        };
        char prom = input.at(4);
        PieceType piece = PieceType(piece_to_int[prom]);
        return Move(piece, source, target, true);
    }
    else {
        std::cout << "FALSE INPUT" << std::endl;
        return Move(NONETYPE, NO_SQ, NO_SQ, false);
    }
}
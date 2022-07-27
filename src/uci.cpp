#include "board.h"
#include "search.h"
#include "scores.h"
#include "uci.h"
#include "tt.h"
#include "evaluation.h"
#include "benchmark.h"
#include "timemanager.h"
#include "neuralnet.h"

std::atomic<bool> stopped;

U64 TT_SIZE = 16 * 1024 * 1024 / sizeof(TEntry); // initialise to 16 MB
TEntry* TTable;   //TEntry size is 14 bytes

Board board = Board();
Search searcher = Search();
NNUE nnue = NNUE();

int threads = 1;

int main(int argc, char** argv) {
    stopped = false;
    signal(SIGINT, signal_callback_handler);
    
    // Initialize TT
    allocate_tt();

    // Initialize NNUE
    // This either loads the weights from a file or makes use of the weights in the binary file that it was compiled with.
    nnue.init("default.net");

    // Initialize reductions used in search
    init_reductions();

    // load default position
    board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // making sure threads and tds are really clear
    searcher.threads.clear();
    searcher.tds.clear();
    while (true) {
        // ./smallbrain bench
        if (argc > 1) {
            if (argv[1] == std::string("bench")) {
                start_bench();
                quit();
            }
        }

        // catching inputs
        std::string input;
        std::getline(std::cin, input);
        std::vector<std::string> tokens = split_input(input);
        // UCI COMMANDS
        if (input == "uci") {
            std::cout << "id name Smallbrain Version 5.0\n" <<
                         "id author Disservin\n" <<
                         "\noption name Hash type spin default 400 min 1 max 57344\n" << // Hash in MB
                         "option name Threads type spin default 1 min 1 max 256\n" << // Threads
                         "option name EvalFile type string default default.net\n" << // NN file
                         "uciok" << std::endl;
        }
        if (input == "isready") std::cout << "readyok" << std::endl;

        if (input == "ucinewgame") {
            board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            stop_threads();
            searcher.tds.clear();
        }
        if (input == "quit") quit();

        if (input == "stop") stop_threads();

        if (input.find("setoption name") != std::string::npos)
        {
            std::string option = tokens[2];
            std::string value = tokens[4];
            if (option == "Hash")
            {
                int sizeMB = std::clamp(std::stoi(value), 2, MAXHASH);
                U64 elements = (static_cast<unsigned long long>(sizeMB) * 1024 * 1024) / sizeof(TEntry);
                reallocate_tt(elements);
            }
            else if (option == "EvalFile")
            {
                std::cout << "Loading eval file: " << value << std::endl;            
                nnue.init(value.c_str());    
            }
            else if (option == "Threads") threads = std::stoi(value); 
        }
        if (input.find("position") != std::string::npos) {
            if (tokens[1] == "fen") 
            {
                std::size_t start_index = input.find("fen");
                std::string fen = input.substr(start_index + 4);
                board.applyFen(fen);
            }
            else
            {
                board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            }
            if (input.find("moves") != std::string::npos) {
                std::size_t index = std::find(tokens.begin(), tokens.end(), "moves") - tokens.begin();
                index++;
                for (; index < tokens.size(); index++) {
                    Move move = convert_uci_to_Move(tokens[index]);
                    board.makeMove(move);
                }
            }
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

            if (limit == "perft") {
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
            if (input.find(side) != std::string::npos) {
                // go wtime 100 btime 100 winc 100 binc 100 
                std::string inc_str = board.sideToMove == White ? "winc" : "binc";
                int64_t timegiven = find_element(side, tokens);
                int64_t inc = 0;
                int64_t mtg = 0;
                // Increment 
                if (input.find(inc_str) != std::string::npos) {
                    auto index = find(tokens.begin(), tokens.end(), inc_str) - tokens.begin();
                    inc = std::stoi(tokens[index + 1]);
                }
                // Moves to next time control
                if (input.find("movestogo") != std::string::npos) {
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
        if (input == "print") board.print();

        if (input == "captures" || input == "moves") {
            Movelist ml = input == "captures" ? board.capturemoves() : board.legalmoves();
            for (int i = 0; i < ml.size; i++)
                std::cout << board.printMove(ml.list[i]) << std::endl;
            std::cout << "count: " << signed(ml.size) << std::endl;
        }

        if (input == "rep") std::cout << board.isRepetition(3) << std::endl;

        if (input == "eval") std::cout << evaluation(board) << std::endl;

        if (input.find("perft") != std::string::npos) {
            Perft perft = Perft();
            perft.board = board;
            perft.testAllPos();
            return 0;
        }
    }
}

void signal_callback_handler(int signum) {
    stopped = true;
    stop_threads();
    free(TTable);
    exit(signum);
}

Move convert_uci_to_Move(std::string input) {
    std::string from = input.substr(0, 2);
    char letter = from[0];
    int file = letter - 96;
    int rank = from[1] - 48;
    int from_index = (rank - 1) * 8 + file - 1;
    Square source = Square(from_index);

    if (input.length() == 4) {
        std::string to = input.substr(2);
        letter = to[0];
        file = letter - 96;
        rank = to[1] - 48;

        int to_index = (rank - 1) * 8 + file - 1;
        Square target = Square(to_index);
        PieceType piece = type_of_piece(board.pieceAtBB(source));
        return make(piece, source, target, false);
    }
    else if (input.length() == 5) {
        std::string to = input.substr(2, 2);
        letter = to[0];
        file = letter - 96;
        rank = to[1] - 48;
        
        int to_index = (rank - 1) * 8 + file - 1;
        Square target = Square(to_index);
 
        char prom = input.at(4);
        PieceType piece = piece_to_int[prom];
        return make(piece, source, target, true);
    }
    else {
        std::cout << "FALSE INPUT" << std::endl;
        return make(NONETYPE, NO_SQ, NO_SQ, false);
    }
}

void stop_threads()
{
    stopped = true;
    for (std::thread& th: searcher.threads) {
        if (th.joinable())
            th.join();
    }
    searcher.threads.clear();    
}

void allocate_tt()
{
    if ((TTable = (TEntry*)malloc(TT_SIZE * sizeof(TEntry))) == NULL) 
    {
        std::cout << "Error: Could not allocate memory for TT" << std::endl;
        exit(1);
    }
    std::memset(TTable, 0, TT_SIZE * sizeof(TEntry));
}

void reallocate_tt(U64 elements)
{
    TEntry* oldbuffer = TTable;
    if ((TTable = (TEntry*)realloc(TTable, elements * sizeof(TEntry))) == NULL)
    {
        std::cout << "Error: Could not allocate memory for TT" << std::endl;
        free(oldbuffer);
        exit(1);
    }

    TT_SIZE = elements;  
    std::memset(TTable, 0, TT_SIZE * sizeof(TEntry));    
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
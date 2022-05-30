#include "board.h"
#include "search.h"
#include "threadmanager.h"
#include "psqt.h"
#include "uci.h"
#include "tt.h"
#include "evaluation.h"
#include "benchmark.h"
#include "timemanager.h"

#include <signal.h>
#include <math.h> 

std::atomic<bool> stopped;
ThreadManager thread;
U64 TT_SIZE = 131071;
TEntry* TTable{};
Board board = Board();
Search searcher_class = Search(board);

int main(int argc, char** argv) {
    searcher_class.board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    stopped = false;
    signal(SIGINT, signal_callback_handler);
    TEntry* oldbuffer;
    if ((TTable = (TEntry*)malloc(TT_SIZE * sizeof(TEntry))) == NULL) {
        std::cout << "Error: Could not allocate memory for TT\n";
        exit(1);
    }
    for (int i = 0; i < 256; i++){
        reductions[i] = log(i) + 1;
    }
    while (true) {
        if (argc > 1) {
            if (argv[1] == std::string("bench")) {
                start_bench();
                return 0;
            }
        }
        std::string input;
        std::getline(std::cin, input);
        if (input == "uci") {
            std::cout << "id name Smallbrain Version 1.2\n" <<
                         "id author Disservin\n" <<
                         "\noption name Hash type spin default 400 min 1 max 100000\n" << //Hash in mb
                         "option name Threads type spin default 1 min 1 max 1\n" << //Threads
                         "uciok\n" << std::endl;
        }
        if (input == "isready") {
            std::cout << "readyok\n" << std::endl;
        }
        if (input == "ucinewgame") {
            searcher_class.board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        }
        if (input == "quit") {
            thread.stop();
            return 0;
        }
        if (input == "stop") {
            thread.stop();
        }
        if (input == "print") {
            searcher_class.board.print();
            std::cout << searcher_class.board.getFen() << std::endl;
        }
        if (input.find("setoption name Hash value") != std::string::npos) {
            std::size_t start_index = input.find("value");
            std::string size_str = input.substr(start_index + 6);
            U64 elements = (static_cast<unsigned long long>(std::stoi(size_str)) * 1000000) / sizeof(TEntry);
            oldbuffer = TTable;
            if ((TTable = (TEntry*)realloc(TTable, elements * sizeof(TEntry))) == NULL)
            {
                std::cout << "Error: Could not allocate memory for TT\n";
                free(oldbuffer);
                exit(1);
            }
            TT_SIZE = elements;
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
        if (input.find("position") != std::string::npos) {
            std::vector<std::string> tokens = split_input(input);
            searcher_class.board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

            if (tokens[1] == "fen") {
                std::size_t start_index = input.find("fen");
                std::string fen = input.substr(start_index + 4);
                searcher_class.board.applyFen(fen);
            }
            if (input.find("moves") != std::string::npos) {
                std::size_t index = std::find(tokens.begin(), tokens.end(), "moves") - tokens.begin();
                index++;

                for (; index < tokens.size(); index++) {
                    Move move = convert_uci_to_Move(tokens[index]);
                    std::cout << searcher_class.board.printMove(move) << std::endl;
                    searcher_class.board.makeMove(move);
                }
            }
        }
        if (input.find("go perft") != std::string::npos) {
            std::vector<std::string> tokens = split_input(input);
            int depth = std::stoi(tokens[2]);
            std::thread threads = std::thread(&Search::perf_Test, searcher_class, depth, depth);
            threads.join();
            return 0;
        }
        if (input.find("perft") != std::string::npos) {
            std::thread threads = std::thread(&Search::testAllPos, searcher_class);
            threads.join();
            return 0;
        }
        if (input.find("go depth") != std::string::npos) {
            thread.stop();
            std::vector<std::string> tokens = split_input(input);
            int depth = std::stoi(tokens[2]);
            Time t;
            t.optimum = 0;
            t.maximum = 0;
            thread.begin(depth, t);
        }
        if (input.find("go infinite") != std::string::npos) {
            thread.stop();
            std::vector<std::string> tokens = split_input(input);
            int depth = 120;
            Time t;
            t.optimum = 0;
            t.maximum = 0;
            thread.begin(depth, t);
        }
        if (input.find("wtime") != std::string::npos) {
            thread.stop();
            std::vector<std::string> tokens = split_input(input);
            int depth = 120;
            // go wtime 100 btime 100 winc 100 binc 100
            std::string side = searcher_class.board.sideToMove == White ? "wtime" : "btime";
            auto indexTime = find(tokens.begin(), tokens.end(), side) - tokens.begin();
            int64_t timegiven = std::stoi(tokens[indexTime + 1]);
            int64_t inc = 0;
            int64_t mtg = 0;
            if (input.find("winc") != std::string::npos && tokens.size() > 4) {
                std::string inc_str = searcher_class.board.sideToMove == White ? "winc" : "binc";
                auto it = find(tokens.begin(), tokens.end(), inc_str);
                int index = it - tokens.begin();
                inc = std::stoi(tokens[index + 1]);
            }
            if (input.find("movestogo") != std::string::npos) {
                auto it = find(tokens.begin(), tokens.end(), "movestogo");
                int index = it - tokens.begin();
                mtg = std::stoi(tokens[index + 1]);
            }

            Time t = optimumTime(timegiven, inc, searcher_class.board.fullMoveNumber, mtg);
            thread.begin(depth, t);
        }
        if (input.find("setoption") != std::string::npos) {
            std::vector<std::string> tokens = split_input(input);
            if (tokens[2] == "PAWN_EVAL_MG") {
                piece_values[0][PAWN] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "PAWN_EVAL_EG") {
                piece_values[1][PAWN] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "KNIGHT_EVAL_MG") {
                piece_values[0][KNIGHT] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "KNIGHT_EVAL_EG") {
                piece_values[1][KNIGHT] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "BISHOP_EVAL_MG") {
                piece_values[0][BISHOP] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "BISHOP_EVAL_EG") {
                piece_values[1][BISHOP] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "ROOK_EVAL_MG") {
                piece_values[0][ROOK] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "ROOK_EVAL_EG") {
                piece_values[1][ROOK] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "QUEEN_EVAL_MG") {
                piece_values[0][QUEEN] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "QUEEN_EVAL_EG") {
                piece_values[1][QUEEN] = std::stoi(tokens[4]);
            }
            if (tokens[2] == "killer1") {
                killerscore1 = std::stoi(tokens[4]);
            }
            if (tokens[2] == "killer2") {
                killerscore2 = std::stoi(tokens[4]);
            }
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
        int from_index;
        int to_index;
        char letter;
        letter = from[0];
        int file = letter - 96;
        int rank = from[1] - 48;
        from_index = (rank - 1) * 8 + file - 1;
        Square source = Square(from_index);
        letter = to[0];
        file = letter - 96;
        rank = to[1] - 48;
        to_index = (rank - 1) * 8 + file - 1;
        Square target = Square(to_index);
        PieceType piece = searcher_class.board.piece_type(searcher_class.board.pieceAtBB(source));
        return Move(piece, source, target, false);
    }
    if (input.length() == 5) {
        std::string from = input.substr(0, 2);
        std::string to = input.substr(2, 2);
        int from_index;
        int to_index;
        char letter;
        letter = from[0];
        int file = letter - 96;
        int rank = from[1] - 48;
        from_index = (rank - 1) * 8 + file - 1;

        Square source = Square(from_index);
        letter = to[0];
        file = letter - 96;
        rank = to[1] - 48;
        to_index = (rank - 1) * 8 + file - 1;
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
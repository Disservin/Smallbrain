#include "board.h"
#include "search.h"
#include "threadmanager.h"
#include "psqt.h"
#include "uci.h"
#include "tt.h"
#include "evaluation.h"

#include <signal.h>


std::atomic<bool> stopped;
ThreadManager thread;
U64 TT_SIZE = 131071;
TEntry* TTable{};

void signal_callback_handler(int signum) {
    thread.stop();
    free(TTable);
    exit(signum);
}

Move convert_uci_to_Move(std::string input, Board& board) {
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
        PieceType piece = board.piece_type(board.pieceAtBB(source));
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
        { 'q', 4 }
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

int main(int argc, char** argv) {
    Board board = Board();
    board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    stopped = false;
    signal(SIGINT, signal_callback_handler);
    TEntry* oldbuffer;
    if ((TTable = (TEntry*)malloc(TT_SIZE * sizeof(TEntry))) == NULL) {
        std::cout << "Error: Could not allocate memory for TT\n";
        exit(1);
    }

    while (true) {
        if (argc > 1) {
            if (argv[1] == std::string("bench")) {
                Search searcher_class = Search(board);
                std::thread threads = std::thread(&Search::iterative_deepening, searcher_class, 5, true, 0);
                threads.join();
                return 0;
            }
        }
        std::string input;
        std::getline(std::cin, input);
        if (input == "uci") {
            std::cout << "id name Chess\n" <<
                "id author Disservin\n" <<
                "uciok\n" << std::endl;

        }
        if (input == "isready") {
            std::cout << "readyok\n" << std::endl;
        }
        if (input == "ucinewgame") {
            board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        }
        if (input == "quit") {
            thread.stop();
            return 0;
        }
        if (input == "stop") {
            thread.stop();
        }
        if (input == "print") {
            board.print();
            std::cout << board.getFen() << std::endl;
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
            Movelist ml = board.legalmoves();
            for (int i = 0; i < ml.size; i++) {
                std::cout << board.printMove(ml.list[i]) << std::endl;
            }
        }
        if (input == "rep") {
            std::cout << board.isRepetition(3) << std::endl;
        }
        if (input == "eval") {
            std::cout << evaluation(board) << std::endl;
        }
        if (input.find("position") != std::string::npos) {
            std::vector<std::string> tokens = split_input(input);
            board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

            if (tokens[1] == "fen") {
                std::size_t start_index = input.find("fen");
                std::string fen = input.substr(start_index + 4);
                board.applyFen(fen);
            }
            if (input.find("moves") != std::string::npos) {
                std::size_t index = std::find(tokens.begin(), tokens.end(), "moves") - tokens.begin();
                index++;

                for (; index < tokens.size(); index++) {
                    Move move = convert_uci_to_Move(tokens[index], board);
                    board.makeMove(move);

                }
            }
        }
        if (input.find("go perft") != std::string::npos) {
            std::vector<std::string> tokens = split_input(input);
            int depth = std::stoi(tokens[2]);
            Search searcher_class = Search(board);
            std::thread threads = std::thread(&Search::perf_Test, searcher_class, depth, depth);
            threads.join();
            return 0;
        }
        if (input.find("perft") != std::string::npos) {
            Search searcher_class = Search(board);
            std::thread threads = std::thread(&Search::testAllPos, searcher_class);
            threads.join();
            return 0;
        }
        if (input.find("go depth") != std::string::npos) {
            thread.stop();
            std::vector<std::string> tokens = split_input(input);
            int depth = std::stoi(tokens[2]);
            thread.begin(board, depth);
        }
        if (input.find("go infinite") != std::string::npos) {
            thread.stop();
            std::vector<std::string> tokens = split_input(input);
            int depth = 120;
            thread.begin(board, depth);
        }
        if (input.find("go wtime") != std::string::npos) {
            thread.stop();
            std::vector<std::string> tokens = split_input(input);
            int depth = 120;
            // go wtime 100 btime 100 winc 100 binc 100
            int64_t timegiven = board.sideToMove == White ? std::stoi(tokens[2]) : std::stoi(tokens[4]);
            int64_t inc = 0;
            int64_t mtg = 0;
            if (input.find("winc") != std::string::npos && tokens.size() > 4) {
                std::string inc_str = board.sideToMove == White ? "winc" : "binc";
                auto it = find(tokens.begin(), tokens.end(), inc_str);
                int index = it - tokens.begin();
                inc = std::stoi(tokens[index + 1]);
            }
            if (input.find("movestogo") != std::string::npos) {
                auto it = find(tokens.begin(), tokens.end(), "movestogo");
                int index = it - tokens.begin();
                mtg = std::stoi(tokens[index + 1]);
            }

            int64_t minimumTime = 1; 
            int64_t searchTime = timegiven / 20;
            if (mtg > 0) {
                searchTime = timegiven / mtg;
            }
            else {
                searchTime = timegiven / 20;
            }
            searchTime += inc / 2;
            if (searchTime >= timegiven) {
                searchTime = std::clamp(searchTime, minimumTime, timegiven / 20);
            }
            thread.begin(board, depth, false, searchTime);
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
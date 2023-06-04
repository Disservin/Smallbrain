#include "uci.h"
#include "benchmark.h"
#include "evaluation.h"
#include "helper.h"
#include "nnue.h"
#include "perft.h"
#include "syzygy/Fathom/src/tbprobe.h"
#include "tests/tests.h"
#include "thread.h"
#include "tt.h"
#include "cli.h"

extern std::atomic_bool UCI_FORCE_STOP;
extern TranspositionTable TTable;
extern ThreadPool Threads;

uciOptions options_ = uciOptions();

UCI::UCI() {
    use_tb_ = false;

    thread_count_ = 1;

    // load default position
    board_.applyFen(DEFAULT_POS);

    // Initialize reductions used in search
}

int UCI::uciLoop() {
    // catching inputs
    std::string input;
    std::cin >> std::ws;

    while (true) {
        if (!std::getline(std::cin, input) && std::cin.eof()) {
            input == "quit";
        }

        if (input == "quit") {
            quit();
            return 0;
        } else {
            processCommand(input);
        }
    }

    return 0;
}

void UCI::processCommand(std::string command) {
    std::vector<std::string> tokens = splitInput(command);

    if (tokens[0] == "stop") {
        Threads.stop_threads();
    } else if (tokens[0] == "ucinewgame") {
        ucinewgameInput();
    } else if (tokens[0] == "uci") {
        uciInput();
    } else if (tokens[0] == "isready") {
        isreadyInput();
    } else if (tokens[0] == "setoption") {
        std::string option = tokens[2];
        std::string value = tokens[4];

        if (option == "Hash")
            options_.uciHash(std::stoi(value));
        else if (option == "EvalFile")
            options_.uciEvalFile(value);
        else if (option == "Threads")
            thread_count_ = options_.uciThreads(std::stoi(value));
        else if (option == "SyzygyPath")
            use_tb_ = options_.uciSyzygy(command);
        else if (option == "UCI_Chess960")
            options_.uciChess960(board_, value);
    } else if (tokens[0] == "position") {
        setPosition(tokens, command);
    } else if (contains(command, "go perft")) {
        int depth = findElement<int>("perft", tokens);
        PerftTesting perft = PerftTesting();
        perft.board = board_;
        perft.perfTest(depth, depth);
    } else if (tokens[0] == "go") {
        startSearch(tokens, command);
    } else if (command == "print") {
        std::cout << board_ << std::endl;
    } else if (command == "fen") {
        std::cout << getRandomfen() << std::endl;
    } else if (command == "captures") {
        Movelist moves;
        movegen::legalmoves<Movetype::CAPTURE>(board_, moves);

        for (auto ext : moves) std::cout << uciMove(ext.move, board_.chess960) << std::endl;

        std::cout << "count: " << signed(moves.size) << std::endl;
    } else if (command == "moves") {
        Movelist moves;
        movegen::legalmoves<Movetype::ALL>(board_, moves);

        for (auto ext : moves) std::cout << uciMove(ext.move, board_.chess960) << std::endl;

        std::cout << "count: " << signed(moves.size) << std::endl;
    }

    else if (command == "rep") {
        std::cout << board_.isRepetition(2) << std::endl;
    }

    else if (command == "eval") {
        std::cout << eval::evaluation(board_) << std::endl;
    }

    else if (command == "perft") {
        PerftTesting perft = PerftTesting();
        perft.board = board_;
        perft.testAllPos();
    } else if (contains(command, "move")) {
        if (contains(tokens, "move")) {
            std::size_t index = std::find(tokens.begin(), tokens.end(), "move") - tokens.begin();
            index++;

            for (; index < tokens.size(); index++) {
                Move move = convertUciToMove(board_, tokens[index]);
                board_.makeMove<true>(move);
            }
        }
    } else {
        std::cout << "Unknown command: " << command << std::endl;
    }
}

void UCI::uciInput() {
    std::cout << "id name " << OptionsParser::getVersion() << std::endl;
    std::cout << "id author Disservin\n" << std::endl;
    options_.printOptions();
    std::cout << "uciok" << std::endl;
}

void UCI::isreadyInput() { std::cout << "readyok" << std::endl; }

void UCI::ucinewgameInput() {
    board_.applyFen(DEFAULT_POS);
    Threads.stop_threads();
    TTable.clear();
}

void UCI::quit() {
    Threads.stop_threads();

    Threads.pool.clear();

    tb_free();
}

void UCI::uciMoves(const std::vector<std::string> &tokens) {
    std::size_t index = std::find(tokens.begin(), tokens.end(), "moves") - tokens.begin();
    index++;
    for (; index < tokens.size(); index++) {
        Move move = convertUciToMove(board_, tokens[index]);
        board_.makeMove<false>(move);
    }
}

void UCI::startSearch(const std::vector<std::string> &tokens, const std::string &command) {
    Limits info;
    std::string limit;

    Threads.stop_threads();

    if (tokens.size() == 1)
        limit = "";
    else
        limit = tokens[1];

    info.depth = (limit == "depth") ? findElement<int>("depth", tokens) : MAX_PLY - 1;
    info.depth = command == "go" ? MAX_PLY - 1 : info.depth;
    info.infinite = limit == "infinite";
    info.nodes = (limit == "nodes") ? findElement<int>("nodes", tokens) : 0;
    info.time.maximum = info.time.optimum =
        (limit == "movetime") ? findElement<int>("movetime", tokens) : 0;

    searchmoves_.size = 0;

    const std::string keyword = "searchmoves_";
    if (contains(tokens, keyword)) {
        std::size_t index = std::find(tokens.begin(), tokens.end(), keyword) - tokens.begin() + 1;
        for (; index < tokens.size(); index++) {
            Move move = convertUciToMove(board_, tokens[index]);
            searchmoves_.Add(move);
        }
    }

    std::string side_str = board_.side_to_move == White ? "wtime" : "btime";
    std::string inc_str = board_.side_to_move == White ? "winc" : "binc";

    if (contains(tokens, side_str)) {
        int64_t timegiven = findElement<int>(side_str, tokens);
        int64_t inc = 0;
        int64_t mtg = 0;

        // Increment
        if (contains(tokens, inc_str)) inc = findElement<int>(inc_str, tokens);

        // Moves to next time control
        if (contains(tokens, "movestogo")) mtg = findElement<int>("movestogo", tokens);

        // Calculate search time
        info.time = optimumTime(timegiven, inc, mtg);
    }

    // start search
    Threads.start_threads(board_, info, searchmoves_, thread_count_, use_tb_);
}

void UCI::setPosition(const std::vector<std::string> &tokens, const std::string &command) {
    Threads.stop_threads();

    bool hasMoves = contains(tokens, "moves");

    if (tokens[1] == "fen")
        board_.applyFen(command.substr(command.find("fen") + 4), false);
    else
        board_.applyFen(DEFAULT_POS, false);

    if (hasMoves) uciMoves(tokens);

    // setup accumulator with the correct board_
    board_.refresh();
}

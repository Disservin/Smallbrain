#include "uci.h"

#include "syzygy/Fathom/src/tbprobe.h"

#include "cli.h"
#include "str_utils.h"
#include "thread.h"
#include "tt.h"
#include "evaluation.h"
#include "perft.h"

extern TranspositionTable TTable;
extern ThreadPool Threads;

namespace uci {
Uci::Uci() {
    options_ = uci::Options();
    board_ = Board();

    options_.add(uci::Option{"Hash", "spin", "16", "16", "1", "60129"});
    options_.add(uci::Option{"Threads", "spin", "1", "1", "1", "256"});
    options_.add(uci::Option{"EvalFile", "string", "", "", "", ""});
    options_.add(uci::Option{"SyzygyPath", "string", "", "", "", ""});
    options_.add(uci::Option{"UCI_Chess960", "check", "false", "false", "", ""});
}

void Uci::uciLoop() {
    board_.setFen(DEFAULT_POS);

    std::string input;
    std::cin >> std::ws;

    while (true) {
        if (!std::getline(std::cin, input) && std::cin.eof()) {
            input = "quit";
        }

        if (input == "quit") {
            quit();
            return;
        } else {
            processLine(input);
        }
    }

    return;
}

void Uci::processLine(const std::string& line) {
    std::vector<std::string> tokens = str_util::splitString(line, ' ');

    if (tokens.empty()) {
        return;
    } else if (tokens[0] == "position") {
        position(line);
    } else if (tokens[0] == "uci") {
        uci();
    } else if (tokens[0] == "isready") {
        isReady();
    } else if (tokens[0] == "ucinewgame") {
        uciNewGame();
    } else if (str_util::contains(line, "go perft")) {
        int depth = str_util::findElement<int>(tokens, "perft").value_or(1);
        PerftTesting perft = PerftTesting();
        perft.board = board_;
        perft.perfTest(depth, depth);
    } else if (tokens[0] == "go") {
        go(line);
    } else if (tokens[0] == "stop") {
        stop();
    } else if (tokens[0] == "setoption") {
        setOption(line);
    } else if (tokens[0] == "eval") {
        std::cout << eval::evaluation(board_) << std::endl;
    } else if (tokens[0] == "print") {
        std::cout << board_ << std::endl;
    } else {
        std::cout << "Unknown command: " << line << std::endl;
    }
}

void Uci::uci() {
    std::cout << "id name " << ArgumentsParser::getVersion() << std::endl;
    std::cout << "id author Disservin\n" << std::endl;
    options_.print();
    std::cout << "uciok" << std::endl;
    applyOptions();
}

void Uci::setOption(const std::string& line) {
    options_.set(line);
    applyOptions();
}

void Uci::applyOptions() {
    const auto path = options_.get<std::string>("SyzygyPath");

    if (!path.empty()) {
        if (tb_init(path.c_str())) {
            use_tb_ = true;
            std::cout << "info string successfully loaded syzygy path " << path << std::endl;
        } else {
            std::cout << "info string failed to load syzygy path " << path << std::endl;
        }
    }

    const auto eval_file = options_.get<std::string>("EvalFile");

    if (!eval_file.empty()) {
        std::cout << "info string EvalFile " << eval_file << std::endl;
        nnue::init(eval_file.c_str());
    }

    worker_threads_ = options_.get<int>("Threads");
    board_.chess960 = options_.get<bool>("UCI_Chess960");

    TTable.allocateMB(options_.get<int>("Hash"));
}

void Uci::isReady() { std::cout << "readyok" << std::endl; }

void Uci::uciNewGame() {
    board_ = Board();
    TTable.clear();
    Threads.stopThreads();
}

void Uci::position(const std::string& line) {
    const auto fen_range = str_util::findRange(line, "fen", "moves");

    const auto fen = str_util::contains(line, "fen") ? line.substr(line.find("fen") + 4, fen_range)
                                                     : DEFAULT_POS;

    const auto moves = str_util::contains(line, "moves") ? line.substr(line.find("moves") + 6) : "";
    const auto moves_vec = str_util::splitString(moves, ' ');

    board_ = Board(fen);

    for (const auto& move : moves_vec) {
        board_.makeMove<false>(uciToMove(board_, move));
    }

    board_.refresh();
}

void Uci::go(const std::string& line) {
    Threads.stopThreads();

    Limits limit;

    const auto tokens = str_util::splitString(line, ' ');

    if (tokens.size() == 1) limit.infinite = true;

    limit.depth = str_util::findElement<int>(tokens, "depth").value_or(MAX_PLY);
    limit.infinite = str_util::findElement<std::string>(tokens, "go").value_or("") == "infinite";
    limit.nodes = str_util::findElement<int64_t>(tokens, "nodes").value_or(0);
    limit.time.maximum = limit.time.optimum =
        str_util::findElement<int64_t>(tokens, "movetime").value_or(0);

    std::string uci_time = board_.side_to_move == Color::WHITE ? "wtime" : "btime";
    std::string uci_inc = board_.side_to_move == Color::WHITE ? "winc" : "binc";

    if (str_util::contains(line, uci_time)) {
        auto time = str_util::findElement<int>(tokens, uci_time).value_or(0);
        auto inc = str_util::findElement<int>(tokens, uci_inc).value_or(0);
        auto mtg = str_util::findElement<int>(tokens, "movestogo").value_or(0);

        limit.time = optimumTime(time, inc, mtg);
    }

    if (str_util::contains(line, "searchmoves")) {
        const auto searchmoves =
            str_util::findElement<std::string>(tokens, "searchmoves").value_or("");
        const auto moves = str_util::splitString(searchmoves, ' ');

        for (const auto& move : moves) {
            searchmoves_.add(uciToMove(board_, move));
        }
    }

    Threads.startThreads(board_, limit, searchmoves_, worker_threads_, use_tb_);
}

void Uci::stop() { Threads.stopThreads(); }

void Uci::quit() {
    Threads.stopThreads();
    tb_free();
}

Square extractSquare(std::string_view squareStr) {
    char letter = squareStr[0];
    int file = letter - 96;
    int rank = squareStr[1] - 48;
    int index = (rank - 1) * 8 + file - 1;
    return Square(index);
}

Move uciToMove(const Board& board, const std::string& input) {
    Square source = extractSquare(input.substr(0, 2));
    Square target = extractSquare(input.substr(2, 2));
    PieceType piece = board.at<PieceType>(source);

    // convert to king captures rook
    if (!board.chess960 && piece == KING && squareDistance(target, source) == 2) {
        target = file_rank_square(target > source ? FILE_H : FILE_A, square_rank(source));
    }

    if (piece == KING && board.at<PieceType>(target) == ROOK &&
        board.at(target) / 6 == board.at(source) / 6) {
        return make<Move::CASTLING>(source, target);
    }

    if (piece == PAWN && target == board.en_passant_square) {
        return make<Move::ENPASSANT>(source, target);
    }

    switch (input.length()) {
        case 4:
            return make(source, target);
        case 5:
            return make<Move::PROMOTION>(source, target, pieceToInt[input.at(4)]);
        default:
            std::cout << "FALSE INPUT" << std::endl;
            return make(NO_SQ, NO_SQ);
    }
}

std::string moveToUci(Move move, bool chess960) {
    std::stringstream ss;

    // Get the from and to squares
    Square from_sq = from(move);
    Square to_sq = to(move);

    // If the move is not a chess960 castling move and is a king moving more than one square,
    // update the to square to be the correct square for a regular castling move
    if (!chess960 && typeOf(move) == CASTLING) {
        to_sq = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C, square_rank(from_sq));
    }

    // Add the from and to squares to the string stream
    ss << squareToString[from_sq];
    ss << squareToString[to_sq];

    // If the move is a promotion, add the promoted piece to the string stream
    if (typeOf(move) == PROMOTION) {
        ss << PieceTypeToPromPiece[promotionType(move)];
    }

    return ss.str();
}

std::string convertScore(int score) {
    if (std::abs(score) <= 4) score = 0;

    if (score >= VALUE_MATE_IN_PLY)
        return "mate " + std::to_string(((VALUE_MATE - score) / 2) + ((VALUE_MATE - score) & 1));
    else if (score <= VALUE_MATED_IN_PLY)
        return "mate " + std::to_string(-((VALUE_MATE + score) / 2) + ((VALUE_MATE + score) & 1));
    else
        return "cp " + std::to_string(score);
}

void output(int score, int depth, uint8_t seldepth, U64 nodes, U64 tbHits, int time,
            const std::string& pv, int hashfull) {
    std::stringstream ss;

    // clang-format off
    ss  << "info depth " << signed(depth) 
        << " seldepth "  << signed(seldepth) 
        << " score "     << convertScore(score)
        << " tbhits "    << tbHits 
        << " nodes "     << nodes 
        << " nps "       << signed((nodes / (time + 1)) * 1000)
        << " hashfull "  << hashfull
        << " time "      << time 
        << " pv"         << pv;
    // clang-format on

    std::cout << ss.str() << std::endl;
}

}  // namespace uci
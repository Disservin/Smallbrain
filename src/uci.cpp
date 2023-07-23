#include "uci.h"

#include <cmath>

#include "syzygy/Fathom/src/tbprobe.h"

#include "cli.h"
#include "evaluation.h"
#include "perft.h"
#include "str_utils.h"
#include "thread.h"
#include "tt.h"

extern ThreadPool Threads;

namespace uci {

constexpr U64 UCI_MAX_HASH_MB =
    static_cast<U64>(TranspositionTable::MAXHASH_MiB * (1024 * 1024) / (1e6));

uci::Options options;

Uci::Uci() {
    options = uci::Options();
    board_ = Board();

    options.add(uci::Option{"Hash", "spin", "16", "16", "1",
                            std::to_string(UCI_MAX_HASH_MB)});  // Size in MB
    options.add(uci::Option{"Threads", "spin", "1", "1", "1", "256"});
    options.add(uci::Option{"EvalFile", "string", "", "", "", ""});
    options.add(uci::Option{"SyzygyPath", "string", "", "", "", ""});
    options.add(uci::Option{"UCI_Chess960", "check", "false", "false", "", ""});
    options.add(uci::Option{"UCI_ShowWDL", "check", "false", "false", "", ""});

    applyOptions();
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
        std::cout << convertScore(eval::evaluate(board_)) << std::endl;
    } else if (tokens[0] == "print") {
        std::cout << board_ << std::endl;
    } else {
        std::cout << "Unknown command: " << line << std::endl;
    }
}

void Uci::uci() {
    std::cout << "id name " << ArgumentsParser::getVersion() << std::endl;
    std::cout << "id author Disservin\n" << std::endl;
    options.print();
    std::cout << "uciok" << std::endl;
}

void Uci::setOption(const std::string& line) {
    options.set(line);
    applyOptions();
}

void Uci::applyOptions() {
    const auto path = options.get<std::string>("SyzygyPath");

    if (!path.empty()) {
        if (tb_init(path.c_str())) {
            use_tb_ = true;
            std::cout << "info string successfully loaded syzygy path " << path << std::endl;
        } else {
            std::cout << "info string failed to load syzygy path " << path << std::endl;
        }
    }

    const auto eval_file = options.get<std::string>("EvalFile");

    if (!eval_file.empty()) {
        std::cout << "info string EvalFile " << eval_file << std::endl;
        nnue::init(eval_file.c_str());
    }

    worker_threads_ = options.get<int>("Threads");
    board_.chess960 = options.get<bool>("UCI_Chess960");

    TTable.allocateMB(options.get<int>("Hash"));
}

void Uci::isReady() { std::cout << "readyok" << std::endl; }

void Uci::uciNewGame() {
    board_ = Board();
    TTable.clear();
    Threads.kill();
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

    board_.refreshNNUE(board_.getAccumulator());
}

void Uci::go(const std::string& line) {
    Threads.kill();

    Limits limit;

    const auto tokens = str_util::splitString(line, ' ');

    if (tokens.size() == 1) limit.infinite = true;

    limit.depth = str_util::findElement<int>(tokens, "depth").value_or(MAX_PLY - 1);
    limit.infinite = str_util::findElement<std::string>(tokens, "go").value_or("") == "infinite";
    limit.nodes = str_util::findElement<int64_t>(tokens, "nodes").value_or(0);
    limit.time.maximum = limit.time.optimum =
        str_util::findElement<int64_t>(tokens, "movetime").value_or(0);

    std::string uci_time = board_.sideToMove() == Color::WHITE ? "wtime" : "btime";
    std::string uci_inc = board_.sideToMove() == Color::WHITE ? "winc" : "binc";

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

        searchmoves_.size = 0;

        for (const auto& move : moves) {
            searchmoves_.add(uciToMove(board_, move));
        }
    }

    Threads.start(board_, limit, searchmoves_, worker_threads_, use_tb_);
}

void Uci::stop() { Threads.kill(); }

void Uci::quit() {
    Threads.kill();
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
    auto piece = board.at<PieceType>(source);

    // convert to king captures rook
    if (!board.chess960 && piece == KING && squareDistance(target, source) == 2) {
        target = fileRankSquare(target > source ? FILE_H : FILE_A, squareRank(source));
    }

    if (piece == KING && board.at<PieceType>(target) == ROOK &&
        board.at(target) / 6 == board.at(source) / 6) {
        return make<Move::CASTLING>(source, target);
    }

    if (piece == PAWN && target == board.enPassant()) {
        return make<Move::ENPASSANT>(source, target);
    }

    switch (input.length()) {
        case 4:
            return make(source, target);
        case 5:
            return make<Move::PROMOTION>(source, target, CHAR_TO_PIECETYPE[input.at(4)]);
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
        to_sq = fileRankSquare(to_sq > from_sq ? FILE_G : FILE_C, squareRank(from_sq));
    }

    // Add the from and to squares to the string stream
    ss << SQUARE_TO_STRING[from_sq];
    ss << SQUARE_TO_STRING[to_sq];

    // If the move is a promotion, add the promoted piece to the string stream
    if (typeOf(move) == PROMOTION) {
        ss << PIECETYPE_TO_CHAR[promotionType(move)];
    }

    return ss.str();
}

std::string convertScore(int score) {
    constexpr int NormalizeToPawnValue = 131;

    if (std::abs(score) <= 4) score = 0;

    if (score >= VALUE_MATE_IN_PLY)
        return "mate " + std::to_string(((VALUE_MATE - score) / 2) + ((VALUE_MATE - score) & 1));
    else if (score <= VALUE_MATED_IN_PLY)
        return "mate " + std::to_string(-((VALUE_MATE + score) / 2) + ((VALUE_MATE + score) & 1));
    else
        return "cp " + std::to_string(score * 100 / NormalizeToPawnValue);
}

// https://github.com/official-stockfish/Stockfish/blob/master/src/uci.cpp#L202
int modelWinRate(int v, int ply) {
    double m = std::min(240, ply) / 64.0;

    constexpr double as[] = {0.54196100, -1.26579121, 24.52268566, 107.83857189};
    constexpr double bs[] = {-3.24292846, 22.96074940, -44.91927840, 64.15812572};

    double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
    double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];

    double x = std::clamp(double(v), -4000.0, 4000.0);

    return int(0.5 + 1000 / (1 + std::exp((a - x) / b)));
}

std::string wdl(int v, int ply) {
    int wdl_w = modelWinRate(v, ply);
    int wdl_l = modelWinRate(-v, ply);
    int wdl_d = 1000 - wdl_w - wdl_l;

    std::stringstream ss;
    ss << wdl_w << " " << wdl_d << " " << wdl_l;

    return ss.str();
}

void output(int score, int ply, int depth, uint8_t seldepth, U64 nodes, U64 tbHits, int time,
            const std::string& pv, int hashfull) {
    std::stringstream ss;

    // clang-format off
    ss  << "info depth " << signed(depth) 
        << " seldepth "  << signed(seldepth) 
        << " score "     << convertScore(score);

    if (options.get<bool>("UCI_ShowWDL")) {
        ss << " wdl " << wdl(score, ply);
    }

    ss  << " tbhits "    << tbHits 
        << " nodes "     << nodes 
        << " nps "       << signed((nodes / (time + 1)) * 1000)
        << " hashfull "  << hashfull
        << " time "      << time 
        << " pv"         << pv;
    // clang-format on

    std::cout << ss.str() << std::endl;
}

}  // namespace uci
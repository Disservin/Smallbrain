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
    board_.applyFen(DEFAULT_POS);

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
    std::vector<std::string> tokens = StrUtil::splitString(line, ' ');

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
    } else if (StrUtil::contains(line, "go perft")) {
        int depth = StrUtil::findElement<int>(tokens, "perft").value_or(1);
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
    Threads.stop_threads();
}

void Uci::position(const std::string& line) {
    const auto fen_range = StrUtil::findRange(line, "fen", "moves");

    const auto fen =
        StrUtil::contains(line, "fen") ? line.substr(line.find("fen") + 4, fen_range) : DEFAULT_POS;

    const auto moves = StrUtil::contains(line, "moves") ? line.substr(line.find("moves") + 6) : "";
    const auto moves_vec = StrUtil::splitString(moves, ' ');

    board_ = Board(fen);

    for (const auto& move : moves_vec) {
        board_.makeMove<false>(uciToMove(board_, move));
    }

    board_.refresh(board_.getAccumulator());
}

void Uci::go(const std::string& line) {
    Threads.stop_threads();

    Limits limit;

    const auto tokens = StrUtil::splitString(line, ' ');

    if (tokens.size() == 1) limit.infinite = true;

    limit.depth = StrUtil::findElement<int>(tokens, "depth").value_or(MAX_PLY);
    limit.infinite = StrUtil::findElement<std::string>(tokens, "go").value_or("") == "infinite";
    limit.nodes = StrUtil::findElement<int64_t>(tokens, "nodes").value_or(0);
    limit.time.maximum = limit.time.optimum =
        StrUtil::findElement<int64_t>(tokens, "movetime").value_or(0);

    std::string uci_time = board_.side_to_move == Color::White ? "wtime" : "btime";
    std::string uci_inc = board_.side_to_move == Color::White ? "winc" : "binc";

    if (StrUtil::contains(line, uci_time)) {
        auto time = StrUtil::findElement<int>(tokens, uci_time).value_or(0);
        auto inc = StrUtil::findElement<int>(tokens, uci_inc).value_or(0);
        auto mtg = StrUtil::findElement<int>(tokens, "movestogo").value_or(0);

        limit.time = optimumTime(time, inc, mtg);
    }

    if (StrUtil::contains(line, "searchmoves")) {
        const auto searchmoves =
            StrUtil::findElement<std::string>(tokens, "searchmoves").value_or("");
        const auto moves = StrUtil::splitString(searchmoves, ' ');

        for (const auto& move : moves) {
            searchmoves_.add(uciToMove(board_, move));
        }
    }

    Threads.start_threads(board_, limit, searchmoves_, worker_threads_, use_tb_);
}

void Uci::stop() { Threads.stop_threads(); }

void Uci::quit() {
    Threads.stop_threads();
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
    PieceType piece = type_of_piece(board.pieceAtB(source));

    // convert to king captures rook
    if (!board.chess960 && piece == KING && square_distance(target, source) == 2) {
        target = file_rank_square(target > source ? FILE_H : FILE_A, square_rank(source));
    }

    if (piece == KING && type_of_piece(board.pieceAtB(target)) == ROOK &&
        board.pieceAtB(target) / 6 == board.pieceAtB(source) / 6) {
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

}  // namespace uci
#include <fstream>

#include "datagen.h"
#include "rand.h"
#include "syzygy/Fathom/src/tbprobe.h"

namespace datagen {

std::string stringFenData(const fenData &fen_data, double score) {
    std::ostringstream sstream;
    sstream << fen_data.fen << " [" << std::fixed << std::setprecision(1) << score << "] "
            << fen_data.score;

    return sstream.str();
}

void TrainingData::generate(int workers, std::string book, int depth, int nodes, bool use_tb) {
    if (book != "") {
        std::ifstream openingFile;
        std::string line;
        openingFile.open(book);

        while (std::getline(openingFile, line)) {
            opening_book_.emplace_back(line);
        }

        openingFile.close();
    }

    use_tb_ = use_tb;

    for (int i = 0; i < workers; i++) {
        threads.emplace_back(&TrainingData::infinitePlay, this, i, depth, nodes);
    }
}

void TrainingData::infinitePlay(int threadId, int depth, int nodes) {
    std::ofstream file;
    std::string filename = "data/data" + std::to_string(threadId) + ".txt";
    file.open(filename, std::ios::app);

    Time t;
    t.maximum = 0;
    t.optimum = 0;

    limit_.depth = depth == 0 ? MAX_PLY - 1 : depth;
    limit_.nodes = nodes;
    limit_.time = t;

    U64 games = 0;

    auto t0 = TimePoint::now();
    auto t1 = TimePoint::now();

    while (!stop_) {
        randomPlayout(file);
        games++;

        if (threadId == 0 && games % 100 == 0) {
            t1 = TimePoint::now();
            auto s = std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count();
            std::cout << "\r" << std::left << std::setw(3) << std::setprecision(2)
                      << double(games) / s << "g/s";
        }
    }

    file.close();
}

Board TrainingData::randomStart() {
    constexpr int randomMoves = 10;

    Board board;

    if (opening_book_.size() != 0) {
        std::uniform_int_distribution<> maxLines{0, int(opening_book_.size() - 1)};

        auto randLine = maxLines(rand_gen::generator);

        board.setFen(opening_book_[randLine], false);
    }

    board.setFen(DEFAULT_POS, false);

    Movelist movelist;

    int ply = 0;

    while (ply++ < randomMoves) {
        movelist.size = 0;
        board.clearStacks();

        movegen::legalmoves<Movetype::ALL>(board, movelist);

        if (movelist.size == 0) {
            ply = 0;
            board.setFen(DEFAULT_POS, false);
            continue;
        }

        std::uniform_int_distribution<> randomNum{0, int(movelist.size - 1)};

        auto index = randomNum(rand_gen::generator);

        Move move = movelist[index].move;
        board.makeMove<false>(move);
    }

    board.clearStacks();
    board.refreshNNUE(board.getAccumulator());

    return board;
}

Side TrainingData::makeMove(Search &search, Board &board, std::vector<fenData> &fens,
                            int &win_count, int &draw_count, int ply) {
    search.nodes = 0;
    board.clearStacks();

    const bool in_check =
        board.isAttacked(~board.sideToMove(), board.kingSQ(board.sideToMove()), board.all());

    Movelist movelist;
    movegen::legalmoves<Movetype::ALL>(board, movelist);

    if (movelist.size == 0) {
        return in_check ? Side(~board.sideToMove()) : Side::DRAW;
    }

    if (board.isRepetition(2)) {
        return Side::DRAW;
    }

    auto drawn = board.isDrawn(in_check);

    if (drawn != Result::NONE) {
        return drawn == Result::LOST ? Side(~board.sideToMove()) : Side::DRAW;
    }

    search.board = board;
    const auto result = search.iterativeDeepening();

    // CATCH BUGS
    if (result.score == VALUE_NONE) {
        std::cout << board << std::endl;
        std::exit(1);
    }

    const bool capture = board.at(to(result.bestmove)) != NONE;

    fenData sfens;
    sfens.score = board.sideToMove() == WHITE ? result.score : -result.score;
    sfens.move = result.bestmove;

    const Score absScore = std::abs(result.score);

    if (absScore >= 2000) {
        win_count++;
        draw_count = 0;
    } else if (absScore <= 4) {
        draw_count++;
        win_count = 0;
    } else {
        draw_count = 0;
        win_count = 0;
    }

    if (win_count >= 4 || absScore > VALUE_TB_WIN_IN_MAX_PLY) {
        return result.score > 0 ? Side(board.sideToMove()) : Side(~board.sideToMove());
    } else if (draw_count >= 12) {
        return Side::DRAW;
    }

    if (!(capture || in_check || ply < 8)) {
        sfens.fen = board.getFen();
        fens.emplace_back(sfens);
    }

    if (use_tb_ && board.halfmoves() >= 40 && builtin::popcount(board.all()) <= TB_LARGEST)
        return Side::TB_CHECK;

    board.makeMove<true>(result.bestmove);

    return Side::NONE;
}

void TrainingData::randomPlayout(std::ofstream &file) {
    std::vector<fenData> fens;
    fens.reserve(40);

    int win_count = 0;
    int draw_count = 0;
    int ply = 10;

    Side winningSide = Side::NONE;

    Board board = randomStart();

    std::unique_ptr<Search> search_player_1 = std::make_unique<Search>();

    search_player_1->use_tb = false;
    search_player_1->id = 0;
    search_player_1->limit = limit_;
    search_player_1->silent = true;

    std::unique_ptr<Search> search_player_2 = std::make_unique<Search>();

    search_player_2->use_tb = false;
    search_player_2->id = 0;
    search_player_2->limit = limit_;
    search_player_2->silent = true;

    bool flip = false;

    while (winningSide == Side::NONE) {
        if (flip) {
            winningSide = makeMove(*search_player_1, board, fens, win_count, draw_count, ply);
        } else {
            winningSide = makeMove(*search_player_2, board, fens, win_count, draw_count, ply);
        }

        flip = !flip;
    }

    Bitboard white = board.us<WHITE>();
    Bitboard black = board.us<BLACK>();

    // Set correct winningSide for if (use_tb && board.halfmoves() >= 40 &&
    // builtin::popcount(board.all()) <= TB_LARGEST)
    if (use_tb_ && builtin::popcount(white | black) <= TB_LARGEST) {
        Square ep = board.enPassant() <= 63 ? board.enPassant() : Square(0);

        unsigned TBresult =
            tb_probe_wdl(white, black, board.pieces(WHITEKING) | board.pieces(BLACKKING),
                         board.pieces(WHITEQUEEN) | board.pieces(BLACKQUEEN),
                         board.pieces(WHITEROOK) | board.pieces(BLACKROOK),
                         board.pieces(WHITEBISHOP) | board.pieces(BLACKBISHOP),
                         board.pieces(WHITEKNIGHT) | board.pieces(BLACKKNIGHT),
                         board.pieces(WHITEPAWN) | board.pieces(BLACKPAWN), 0, 0, ep,
                         board.sideToMove() == WHITE);  //  * - turn: true=white, false=black

        if (TBresult == TB_LOSS || TBresult == TB_BLESSED_LOSS) {
            winningSide = Side(~board.sideToMove());
        } else if (TBresult == TB_WIN || TBresult == TB_CURSED_WIN) {
            winningSide = Side(board.sideToMove());
        } else if (TBresult == TB_DRAW) {
            winningSide = Side::DRAW;
        }
    }

    double score;
    if (winningSide == Side::WHITE)
        score = 1.0;
    else if (winningSide == Side::BLACK)
        score = 0.0;
    else
        score = 0.5;

    for (auto &f : fens) {
        file << stringFenData(f, score) << "\n";
    }

    // file.flush();
}

}  // namespace datagen

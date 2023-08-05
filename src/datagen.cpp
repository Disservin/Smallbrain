#include <fstream>

#include "datagen.h"
#include "rand.h"
#include "syzygy/Fathom/src/tbprobe.h"

namespace datagen {

std::string stringFenData(const fenData &fen_data, double score) {
    std::ostringstream sstream;
    sstream << fen_data.fen << " [" << std::fixed << std::setprecision(1) << score << "] "
            << fen_data.score << "\n";

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

    board.setFen(DEFAULT_POS, false);

    if (opening_book_.size() != 0) {
        std::uniform_int_distribution<> maxLines{0, int(opening_book_.size() - 1)};

        auto randLine = maxLines(rand_gen::generator);

        board.setFen(opening_book_[randLine], false);
    }

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
    board.clearHash();
    board.refreshNNUE(board.getAccumulator());

    return board;
}

Side TrainingData::makeMove(Search &search, std::vector<fenData> &fens, int &win_count,
                            int &draw_count, int ply) {
    search.board.clearStacks();
    search.board.clearHash();
    search.board.refreshNNUE(search.board.getAccumulator());

    search.nodes = 0;

    const bool in_check =
        search.board.isAttacked(~search.board.sideToMove(),
                                search.board.kingSQ(search.board.sideToMove()), search.board.all());

    Movelist movelist;
    movegen::legalmoves<Movetype::ALL>(search.board, movelist);

    if (movelist.size == 0) {
        return in_check ? Side(~search.board.sideToMove()) : Side::DRAW;
    }

    auto drawn = search.board.isDrawn(in_check);

    if (drawn != Result::NONE) {
        return drawn == Result::LOST ? Side(~search.board.sideToMove()) : Side::DRAW;
    }

    const auto result = search.iterativeDeepening();

    // CATCH BUGS
    if (result.score == VALUE_NONE) {
        std::cout << search.board << std::endl;
        std::exit(1);
    }

    const bool capture = search.board.at(to(result.bestmove)) != NONE;

    fenData sfens;
    sfens.score = search.board.sideToMove() == WHITE ? result.score : -result.score;
    sfens.move = result.bestmove;

    const Score abs_score = std::abs(result.score);

    if (abs_score >= 2000) {
        win_count++;
        draw_count = 0;
    } else if (abs_score <= 4) {
        draw_count++;
        win_count = 0;
    } else {
        draw_count = 0;
        win_count = 0;
    }

    if (win_count >= 4 || abs_score > VALUE_TB_WIN_IN_MAX_PLY) {
        return result.score > 0 ? Side(search.board.sideToMove())
                                : Side(~search.board.sideToMove());
    } else if (draw_count >= 12) {
        return Side::DRAW;
    }

    if (!(capture || in_check || ply < 8)) {
        sfens.fen = search.board.getFen();
        fens.emplace_back(sfens);
    }

    if (use_tb_ && search.board.halfmoves() >= 40 &&
        builtin::popcount(search.board.all()) <= (signed)TB_LARGEST)
        return Side::TB_CHECK;

    search.board.makeMove<false>(result.bestmove);

    return Side::NONE;
}

void TrainingData::randomPlayout(std::ofstream &file) {
    std::vector<fenData> fens;
    fens.reserve(40);

    int win_count = 0;
    int draw_count = 0;
    int ply = 0;

    Side winningSide = Side::NONE;

    Search search_player = Search();

    search_player.use_tb = false;
    search_player.id = 0;
    search_player.limit = limit_;
    search_player.silent = true;
    search_player.board = randomStart();

    search_player.board.clearHash();

    while (winningSide == Side::NONE) {
        winningSide = makeMove(search_player, fens, win_count, draw_count, ply);
        ply++;
    }

    Bitboard white = search_player.board.us<WHITE>();
    Bitboard black = search_player.board.us<BLACK>();

    // Set correct winningSide for if (use_tb && search_player.board.halfmoves() >= 40 &&
    // builtin::popcount(search_player.board.all()) <= TB_LARGEST)
    if (use_tb_ && builtin::popcount(search_player.board.all()) <= (signed)TB_LARGEST) {
        Square ep =
            search_player.board.enPassant() <= 63 ? search_player.board.enPassant() : Square(0);

        unsigned TBresult = tb_probe_wdl(
            white, black,
            search_player.board.pieces(WHITEKING) | search_player.board.pieces(BLACKKING),
            search_player.board.pieces(WHITEQUEEN) | search_player.board.pieces(BLACKQUEEN),
            search_player.board.pieces(WHITEROOK) | search_player.board.pieces(BLACKROOK),
            search_player.board.pieces(WHITEBISHOP) | search_player.board.pieces(BLACKBISHOP),
            search_player.board.pieces(WHITEKNIGHT) | search_player.board.pieces(BLACKKNIGHT),
            search_player.board.pieces(WHITEPAWN) | search_player.board.pieces(BLACKPAWN), 0, 0, ep,
            search_player.board.sideToMove() == WHITE);  //  * - turn: true=white, false=black

        if (TBresult == TB_LOSS || TBresult == TB_BLESSED_LOSS) {
            winningSide = Side(~search_player.board.sideToMove());
        } else if (TBresult == TB_WIN || TBresult == TB_CURSED_WIN) {
            winningSide = Side(search_player.board.sideToMove());
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
        file << stringFenData(f, score);
    }

    // file.flush();
}

}  // namespace datagen

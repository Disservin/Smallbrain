#include <fstream>

#include "builtin.h"
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

void TrainingData::generate(int workers, const std::string &book, int depth, int nodes,
                            bool use_tb) {
    if (!book.empty()) {
        std::ifstream openingFile;
        std::string line;
        openingFile.open(book);

        while (std::getline(openingFile, line)) {
            opening_book_.emplace_back(line);
        }

        openingFile.close();
    }

    for (int i = 0; i < workers; i++) {
        threads.emplace_back(&TrainingData::infinitePlay, this, i, depth, nodes, use_tb);
    }
}

void TrainingData::infinitePlay(int threadId, int depth, int nodes, bool use_tb) {
    std::ofstream file;
    std::string filename = "data/data" + std::to_string(threadId) + ".txt";
    file.open(filename, std::ios::app);

    std::unique_ptr<Search> search = std::make_unique<Search>();
    Board board = Board();
    Movelist movelist;

    Time t;
    t.maximum = 0;
    t.optimum = 0;

    Limits limit;
    limit.depth = depth == 0 ? MAX_PLY - 1 : depth;
    limit.nodes = nodes;
    limit.time = t;

    U64 games = 0;

    auto t0 = TimePoint::now();
    auto t1 = TimePoint::now();

    while (!stop_) {
        board.clearStacks();
        search->reset();

        search->silent = true;
        search->use_tb = false;
        search->id = 0;
        search->limit = limit;

        movelist.size = 0;

        board.setFen(DEFAULT_POS, false);

        randomPlayout(file, board, movelist, search, use_tb);
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

void TrainingData::randomPlayout(std::ofstream &file, Board &board, Movelist &movelist,
                                 std::unique_ptr<Search> &search, bool use_tb) {
    std::vector<fenData> fens;
    fens.reserve(40);

    int ply = 0;
    int randomMoves = 10;

    if (!opening_book_.empty()) {
        std::uniform_int_distribution<> maxLines{0, int(opening_book_.size() - 1)};

        auto randLine = maxLines(rand_gen::generator);

        board.setFen(opening_book_[randLine], false);
    }

    board.setFen(DEFAULT_POS, true);

    while (ply < randomMoves) {
        movelist.size = 0;
        board.clearStacks();

        movegen::legalmoves<Movetype::ALL>(board, movelist);

        if (movelist.size == 0) return;

        std::uniform_int_distribution<> randomNum{0, int(movelist.size - 1)};

        auto index = randomNum(rand_gen::generator);

        Move move = movelist[index].move;
        board.makeMove<false>(move);
        ply++;
    }

    board.refreshNNUE(board.getAccumulator());
    search->board = board;

    fenData sfens;
    int drawCount = 0;
    int winCount = 0;
    Color winningSide = NO_COLOR;

    while (true) {
        search->board.clearStacks();

        search->nodes = 0;
        movelist.size = 0;

        const bool in_check = search->board.isAttacked(
            ~search->board.sideToMove(), search->board.kingSQ(search->board.sideToMove()),
            search->board.all());

        movegen::legalmoves<Movetype::ALL>(search->board, movelist);

        if (movelist.size == 0) {
            winningSide = in_check ? ~search->board.sideToMove() : NO_COLOR;
            break;
        }

        auto drawn = search->board.isDrawn(in_check);

        if (drawn != Result::NONE) {
            winningSide = drawn == Result::LOST ? ~search->board.sideToMove() : NO_COLOR;
            break;
        }

        const auto result = search->iterativeDeepening();

        // CATCH BUGS
        if (result.score == VALUE_NONE) {
            std::cout << search->board << std::endl;
            exit(1);
        }

        const bool capture = search->board.at(to(result.bestmove)) != NONE;

        sfens.score = search->board.sideToMove() == WHITE ? result.score : -result.score;
        sfens.move = result.bestmove;

        const Score absScore = std::abs(result.score);

        if (absScore >= 2000) {
            winCount++;
            drawCount = 0;
        } else if (absScore <= 4) {
            drawCount++;
            winCount = 0;
        } else {
            drawCount = 0;
            winCount = 0;
        }

        if (winCount >= 4 || absScore > VALUE_TB_WIN_IN_MAX_PLY) {
            winningSide =
                result.score > 0 ? search->board.sideToMove() : ~search->board.sideToMove();
            break;
        } else if (drawCount >= 12) {
            winningSide = NO_COLOR;
            break;
        }

        if (!(capture || in_check || ply < 8)) {
            sfens.fen = search->board.getFen();
            fens.emplace_back(sfens);
        }

        if (use_tb && search->board.halfmoves() >= 40 &&
            builtin::popcount(search->board.all()) <= 6)
            break;

        ply++;
        search->board.makeMove<true>(result.bestmove);
    }

    Bitboard white = search->board.us<WHITE>();
    Bitboard black = search->board.us<BLACK>();

    // Set correct winningSide for if (use_tb && search->board.halfmoves() >= 40 &&
    // builtin::popcount(search->board.all()) <= 6)
    if (use_tb && builtin::popcount(white | black) <= 6) {
        Square ep = search->board.enPassant() <= 63 ? search->board.enPassant() : Square(0);

        unsigned TBresult = tb_probe_wdl(
            white, black, search->board.pieces(WHITEKING) | search->board.pieces(BLACKKING),
            search->board.pieces(WHITEQUEEN) | search->board.pieces(BLACKQUEEN),
            search->board.pieces(WHITEROOK) | search->board.pieces(BLACKROOK),
            search->board.pieces(WHITEBISHOP) | search->board.pieces(BLACKBISHOP),
            search->board.pieces(WHITEKNIGHT) | search->board.pieces(BLACKKNIGHT),
            search->board.pieces(WHITEPAWN) | search->board.pieces(BLACKPAWN), 0, 0, ep,
            search->board.sideToMove() == WHITE);  //  * - turn: true=white, false=black

        if (TBresult == TB_LOSS || TBresult == TB_BLESSED_LOSS) {
            winningSide = ~search->board.sideToMove();
        } else if (TBresult == TB_WIN || TBresult == TB_CURSED_WIN) {
            winningSide = search->board.sideToMove();
        } else if (TBresult == TB_DRAW) {
            winningSide = NO_COLOR;
        }
    }

    double score;
    if (winningSide == WHITE)
        score = 1.0;
    else if (winningSide == BLACK)
        score = 0.0;
    else
        score = 0.5;

    for (auto &f : fens) {
        file << stringFenData(f, score) << "\n";
    }

    file.flush();
}

}  // namespace datagen

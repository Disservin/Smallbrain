#include <fstream>

#include "datagen.h"
#include "randomFen.h"
#include "syzygy/Fathom/src/tbprobe.h"

namespace Datagen
{

std::string stringFenData(const fenData &fenData, double score)
{
    std::ostringstream sstream;
    sstream << fenData.fen << " [" << std::fixed << std::setprecision(1) << score << "] " << fenData.score;

    return sstream.str();
}

void TrainingData::generate(int workers, std::string book, int depth, int nodes, bool useTB)
{
    if (book != "")
    {
        std::ifstream openingFile;
        std::string line;
        openingFile.open(book);

        while (std::getline(openingFile, line))
        {
            openingBook.emplace_back(line);
        }

        openingFile.close();
    }

    for (int i = 0; i < workers; i++)
    {
        threads.emplace_back(&TrainingData::infinitePlay, this, i, depth, nodes, useTB);
    }
}

void TrainingData::infinitePlay(int threadId, int depth, int nodes, bool useTB)
{
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

    uint64_t games = 0;

    auto t0 = TimePoint::now();
    auto t1 = TimePoint::now();

    while (!UCI_FORCE_STOP)
    {
        board.clearStacks();
        search->reset();

        search->normalSearch = false;
        search->useTB = false;
        search->id = 0;
        search->limit = limit;

        movelist.size = 0;

        board.applyFen(DEFAULT_POS, false);

        randomPlayout(file, board, movelist, search, useTB);
        games++;

        if (threadId == 0 && games % 100 == 0)
        {
            t1 = TimePoint::now();
            auto s = std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count();
            std::cout << "\r" << std::left << std::setw(3) << std::setprecision(2) << double(games) / s << "g/s";
        }
    }

    file.close();
}

void TrainingData::randomPlayout(std::ofstream &file, Board &board, Movelist &movelist, std::unique_ptr<Search> &search,
                                 bool useTB)
{
    std::mt19937 generator(rd());

    std::vector<fenData> fens;
    fens.reserve(40);

    int ply = 0;
    int randomMoves = 10;

    if (openingBook.size() != 0)
    {
        std::uniform_int_distribution<std::mt19937::result_type> maxLines{
            0, static_cast<std::mt19937::result_type>(openingBook.size() - 1)};

        uint64_t randLine = maxLines(generator);

        board.applyFen(openingBook[randLine], false);
    }
    // else if (maxLines(e) == 1)
    // {
    //     ply = randomMoves;
    //     board.applyFen(getRandomfen());
    // }

    while (ply < randomMoves)
    {
        movelist.size = 0;
        board.clearStacks();

        Movegen::legalmoves<Movetype::ALL>(board, movelist);

        if (movelist.size == 0)
            return;

        std::uniform_int_distribution<std::mt19937::result_type> randomNum{
            0, static_cast<std::mt19937::result_type>(movelist.size - 1)};

        int index = randomNum(generator);

        Move move = movelist[index].move;
        board.makeMove<false>(move);
        ply++;
    }

    board.refresh();
    board.hashHistory.clear();

    Color winningSide = NO_COLOR;
    int drawCount = 0;
    int winCount = 0;
    fenData sfens;

    search->board = board;

    while (true)
    {
        search->board.clearStacks();

        search->nodes = 0;
        movelist.size = 0;

        const bool inCheck = search->board.isSquareAttacked(
            ~search->board.sideToMove, search->board.KingSQ(search->board.sideToMove), search->board.All());

        Movegen::legalmoves<Movetype::ALL>(search->board, movelist);

        if (movelist.size == 0)
        {
            winningSide = inCheck ? ((search->board.sideToMove == White) ? Black : White) : NO_COLOR;
            break;
        }
        else if (search->board.isRepetition(2) || search->board.halfMoveClock >= 100)
        {
            winningSide = NO_COLOR;
            break;
        }

        const SearchResult result = search->iterativeDeepening();

        // CATCH BUGS
        if (result.score == VALUE_NONE)
        {
            std::cout << search->board << std::endl;
            exit(1);
        }

        const bool capture = search->board.pieceAtB(to(result.move)) != None;

        sfens.score = search->board.sideToMove == White ? result.score : -result.score;
        sfens.move = result.move;

        const Score absScore = std::abs(result.score);

        if (absScore >= 2000)
        {
            winCount++;
            drawCount = 0;
        }
        else if (absScore <= 4)
        {
            drawCount++;
            winCount = 0;
        }
        else
        {
            drawCount = 0;
            winCount = 0;
        }

        if (winCount >= 4 || absScore > VALUE_TB_WIN_IN_MAX_PLY)
        {
            winningSide = result.score > 0 ? search->board.sideToMove : ~search->board.sideToMove;
            break;
        }
        else if (drawCount >= 12)
        {
            winningSide = NO_COLOR;
            break;
        }

        if (!(capture || inCheck || ply < 8))
        {
            sfens.fen = search->board.getFen();
            fens.emplace_back(sfens);
        }

        if (useTB && search->board.halfMoveClock >= 40 && popcount(search->board.All()) <= 6)
            break;

        ply++;
        search->board.makeMove<true>(result.move);
    }

    U64 white = search->board.Us<White>();
    U64 black = search->board.Us<Black>();

    // Set correct winningSide for if (useTB && search->board.halfMoveClock >= 40 && popcount(search->board.All()) <= 6)
    if (useTB && popcount(white | black) <= 6)
    {
        Square ep = search->board.enPassantSquare <= 63 ? search->board.enPassantSquare : Square(0);

        unsigned TBresult =
            tb_probe_wdl(white, black, search->board.pieces<WhiteKing>() | search->board.pieces<BlackKing>(),
                         search->board.pieces<WhiteQueen>() | search->board.pieces<BlackQueen>(),
                         search->board.pieces<WhiteRook>() | search->board.pieces<BlackRook>(),
                         search->board.pieces<WhiteBishop>() | search->board.pieces<BlackBishop>(),
                         search->board.pieces<WhiteKnight>() | search->board.pieces<BlackKnight>(),
                         search->board.pieces<WhitePawn>() | search->board.pieces<BlackPawn>(), 0, 0, ep,
                         search->board.sideToMove == White); //  * - turn: true=white, false=black

        if (TBresult == TB_LOSS)
        {
            winningSide = ~search->board.sideToMove;
        }
        else if (TBresult == TB_WIN)
        {
            winningSide = search->board.sideToMove;
        }
        else if (TBresult == TB_DRAW || TB_BLESSED_LOSS || TB_CURSED_WIN)
        {
            winningSide = NO_COLOR;
        }
    }

    double score;
    if (winningSide == White)
        score = 1.0;
    else if (winningSide == Black)
        score = 0.0;
    else
        score = 0.5;

    for (auto &f : fens)
    {
        file << stringFenData(f, score) << "\n";
    }

    file.flush();
}

} // namespace Datagen

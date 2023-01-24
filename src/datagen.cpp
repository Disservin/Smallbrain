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

    Board board = Board();
    Movelist movelist;

    Time t;
    t.maximum = 0;
    t.optimum = 0;

    Limits limit;
    limit.depth = depth == 0 ? MAX_PLY - 1 : depth;
    limit.nodes = nodes;
    limit.time = t;

    while (!UCI_FORCE_STOP)
    {
        std::unique_ptr<Search> search = std::make_unique<Search>();

        search->normalSearch = false;
        search->useTB = false;
        search->id = 0;
        search->limit = limit;

        randomPlayout(file, board, movelist, search, useTB);
    }

    file.close();
}

void TrainingData::randomPlayout(std::ofstream &file, Board &board, Movelist &movelist, std::unique_ptr<Search> &search,
                                 bool useTB)
{
    std::vector<fenData> fens;
    std::mt19937 generator(rd());

    movelist.size = 0;
    board.applyFen(DEFAULT_POS);

    int ply = 0;
    int randomMoves = 10;

    if (openingBook.size() != 0)
    {
        std::uniform_int_distribution<std::mt19937::result_type> maxLines{
            0, static_cast<std::mt19937::result_type>(openingBook.size() - 1)};

        uint64_t randLine = maxLines(generator);

        board.applyFen(openingBook[randLine]);
    }
    // else if (maxLines(e) == 1)
    // {
    //     ply = randomMoves;
    //     board.applyFen(getRandomfen());
    // }

    while (ply < randomMoves)
    {
        board.clearStacks();

        search->nodes = 0;
        movelist.size = 0;

        Movegen::legalmoves<Movetype::ALL>(board, movelist);

        if (movelist.size == 0)
            return;

        std::uniform_int_distribution<std::mt19937::result_type> randomNum{
            0, static_cast<std::mt19937::result_type>(movelist.size - 1)};

        int index = randomNum(generator);

        Move move = movelist[index].move;
        board.makeMove<true>(move);
        ply++;
    }

    board.hashHistory.clear();

    movelist.size = 0;
    Movegen::legalmoves<Movetype::ALL>(board, movelist);

    if (movelist.size == 0)
        return;

    Color winningSide = NO_COLOR;
    int drawCount = 0;
    int winCount = 0;

    search->board = board;

    while (true)
    {
        board.clearStacks();

        search->nodes = 0;
        movelist.size = 0;

        const bool inCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));

        Movegen::legalmoves<Movetype::ALL>(board, movelist);

        if (movelist.size == 0)
        {
            winningSide = inCheck ? ((board.sideToMove == White) ? Black : White) : NO_COLOR;
            break;
        }
        else if (board.isRepetition(2) || board.halfMoveClock >= 100)
        {
            winningSide = NO_COLOR;
            break;
        }

        SearchResult result = search->iterativeDeepening();

        // CATCH BUGS
        if (result.score == VALUE_NONE)
        {
            std::cout << board << std::endl;
            exit(1);
        }

        const bool capture = board.pieceAtB(to(result.move)) != None;

        fenData fn;
        fn.score = board.sideToMove == White ? result.score : -result.score;
        fn.move = result.move;
        fn.use = !(capture || inCheck || ply < 8);

        if (fn.use)
            fn.fen = board.getFen();
        else
            fn.fen = "";

        Score absScore = std::abs(result.score);

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
            winningSide = result.score > 0 ? board.sideToMove : ~board.sideToMove;
            break;
        }
        else if (drawCount >= 12)
        {
            winningSide = NO_COLOR;
            break;
        }

        fens.emplace_back(fn);

        if (useTB && board.halfMoveClock >= 40 && popcount(board.All()) <= 6)
            break;

        ply++;
        board.makeMove<true>(result.move);
        search->board = board;
    }

    U64 white = board.Us<White>();
    U64 black = board.Us<Black>();
    // Set correct winningSide for if (useTB && board.halfMoveClock >= 40 && popcount(board.All()) <= 6)
    if (useTB && popcount(white | black) <= 6)
    {
        Square ep = board.enPassantSquare <= 63 ? board.enPassantSquare : Square(0);

        unsigned TBresult = tb_probe_wdl(white, black, board.pieces<WhiteKing>() | board.pieces<BlackKing>(),
                                         board.pieces<WhiteQueen>() | board.pieces<BlackQueen>(),
                                         board.pieces<WhiteRook>() | board.pieces<BlackRook>(),
                                         board.pieces<WhiteBishop>() | board.pieces<BlackBishop>(),
                                         board.pieces<WhiteKnight>() | board.pieces<BlackKnight>(),
                                         board.pieces<WhitePawn>() | board.pieces<BlackPawn>(), 0, 0, ep,
                                         board.sideToMove == White); //  * - turn: true=white, false=black

        if (TBresult == TB_LOSS)
        {
            winningSide = ~board.sideToMove;
        }
        else if (TBresult == TB_WIN)
        {
            winningSide = board.sideToMove;
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
        if (f.use)
            file << stringFenData(f, score) << std::endl;
    }
}

} // namespace Datagen

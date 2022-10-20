#include "datagen.h"

#include <fstream>

// random number generator
std::random_device rd;
std::mt19937 e(rd());

namespace Datagen
{

std::string stringFenData(fenData fenData, double score)
{
    std::ostringstream sstream;
    sstream << std::fixed << std::setprecision(1) << score;
    return std::string{fenData.fen + " [" + sstream.str() + "] " + std::to_string(fenData.score)};
}

void TrainingData::generate(int workers, std::string book, int depth)
{
    for (int i = 0; i < workers; i++)
    {
        threads.emplace_back(&TrainingData::infinitePlay, this, i, book, depth);
    }
}

void TrainingData::infinitePlay(int threadId, std::string book, int depth)
{
    std::ofstream file;
    std::string filename = "data/data" + std::to_string(threadId) + ".txt";
    file.open(filename, std::ios::app);

    int number_of_lines = 0;
    if (book != "")
    {
        std::ifstream openingFile;
        std::string line;
        openingFile.open(book);

        while (std::getline(openingFile, line))
            ++number_of_lines;

        openingFile.close();
    }

    U64 games = 0;
    while (!UCI_FORCE_STOP)
    {
        games++;
        randomPlayout(file, threadId, book, depth, number_of_lines);
    }
    file.close();
}

void TrainingData::randomPlayout(std::ofstream &file, int threadId, std::string &book, int depth, int numLines)
{
    Board board;

    std::vector<fenData> fens;
    Movelist movelist;
    movelist.size = 0;
    int ply = 0;
    int randomMoves = 10;

    std::uniform_int_distribution<std::mt19937::result_type> maxLines{0, static_cast<std::mt19937::result_type>(15)};
    if (maxLines(e) == 1)
    {
        ply = randomMoves;
        board.applyFen(getRandomfen());
    }
    if (maxLines(e) % 5 == 0 && book != "")
    {
        std::ifstream openingFile;
        openingFile.open(book);

        std::string line;
        uint64_t count = 0;

        std::uniform_int_distribution<std::mt19937::result_type> maxLines{
            0, static_cast<std::mt19937::result_type>(numLines)};

        uint64_t randLine = maxLines(e);
        while (std::getline(openingFile, line))
        {
            if (count == randLine)
            {
                board.applyFen(line);
                break;
            }
            count++;
        }
        openingFile.close();
    }
    else
    {
        board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }

    while (ply < randomMoves)
    {
        movelist.size = 0;
        Movegen::legalmoves<Movetype::ALL>(board, movelist);
        if (movelist.size == 0 || UCI_FORCE_STOP)
            return;

        std::uniform_int_distribution<std::mt19937::result_type> randomNum{
            0, static_cast<std::mt19937::result_type>(movelist.size - 1)};
        int index = randomNum(e);

        Move move = movelist[index].move;
        board.makeMove<true>(move);
        ply++;
    }

    movelist.size = 0;
    board.hashHistory.clear();

    Movegen::legalmoves<Movetype::ALL>(board, movelist);
    if (movelist.size == 0)
        return;

    Search search = Search();

    ThreadData td;
    td.board = board;
    td.id = 0;
    td.allowPrint = false;
    search.tds.push_back(td);

    constexpr uint64_t nodes = 0;

    Time t;

    int drawCount = 0;
    int winCount = 0;

    Color winningSide = NO_COLOR;

    while (true)
    {
        const bool inCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
        movelist.size = 0;
        Movegen::legalmoves<Movetype::ALL>(board, movelist);
        if (movelist.size == 0)
        {
            winningSide = inCheck ? ((board.sideToMove == White) ? Black : White) : NO_COLOR;
            break;
        }
        else if (board.isRepetition(3) || board.halfMoveClock >= 100)
        {
            winningSide = NO_COLOR;
            break;
        }

        SearchResult result = search.iterativeDeepening(depth, nodes, t, 0);

        // CATCH BUGS
        if (result.score == VALUE_NONE)
        {
            board.print();
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
            if (winCount >= 4 || absScore > VALUE_TB_WIN_IN_MAX_PLY)
            {
                winningSide = result.score > 0 ? board.sideToMove : ~board.sideToMove;
                break;
            }
        }
        else if (absScore <= 4)
        {
            drawCount++;
            if (drawCount >= 12)
            {
                winningSide = NO_COLOR;
                break;
            }
            winCount = 0;
        }
        else
        {
            drawCount = 0;
            winCount = 0;
        }

        if (board.halfMoveClock >= 50 && popcount(board.All()) <= 6 && useTB)
        {
            Square ep = board.enPassantSquare <= 63 ? board.enPassantSquare : Square(0);

            unsigned TBresult = tb_probe_wdl(
                board.Us<White>(), board.Us<Black>(), board.Kings<White>() | board.Kings<Black>(),
                board.Queens<White>() | board.Queens<Black>(), board.Rooks<White>() | board.Rooks<Black>(),
                board.Bishops<White>() | board.Bishops<Black>(), board.Knights<White>() | board.Knights<Black>(),
                board.Pawns<White>() | board.Pawns<Black>(), 0, board.castlingRights, ep,
                ~board.sideToMove); //  * - turn: true=white, false=black

            if (TBresult == TB_LOSS || TBresult == TB_BLESSED_LOSS)
            {
                winningSide = ~board.sideToMove;
                break;
            }
            else if (TBresult == TB_WIN || TBresult == TB_CURSED_WIN)
            {
                winningSide = board.sideToMove;
                break;
            }
            else if (TBresult == TB_DRAW)
            {
                winningSide = NO_COLOR;
                break;
            }
        }

        ply++;
        fens.push_back(fn);

        board.makeMove<true>(result.move);
        search.tds[0].board = board;
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

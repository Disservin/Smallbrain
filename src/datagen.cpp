#include "datagen.h"

// random number generator
std::random_device rd;
std::mt19937 e(rd()); // or std::default_random_engine e{rd()};

void Datagen::generate(int workers, std::string book)
{
    for (std::thread &th : threads)
    {
        if (th.joinable())
            th.join();
    }
    threads.clear();

    for (int i = 0; i < workers; i++)
    {
        threads.emplace_back(&Datagen::infinitePlay, this, i, book);
    }
}

void Datagen::infinitePlay(int threadId, std::string book)
{
    std::ofstream file;
    std::string filename = "data/data" + std::to_string(threadId) + ".txt";
    file.open(filename, std::ios::app);

    U64 games = 0;
    while (!UCI_FORCE_STOP)
    {
        games++;
        randomPlayout(file, threadId, book);
    }
    file.close();
}

void Datagen::randomPlayout(std::ofstream &file, int threadId, std::string &book)
{
    Board board;
    board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::vector<fenData> fens;
    Movelist movelist;
    int ply = 0;
    int randomMoves = 10;

    // USE BOOK
    std::uniform_int_distribution<std::mt19937::result_type> dfrc{0, static_cast<std::mt19937::result_type>(4)};
    if (book != "" && dfrc(e) == 0)
    {
        std::ifstream openingFile;
        openingFile.open(book);

        std::string line;
        uint64_t count = 0;

        std::uniform_int_distribution<std::mt19937::result_type> maxLines{
            0, static_cast<std::mt19937::result_type>(921'600)};

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

    while (ply < randomMoves)
    {
        movelist = board.legalmoves();
        if (movelist.size == 0 || UCI_FORCE_STOP)
            return;

        std::uniform_int_distribution<std::mt19937::result_type> randomNum{
            0, static_cast<std::mt19937::result_type>(movelist.size - 1)};
        int index = randomNum(e);

        Move move = movelist.list[index];
        board.makeMove<true>(move);
        ply++;
    }

    if (movelist.size == 0 || board.isRepetition(3))
        return;

    Search search = Search();

    ThreadData td;
    td.board = board;
    td.id = 0;
    td.allowPrint = false;
    search.tds.push_back(td);

    constexpr uint64_t nodes = 0;
    constexpr int depth = 7;

    Time t;

    int drawCount = 0;
    int winCount = 0;

    Color winningSide = NO_COLOR;
    while (true)
    {
        const bool inCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));

        ply++;
        movelist = board.legalmoves();

        if (movelist.size == 0)
        {
            winningSide = inCheck ? ((board.sideToMove == White) ? Black : White) : NO_COLOR;
            break;
        }

        SearchResult result = search.iterativeDeepening(depth, nodes, t, 0);

        // Shouldnt happen
        if (result.score == VALUE_NONE)
        {
            std::cout << "meh" << std::endl;
            board.print();
            exit(1);
        }

        int all = popcount(board.All());

        if (board.isRepetition(3) || board.halfMoveClock >= 100)
        {
            winningSide = NO_COLOR;
            break;
        }
        else if (all == 2)
        {
            winningSide = NO_COLOR;
            break;
        }
        else if (all == 3 && (board.Bitboards[WhiteKnight] || board.Bitboards[BlackKnight] ||
                              board.Bitboards[WhiteBishop] || board.Bitboards[BlackBishop]))
        {
            winningSide = NO_COLOR;
            break;
        }

        const bool capture = board.pieceAtB(to(result.move)) != None;

        fenData fn;
        fn.score = board.sideToMove == White ? result.score : -result.score;
        fn.move = result.move;
        fn.use = !(capture || inCheck || ply < 12 || promoted(result.move));

        Score absScore = std::abs(result.score);
        if (absScore >= 1500 && ply == randomMoves)
            return;
        else if (absScore <= 4)
        {
            drawCount++;
            winCount = 0;
        }
        else if (absScore >= 1500)
        {
            winCount++;
            drawCount = 0;
        }
        else
        {
            drawCount = 0;
            winCount = 0;
        }

        if (all <= 5 && useTB)
        {
            Square ep = board.enPassantSquare <= 63 ? board.enPassantSquare : Square(0);

            unsigned TBresult =
                tb_probe_wdl(board.Us(White), board.Us(Black), board.Kings(White) | board.Kings(Black),
                             board.Queens(White) | board.Queens(Black), board.Rooks(White) | board.Rooks(Black),
                             board.Bishops(White) | board.Bishops(Black), board.Knights(White) | board.Knights(Black),
                             board.Pawns(White) | board.Pawns(Black), 0, board.castlingRights, ep,
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
        else if (winCount >= 4 || absScore >= VALUE_TB_WIN_IN_MAX_PLY)
        {
            winningSide = result.score > 0 ? board.sideToMove : ~board.sideToMove;
            break;
        }
        else if (drawCount >= 12)
        {
            winningSide = NO_COLOR;
            break;
        }

        if (fn.use)
            fn.fen = board.getFen();
        else
            fn.fen = "";

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

std::string stringFenData(fenData fenData, double score)
{
    std::ostringstream sstream;
    sstream << std::fixed << std::setprecision(1) << score;
    return std::string{fenData.fen + " [" + sstream.str() + "] " + std::to_string(fenData.score)};
}
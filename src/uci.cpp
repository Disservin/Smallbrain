#include "uci.h"
#include "benchmark.h"
#include "evaluation.h"
#include "helper.h"
#include "nnue.h"
#include "perft.h"
#include "syzygy/Fathom/src/tbprobe.h"
#include "tests/tests.h"
#include "thread.h"
#include "tt.h"

extern std::atomic_bool UCI_FORCE_STOP;
extern TranspositionTable TTable;
extern ThreadPool Threads;

uciOptions options = uciOptions();

// START OF TUNE
// TUNE_INT(BONUS);
// END OF TUNE

UCI::UCI()
{
    useTB = false;

    threadCount = 1;

    // load default position
    board.applyFen(DEFAULT_POS);

    // Initialize reductions used in search
    init_reductions();
}

int UCI::uciLoop(int argc, char **argv)
{
    std::vector<std::string> allArgs(argv + 1, argv + argc);

    if (argc > 1 && parseArgs(argc, argv, options))
        return 0;

    // START OF TUNE

    // TUNE_INT(razorMargin, -100, 100);

    // END OF TUNE

    while (true)
    {
        // catching inputs
        std::string input;

        if (!std::getline(std::cin, input) && argc == 1)
            input = "quit";

        if (input == "quit")
        {
            quit();
            return 0;
        }
        else
        {
            processCommand(input);
        }
    }

    return 0;
}

void UCI::processCommand(std::string command)
{
    std::vector<std::string> tokens = splitInput(command);

    if (tokens[0] == "stop")
    {
        Threads.stop_threads();
    }
    else if (tokens[0] == "ucinewgame")
    {
        ucinewgameInput();
    }
    else if (tokens[0] == "uci")
    {
        uciInput();
    }
    else if (tokens[0] == "isready")
    {
        isreadyInput();
    }
    else if (tokens[0] == "setoption")
    {
        std::string option = tokens[2];
        std::string value = tokens[4];

        if (option == "Hash")
            options.uciHash(std::stoi(value));
        else if (option == "EvalFile")
            options.uciEvalFile(value);
        else if (option == "Threads")
            threadCount = options.uciThreads(std::stoi(value));
        else if (option == "SyzygyPath")
            useTB = options.uciSyzygy(command);
        else if (option == "UCI_Chess960")
            options.uciChess960(board, value);
    }
    else if (tokens[0] == "position")
    {
        setPosition(tokens, command);
    }
    else if (contains(command, "go perft"))
    {
        int depth = findElement<int>("perft", tokens);
        Perft perft = Perft();
        perft.board = board;
        perft.perfTest(depth, depth);
    }
    else if (tokens[0] == "go")
    {
        startSearch(tokens, command);
    }
    else if (command == "print")
    {
        std::cout << board << std::endl;
    }
    else if (command == "fen")
    {
        std::cout << getRandomfen() << std::endl;
    }
    else if (command == "captures")
    {
        Movelist moves;
        Movegen::legalmoves<Movetype::CAPTURE>(board, moves);

        for (auto ext : moves)
            std::cout << uciMove(ext.move, board.chess960) << std::endl;

        std::cout << "count: " << signed(moves.size) << std::endl;
    }
    else if (command == "moves")
    {
        Movelist moves;
        Movegen::legalmoves<Movetype::ALL>(board, moves);

        for (auto ext : moves)
            std::cout << uciMove(ext.move, board.chess960) << std::endl;

        std::cout << "count: " << signed(moves.size) << std::endl;
    }

    else if (command == "rep")
    {
        std::cout << board.isRepetition(2) << std::endl;
    }

    else if (command == "eval")
    {
        std::cout << Eval::evaluation(board) << std::endl;
    }

    else if (command == "perft")
    {
        Perft perft = Perft();
        perft.board = board;
        perft.testAllPos();
    }
    else if (contains(command, "move"))
    {
        if (contains(tokens, "move"))
        {
            std::size_t index = std::find(tokens.begin(), tokens.end(), "move") - tokens.begin();
            index++;

            for (; index < tokens.size(); index++)
            {
                Move move = convertUciToMove(board, tokens[index]);
                board.makeMove<true>(move);
            }
        }
    }
    else
    {
        std::cout << "Unknown command: " << command << std::endl;
    }
}

bool UCI::parseArgs(int argc, char **argv, uciOptions options)
{
    std::vector<std::string> allArgs(argv + 1, argv + argc);

    // ./smallbrain bench
    if (contains(allArgs, "go"))
    {
        board.applyFen("r2qk2r/1p1n1pp1/p2p4/3Pp2p/8/1N1QbP2/PPP3PP/1K1R3R w kq - 0 16");
        std::stringstream ss;

        for (auto str : allArgs)
            ss << str;

        startSearch(allArgs, ss.str());

        // wait for finish
        while (!Threads.stop)
        {
        };

        Threads.stop_threads();

        Bench::startBench(12);

        quit();

        return true;
    }

    if (contains(allArgs, "bench"))
    {
        Bench::startBench(contains(allArgs, "depth") ? findElement<int>("depth", allArgs) : 12);
        quit();
        return true;
    }
    else if (contains(allArgs, "perft"))
    {
        int n = 1;
        if (contains(allArgs, "-n"))
            n = findElement<int>("-n", allArgs);

        Perft perft = Perft();
        perft.board = board;
        perft.testAllPos(n);
        quit();
        return true;
    }
    else if (contains(allArgs, "-gen"))
    {
        std::string bookPath = "";
        std::string tbPath = "";
        int workers = 1;
        int depth = 7;
        int nodes = 0;
        int hash = 16;
        int random = 0;
        bool useTB = false;

        if (contains(allArgs, "-threads"))
        {
            workers = findElement<int>("-threads", allArgs);
        }

        if (contains(allArgs, "-book"))
        {
            bookPath = findElement<std::string>("-book", allArgs);
        }

        if (contains(allArgs, "-tb"))
        {
            std::string s = "setoption name SyzygyPath value " + findElement<std::string>("-tb", allArgs);
            useTB = options.uciSyzygy(s);
        }

        if (contains(allArgs, "-depth"))
        {
            depth = findElement<int>("-depth", allArgs);
        }

        if (contains(allArgs, "-nodes"))
        {
            depth = 0;
            nodes = findElement<int>("-nodes", allArgs);
        }

        if (contains(allArgs, "-hash"))
        {
            hash = findElement<int>("-hash", allArgs);
        }

        if (contains(allArgs, "-random"))
        {
            random = findElement<int>("-random", allArgs);
        }

        int ttsize = hash * 1024 * 1024 / sizeof(TEntry); // 16 MiB
        TTable.allocateTT(ttsize * workers);

        UCI_FORCE_STOP = false;

        datagen.generate(workers, bookPath, depth, nodes, useTB, random);

        std::cout << "Data generation started" << std::endl;
        std::cout << "Workers: " << workers << "\nBookPath: " << bookPath << "\nDepth: " << depth
                  << "\nNodes: " << nodes << "\nUseTb: " << useTB << "\nHash: " << hash << "\nRandom: " << random
                  << std::endl;

        return false;
    }
    else if (contains(allArgs, "-tests"))
    {
        assert(Tests::testAll());
        return true;
    }
    else
    {
        std::cout << "Unknown argument" << std::endl;
    }
    return false;
}

void UCI::uciInput()
{
    std::cout << "id name " << getVersion() << std::endl;
    std::cout << "id author Disservin\n" << std::endl;
    options.printOptions();
    std::cout << "uciok" << std::endl;
}

void UCI::isreadyInput()
{
    std::cout << "readyok" << std::endl;
}

void UCI::ucinewgameInput()
{
    board.applyFen(DEFAULT_POS);
    Threads.stop_threads();
    TTable.clearTT();
}

void UCI::quit()
{
    Threads.stop_threads();

    for (std::thread &th : datagen.threads)
    {
        if (th.joinable())
            th.join();
    }

    Threads.pool.clear();
    datagen.threads.clear();

    tb_free();
}

const std::string UCI::getVersion()
{
    std::unordered_map<std::string, std::string> months({{"Jan", "01"},
                                                         {"Feb", "02"},
                                                         {"Mar", "03"},
                                                         {"Apr", "04"},
                                                         {"May", "05"},
                                                         {"Jun", "06"},
                                                         {"Jul", "07"},
                                                         {"Aug", "08"},
                                                         {"Sep", "09"},
                                                         {"Oct", "10"},
                                                         {"Nov", "11"},
                                                         {"Dec", "12"}});

    std::string month, day, year;
    std::stringstream ss, date(__DATE__); // {month} {date} {year}

    const std::string version = "dev";

    ss << "Smallbrain " << version;
    ss << "-";
#ifdef GIT_DATE
    ss << GIT_DATE;
#else

    date >> month >> day >> year;
    if (day.length() == 1)
        day = "0" + day;
    ss << year.substr(2) << months[month] << day;
#endif

#ifdef GIT_SHA
    ss << "-" << GIT_SHA;
#endif

    return ss.str();
}

void UCI::uciMoves(const std::vector<std::string> &tokens)
{
    std::size_t index = std::find(tokens.begin(), tokens.end(), "moves") - tokens.begin();
    index++;
    for (; index < tokens.size(); index++)
    {
        Move move = convertUciToMove(board, tokens[index]);
        board.makeMove<false>(move);
    }
}

void UCI::startSearch(const std::vector<std::string> &tokens, const std::string &command)
{
    Limits info;
    std::string limit;

    Threads.stop_threads();

    if (tokens.size() == 1)
        limit = "";
    else
        limit = tokens[1];

    info.depth = (limit == "depth") ? findElement<int>("depth", tokens) : MAX_PLY - 1;
    info.depth = command == "go" ? MAX_PLY - 1 : info.depth;
    info.infinite = limit == "infinite";
    info.nodes = (limit == "nodes") ? findElement<int>("nodes", tokens) : 0;
    info.time.maximum = info.time.optimum = (limit == "movetime") ? findElement<int>("movetime", tokens) : 0;

    searchmoves.size = 0;

    const std::string keyword = "searchmoves";
    if (contains(tokens, keyword))
    {
        std::size_t index = std::find(tokens.begin(), tokens.end(), keyword) - tokens.begin() + 1;
        for (; index < tokens.size(); index++)
        {
            Move move = convertUciToMove(board, tokens[index]);
            searchmoves.Add(move);
        }
    }

    std::string side_str = board.sideToMove == White ? "wtime" : "btime";
    std::string inc_str = board.sideToMove == White ? "winc" : "binc";

    if (contains(tokens, side_str))
    {
        int64_t timegiven = findElement<int>(side_str, tokens);
        int64_t inc = 0;
        int64_t mtg = 0;

        // Increment
        if (contains(tokens, inc_str))
            inc = findElement<int>(inc_str, tokens);

        // Moves to next time control
        if (contains(tokens, "movestogo"))
            mtg = findElement<int>("movestogo", tokens);

        // Calculate search time
        info.time = optimumTime(timegiven, inc, mtg);
    }

    // start search
    Threads.start_threads(board, info, searchmoves, threadCount, useTB);
}

void UCI::setPosition(const std::vector<std::string> &tokens, const std::string &command)
{
    Threads.stop_threads();

    bool hasMoves = contains(tokens, "moves");

    if (tokens[1] == "fen")
        board.applyFen(command.substr(command.find("fen") + 4), false);
    else
        board.applyFen(DEFAULT_POS, false);

    if (hasMoves)
        uciMoves(tokens);

    // setup accumulator with the correct board
    board.refresh();
}

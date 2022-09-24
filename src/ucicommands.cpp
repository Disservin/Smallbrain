#include "ucicommands.h"

namespace uciCommand
{

void uciInput(uciOptions options)
{
    std::cout << "id name Smallbrain dev " << uciCommand::currentDateTime() << std::endl;
    std::cout << "id author Disservin\n" << std::endl;
    options.printOptions();
    std::cout << "uciok" << std::endl;
}

void isreadyInput()
{
    std::cout << "readyok" << std::endl;
}

void ucinewgameInput(uciOptions &options, Board &board, Search &searcher, Datagen &dg)
{
    options.uciPosition(board);
    stopThreads(searcher, dg);
    searcher.tds.clear();
}

void parseInput(std::string input, Search &searcher, Board &board, Datagen &dg)
{
    if (input == "print")
    {
        board.print();
    }

    else if (input == "captures")
    {
        Movelist ml = board.capturemoves();
        for (int i = 0; i < ml.size; i++)
            std::cout << printMove(ml.list[i]) << std::endl;
        std::cout << "count: " << signed(ml.size) << std::endl;
    }

    else if (input == "moves")
    {
        Movelist ml = board.legalmoves();
        for (int i = 0; i < ml.size; i++)
            std::cout << printMove(ml.list[i]) << std::endl;
        std::cout << "count: " << signed(ml.size) << std::endl;
    }

    else if (input == "rep")
    {
        std::cout << board.isRepetition(3) << std::endl;
    }

    else if (input == "eval")
    {
        std::cout << Eval::evaluation(board) << std::endl;
    }

    else if (input == "perft")
    {
        Perft perft = Perft();
        perft.board = board;
        perft.testAllPos();
        quit(searcher, dg);
        exit(0);
    }
}

void stopThreads(Search &searcher, Datagen &dg)
{
    stopped = true;
    UCI_FORCE_STOP = true;

    for (std::thread &th : searcher.threads)
    {
        if (th.joinable())
            th.join();
    }

    for (std::thread &th : dg.threads)
    {
        if (th.joinable())
            th.join();
    }

    searcher.threads.clear();
    dg.threads.clear();
}

void quit(Search &searcher, Datagen &dg)
{
    stopThreads(searcher, dg);
    free(TTable);
    tb_free();
}

bool elementInVector(std::string el, std::vector<std::string> tokens)
{
    return std::find(tokens.begin(), tokens.end(), el) != tokens.end();
}

int findElement(std::string param, std::vector<std::string> tokens)
{
    int index = find(tokens.begin(), tokens.end(), param) - tokens.begin();
    return std::stoi(tokens[index + 1]);
}

std::string findElementString(std::string param, std::vector<std::string> tokens)
{
    int index = find(tokens.begin(), tokens.end(), param) - tokens.begin();
    return tokens[index + 1];
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%d%m%Y", &tstruct);

    return buf;
}

} // namespace uciCommand
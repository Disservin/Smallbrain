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

void ucinewgameInput(uciOptions &options, Board &board, Search &searcher, Datagen::TrainingData &dg)
{
    options.uciPosition(board);
    stopThreads(searcher, dg);
    searcher.tds.clear();
}

void parseInput(std::string input, Search &searcher, Board &board, Datagen::TrainingData &dg)
{
    if (input == "print")
    {
        board.print();
    }

    else if (input == "captures")
    {
        Movelist moves;
        Movegen::legalmoves<CAPTURE>(board, moves);
        for (int i = 0; i < moves.size; i++)
            std::cout << uciRep(moves[i].move) << std::endl;
        std::cout << "count: " << signed(moves.size) << std::endl;
    }

    else if (input == "moves")
    {
        Movelist moves;
        Movegen::legalmoves<ALL>(board, moves);
        for (int i = 0; i < moves.size; i++)
            std::cout << uciRep(moves[i].move) << std::endl;
        std::cout << "count: " << signed(moves.size) << std::endl;
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
    }
}

void stopThreads(Search &searcher, Datagen::TrainingData &dg)
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

    stopped = false;
}

void quit(Search &searcher, Datagen::TrainingData &dg)
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

bool stringContain(std::string s, std::string origin)
{
    return origin.find(s) != std::string::npos;
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
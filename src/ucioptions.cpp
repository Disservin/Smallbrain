#include "syzygy/Fathom/src/tbprobe.h"

#include "ucioptions.h"

// clang-format off
std::vector<optionType> optionsPrint{
    optionType("Hash",         "spin",   "16",          "1", "57344"),
    optionType("EvalFile",     "string", NETWORK_NAME,   "",  ""),
    optionType("Threads",      "spin",   "1",            "1", "256"),
    optionType("SyzygyPath",   "string", "<empty>",      "",  ""),
    optionType("UCI_Chess960", "check",  "false",        "",  "")
};

// clang-format on

void uciOptions::printOptions()
{
    for (const auto &option : optionsPrint)
    {
        std::cout << "option name " << option.name << " type " << option.type << " default " << option.defaultValue;
        if (option.min != "")
            std::cout << " min " << option.min << " max " << option.max;

        std::cout << std::endl;
    }
}

void uciOptions::uciHash(int value)
{
    // value * 10^6 / 2^20
    int sizeMiB = static_cast<uint64_t>(value) * 1000000 / 1048576;
    sizeMiB = std::clamp(sizeMiB, 1, MAXHASH);
    U64 elements = (static_cast<uint64_t>(sizeMiB) * 1024 * 1024) / sizeof(TEntry);
    TTable.allocateTT(elements);
}

void uciOptions::uciEvalFile(std::string name)
{
    std::cout << "Loading eval file: " << name << std::endl;
    NNUE::init(name.c_str());
}

int uciOptions::uciThreads(int value)
{
    return std::clamp(value, 1, 512);
}

bool uciOptions::uciSyzygy(std::string input)
{
    std::string path = input.substr(input.find("value ") + 6);

    tb_init(path.c_str());
    if (TB_LARGEST == 0)
    {
        std::cout << "TB NOT FOUND" << std::endl;
        return false;
    }
    else
    {
        std::cout << "using " << TB_LARGEST << " syzygy " << std::endl;
        return true;
    }
}

void uciOptions::uciChess960(Board &board, std::string_view v)
{
    board.chess960 = v == "true";
}

bool uciOptions::addIntTuneOption(std::string name, std::string type, int defaultValue, int min, int max)
{
    optionsPrint.emplace_back(
        optionType(name, type, std::to_string(defaultValue), std::to_string(min), std::to_string(max)));
    return true;
}

bool uciOptions::addDoubleTuneOption(std::string name, std::string type, double defaultValue, double min, double max)
{
    optionsPrint.emplace_back(
        optionType(name, type, std::to_string(defaultValue), std::to_string(min), std::to_string(max)));
    return true;
}
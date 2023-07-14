#include "thread.h"
#include "uci.h"
#include "cli.h"

// Transposition Table
// Each entry is 14 bytes large
TranspositionTable TTable{};
ThreadPool Threads;

int main(int argc, char const *argv[]) {
    Threads.stop = false;

    // Initialize NNUE
    // This either loads the weights from a file or makes use of the weights in the binary file that
    // it was compiled with.
    nnue::init("");

    init_reductions();

    ArgumentsParser optionsParser = ArgumentsParser();

    if (optionsParser.parse(argc, argv) >= 1) return 0;

    uci::Uci communication = uci::Uci();
    communication.uciLoop();
}

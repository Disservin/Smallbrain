#include <atomic>

#include "rand.h"
#include "thread.h"
#include "uci.h"
#include "cli.h"

// Transposition Table
// Each entry is 14 bytes large
TranspositionTable TTable{};
ThreadPool Threads;

std::atomic_bool UCI_FORCE_STOP;

int main(int argc, char const *argv[]) {
    Threads.stop = false;
    UCI_FORCE_STOP = false;

    // Initialize NNUE
    // This either loads the weights from a file or makes use of the weights in the binary file that
    // it was compiled with.
    nnue::init("");

    init_reductions();

    OptionsParser optionsParser(argc, argv);

    UCI communication = UCI();
    communication.uciLoop();
}
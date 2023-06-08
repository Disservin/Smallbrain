#include <atomic>

#include "syzygy/Fathom/src/tbprobe.h"

#include "rand.h"
#include "thread.h"
#include "uci.h"
#include "cli.h"

// Transposition Table
// Each entry is 14 bytes large
TranspositionTable TTable{};
ThreadPool Threads;

std::atomic_bool UCI_FORCE_STOP;

void quit() {
    Threads.stop_threads();
    tb_free();
}

int main(int argc, char const *argv[]) {
    std::atexit(quit);

    Threads.stop = false;
    UCI_FORCE_STOP = false;

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

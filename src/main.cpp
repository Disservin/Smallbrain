
#include "uci.h"
#include <atomic>
#include <signal.h>

// initialise to 16 MB
U64 TT_SIZE = 16 * 1024 * 1024 / sizeof(TEntry);

// Transposition Table
// Each entry is 14 bytes large
TEntry *TTable;

std::atomic<bool> searchStop;
std::atomic<bool> UCI_FORCE_STOP;

int main(int argc, char **argv)
{
    UCI_FORCE_STOP = false;
    searchStop = false;

    // Initialize TT
    allocateTT();

    // Initialize NNUE
    // This either loads the weights from a file or makes use of the weights in the binary file that it was compiled
    // with.
    NNUE::init("");

    UCI communication = UCI();
    communication.uciLoop(argc, argv);
}
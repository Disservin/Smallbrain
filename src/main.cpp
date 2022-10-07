
#include "uci.h"
#include <signal.h>

// initialise to 16 MB
U64 TT_SIZE = 16 * 1024 * 1024 / sizeof(TEntry);

// Transposition Table
// Each entry is 14 bytes large
TEntry *TTable;

std::atomic<bool> stopped;
std::atomic<bool> UCI_FORCE_STOP;
std::atomic<bool> useTB;

int main(int argc, char **argv)
{
    UCI_FORCE_STOP = false;
    stopped = false;
    useTB = false;

    // Initialize TT
    allocateTT();

    // Initialize NNUE
    // This either loads the weights from a file or makes use of the weights in the binary file that it was compiled
    // with.
    NNUE::init("default.nnue");

    UCI communication = UCI();
    communication.uciLoop(argc, argv);
}
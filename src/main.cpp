#include "uci.h"

U64 TT_SIZE = 16 * 1024 * 1024 / sizeof(TEntry); // initialise to 16 MB
TEntry *TTable;                                  // TEntry size is 14 bytes

std::atomic<bool> stopped;
std::atomic<bool> UCI_FORCE_STOP;
std::atomic<bool> useTB;

// TEST CI

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

    // Initialize reductions used in search
    init_reductions();

    signal(SIGINT, signalCallbackHandler);
    signal(SIGTERM, signalCallbackHandler);
#ifdef SIGBREAK
    signal(SIGBREAK, signalCallbackHandler);
#endif

    uciLoop(argc, argv);
}
#include "tt.h"

void store_entry(int depth, int bestvalue,
                 int old_alpha, int beta, U64 key,
                 uint16_t startAge, uint16_t move) 
{
    U64  index = key % TT_SIZE;                
    TEntry tte = TTable[index];

    Flag b = bestvalue <= old_alpha ? UPPERBOUND : bestvalue >= beta ? LOWERBOUND : EXACT;

    if (!(bestvalue >= 19000) && !(bestvalue <= -19000) &&
        (tte.key != key || b == EXACT || depth - 3 + 1 > tte.depth - 2)) 
    {
        tte.depth = depth;
        tte.score = bestvalue;
        tte.age = startAge;
        tte.key = key;
        tte.move = move;
        TTable[index] = tte;
    }
}

void probe_tt(TEntry &tte, bool &ttHit, U64 key, int depth)
{
    U64 index = key % TT_SIZE;
    tte = TTable[index];
    if (tte.key == key && tte.depth >= depth) {
        // use TT move
        ttHit = true;
    }
}
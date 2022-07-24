#include "tt.h"

void store_entry(int depth, Score bestvalue,
                 Score old_alpha, Score beta, U64 key,
                 uint16_t move) 
{
    U64  index = key % TT_SIZE;                
    TEntry tte = TTable[index];

    Flag b = bestvalue <= old_alpha ? UPPERBOUND : bestvalue >= beta ? LOWERBOUND : EXACT;

    if (bestvalue < 19000 && bestvalue > -19000
        && (tte.key != key || b == EXACT || depth > (tte.depth * 2) / 3)) 
    {
        tte.depth = depth;
        tte.score = bestvalue;
        tte.key = key;
        tte.move = move;
        tte.flag = b;
        TTable[index] = tte;
    }
}

void probe_tt(TEntry &tte, bool &ttHit, U64 key)
{
    U64 index = key % TT_SIZE;
    tte = TTable[index];
    if (tte.key == key) {
        // use TT move
        ttHit = true;
    }
}
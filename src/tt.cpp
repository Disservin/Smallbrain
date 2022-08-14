#include "tt.h"

void store_entry(int depth, Score bestvalue, Flag b, U64 key, uint16_t move)
{
    U64 index = tt_index(key);
    TEntry tte = TTable[index];
    if ((tte.key != key || b == EXACT || depth > (tte.depth * 2) / 3))
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
    U64 index = tt_index(key);
    tte = TTable[index];
    ttHit = (tte.key == key);
}

uint32_t tt_index(U64 key)
{
    // return key & (TT_SIZE-1);
    return ((uint32_t)key * (uint64_t)TT_SIZE) >> 32;
}
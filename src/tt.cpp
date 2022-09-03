#include "tt.h"

void storeEntry(int depth, Score bestvalue, Flag b, U64 key, uint16_t move)
{
    U64 index = ttIndex(key);
    TEntry tte = TTable[index];

    if (tte.key != key || move)
        tte.move = move;

    if (tte.key != key || b == EXACT || depth + 4 > tte.depth)
    {
        tte.depth = depth;
        tte.score = bestvalue;
        tte.key = key;
        tte.flag = b;
        TTable[index] = tte;
    }
}

void probeTT(TEntry &tte, bool &ttHit, U64 key)
{
    U64 index = ttIndex(key);
    tte = TTable[index];
    ttHit = (tte.key == key);
}

uint32_t ttIndex(U64 key)
{
    // return key & (TT_SIZE-1);
    return ((uint32_t)key * (uint64_t)TT_SIZE) >> 32;
}
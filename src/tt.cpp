#include "tt.h"

TranspositionTable::TranspositionTable()
{
    static constexpr uint64_t size = 16 * 1024 * 1024 / sizeof(TEntry);
    allocateTT(size);
}

void TranspositionTable::storeEntry(int depth, Score bestvalue, Flag b, U64 key, Move move)
{
    U64 index = ttIndex(key);
    TEntry *tte = &entries[index];

    if (tte->key != key || move)
        tte->move = move;

    if (tte->key != key || b == EXACTBOUND || depth + 4 > tte->depth)
    {
        tte->depth = depth;
        tte->score = bestvalue;
        tte->key = key;
        tte->flag = b;
    }
}

TEntry *TranspositionTable::probeTT(bool &ttHit, Move &ttmove, U64 key)
{
    U64 index = ttIndex(key);
    TEntry *tte = &entries[index];
    ttHit = (tte->key == key);
    ttmove = tte->move;
    return tte;
}

uint32_t TranspositionTable::ttIndex(U64 key)
{
    // return key & (TT_SIZE-1);
    return ((uint32_t)key * entries.size()) >> 32;
}

void TranspositionTable::allocateTT(uint64_t size)
{
    entries.resize(size, TEntry());
    std::fill(entries.begin(), entries.end(), TEntry());
}

void TranspositionTable::clearTT()
{
    std::fill(entries.begin(), entries.end(), TEntry());
}

void TranspositionTable::prefetchTT(uint64_t key)
{
    prefetch(&entries[ttIndex(key)]);
}
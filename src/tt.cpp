#include "tt.h"

TranspositionTable::TranspositionTable()
{
    static constexpr uint64_t size = 16 * 1024 * 1024 / sizeof(TEntry);
    allocateTT(size);
}

void TranspositionTable::storeEntry(int depth, Score bestvalue, Flag b, U64 key, Move move)
{
    TEntry *tte = &entries[index(key)];

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
    TEntry *tte = &entries[index(key)];
    ttHit = (tte->key == key);
    ttmove = tte->move;
    return tte;
}

uint32_t TranspositionTable::index(U64 key) const
{
    assert((((uint32_t)key * entries.size()) >> 32) < entries.size());

    return ((uint32_t)key * entries.size()) >> 32;
}

void TranspositionTable::allocateTT(uint64_t size)
{
    entries.resize(size, TEntry());
}

void TranspositionTable::clearTT()
{
    std::fill(entries.begin(), entries.end(), TEntry());
}

void TranspositionTable::prefetchTT(uint64_t key) const
{
    prefetch(&entries[index(key)]);
}

int TranspositionTable::hashfull() const
{
    size_t used = 0;
    for (size_t i = 0; i < 1000; i++)
    {
        used += entries[i].flag != NONEBOUND;
    }
    return used;
}

#include "tt.h"

TranspositionTable::TranspositionTable() { allocateMB(16); }

void TranspositionTable::store(int depth, Score bestvalue, Flag b, U64 key, Move move) {
    TEntry *tte = &entries_[index(key)];

    if (tte->key != key || move) tte->move = move;

    if (tte->key != key || b == EXACTBOUND || depth + 4 > tte->depth) {
        tte->depth = depth;
        tte->score = bestvalue;
        tte->key = key;
        tte->flag = b;
    }
}

const TEntry *TranspositionTable::probe(bool &tt_hit, Move &ttmove, U64 key) const {
    const TEntry *tte = &entries_[index(key)];
    tt_hit = (tte->key == key);
    ttmove = tte->move;
    return tte;
}

uint32_t TranspositionTable::index(U64 key) const {
#ifdef __SIZEOF_INT128__
    return (uint64_t)(((__uint128_t)key * (__uint128_t)entries_.size()) >> 64);
#else
    return key % entries_.size();
#endif
}

void TranspositionTable::allocate(U64 size) { entries_.resize(size, TEntry()); }

void TranspositionTable::allocateMB(U64 size_mb) {
    U64 sizeB = size_mb * static_cast<int>(1e6);
    sizeB = std::clamp(sizeB, U64(1), U64(MAXHASH_MiB * 1e6));
    U64 elements = sizeB / sizeof(TEntry);
    allocate(elements);
    std::cout << "hash set to " << sizeB / 1e6 << " MB" << std::endl;
}

void TranspositionTable::clear() { std::fill(entries_.begin(), entries_.end(), TEntry()); }

int TranspositionTable::hashfull() const {
    int used = 0;
    for (size_t i = 0; i < 1000; i++) {
        used += entries_[i].flag != NONEBOUND;
    }
    return used;
}

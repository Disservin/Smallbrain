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

TEntry *TranspositionTable::probe(bool &tt_hit, Move &ttmove, U64 key) {
    TEntry *tte = &entries_[index(key)];
    tt_hit = (tte->key == key);
    ttmove = tte->move;
    return tte;
}

uint32_t TranspositionTable::index(U64 key) const {
    assert((((uint32_t)key * entries_.size()) >> 32) < entries_.size());

    return ((uint32_t)key * entries_.size()) >> 32;
}

void TranspositionTable::allocate(uint64_t size) { entries_.resize(size, TEntry()); }

void TranspositionTable::allocateMB(uint64_t size_mb) {
    uint64_t sizeB = size_mb * 1e6;
    sizeB = std::clamp(sizeB, uint64_t(1), uint64_t(MAXHASH * 1e6));
    uint64_t elements = sizeB / sizeof(TEntry);
    allocate(elements);
    std::cout << "info string hash set to " << sizeB / 1e6 << " MB" << std::endl;
}

void TranspositionTable::clear() { std::fill(entries_.begin(), entries_.end(), TEntry()); }

int TranspositionTable::hashfull() const {
    size_t used = 0;
    for (size_t i = 0; i < 1000; i++) {
        used += entries_[i].flag != NONEBOUND;
    }
    return used;
}

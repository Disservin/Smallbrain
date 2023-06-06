#include "tt.h"
#include "helper.h"

TranspositionTable::TranspositionTable() {
    static constexpr uint64_t size = 16 * 1024 * 1024 / sizeof(TEntry);
    allocate(size);
}

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

TEntry *TranspositionTable::probe(bool &ttHit, Move &ttmove, U64 key) {
    TEntry *tte = &entries_[index(key)];
    ttHit = (tte->key == key);
    ttmove = tte->move;
    return tte;
}

uint32_t TranspositionTable::index(U64 key) const {
    assert((((uint32_t)key * entries_.size()) >> 32) < entries_.size());

    return ((uint32_t)key * entries_.size()) >> 32;
}

void TranspositionTable::allocate(uint64_t size) { entries_.resize(size, TEntry()); }

void TranspositionTable::allocateMB(uint64_t size_mb) {
    // value * 10^6 / 2^20
    uint64_t sizeMiB = static_cast<uint64_t>(size_mb) * 1000000 / 1048576;
    sizeMiB = std::clamp(sizeMiB, uint64_t(1), MAXHASH);
    uint64_t elements = (static_cast<uint64_t>(sizeMiB) * 1024 * 1024) / sizeof(TEntry);
    allocate(elements);
    std::cout << "info string hash set to " << sizeMiB << " MiB"
              << " " << elements << std::endl;
}

void TranspositionTable::clear() { std::fill(entries_.begin(), entries_.end(), TEntry()); }

void TranspositionTable::prefetch(uint64_t key) const { builtin::prefetch(&entries_[index(key)]); }

int TranspositionTable::hashfull() const {
    size_t used = 0;
    for (size_t i = 0; i < 1000; i++) {
        used += entries_[i].flag != NONEBOUND;
    }
    return used;
}

#include "tt.h"

TranspositionTable::TranspositionTable() { allocateMB(16); }

void TranspositionTable::store(int depth, Score bestvalue, Flag b, U64 key, Move move) {
    auto *tte = &entries_[index(key)];

    auto &replace = tte->entries[0];

    for (int i = 0; i < BUCKET_SIZE; ++i) {
        if (replace.depth > tte->entries[i].depth && tte->entries[i].key != key) {
            replace = tte->entries[i];
        }
    }

    if (replace.key != key || move) replace.move = move;

    if (replace.key != key || b == EXACTBOUND || depth + 4 > replace.depth) {
        replace.depth = depth;
        replace.score = bestvalue;
        replace.key = key;
        replace.flag = b;
    }
}

const TEntry *TranspositionTable::probe(bool &tt_hit, Move &ttmove, U64 key) const {
    const auto idx = index(key);
    const auto *tte = &entries_[idx].entries[0];

    ttmove = tte->move;

    for (int i = 0; i < BUCKET_SIZE; i++) {
        if (entries_[idx].entries[i].key == key) {
            tt_hit = true;
            ttmove = entries_[idx].entries[i].move;
            return &entries_[idx].entries[i];
        }
    }

    return tte;
}

uint32_t TranspositionTable::index(U64 key) const {
    assert((((uint32_t)key * entries_.size()) >> 32) < entries_.size());

    return ((uint32_t)key * entries_.size()) >> 32;
}

void TranspositionTable::allocate(U64 size) { entries_.resize(size, Bucket()); }

void TranspositionTable::allocateMB(U64 size_mb) {
    U64 sizeB = size_mb * static_cast<int>(1e6);
    sizeB = std::clamp(sizeB, U64(1), U64(MAXHASH_MiB * 1e6));
    U64 elements = sizeB / sizeof(Bucket);
    allocate(elements);
    std::cout << "info string hash set to " << sizeB / 1e6 << " MB" << std::endl;
}

void TranspositionTable::clear() { std::fill(entries_.begin(), entries_.end(), Bucket()); }

int TranspositionTable::hashfull() const {
    int used = 0;

    for (size_t i = 0; i < 1000; i++) {
        for (size_t j = 0; j < BUCKET_SIZE; j++) {
            if (entries_[i].entries[j].flag != NONEBOUND) used++;
        }
    }

    return used / BUCKET_SIZE;
}

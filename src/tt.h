#pragma once

#include "types.h"

#include <vector>

PACK(struct TEntry {
    U64 key = 0;
    Score score = 0;
    Move move = NO_MOVE;
    uint8_t depth = 0;
    Flag flag = NONEBOUND;
});

class TranspositionTable
{
  private:
    std::vector<TEntry> entries;

  public:
    TranspositionTable();

    /// @brief store an entry in the TT
    /// @param depth
    /// @param bestvalue
    /// @param b Type of bound
    /// @param key Position hash
    /// @param move
    void storeEntry(int depth, Score bestvalue, Flag b, U64 key, Move move);

    /// @brief probe the TT for an entry
    /// @param tte
    /// @param ttHit
    /// @param key Position hash
    TEntry *probeTT(bool &ttHit, Move &ttmove, U64 key);

    /// @brief calculates the TT index of key
    /// @param key
    /// @return
    uint32_t index(U64 key) const;

    /// @brief allocate Transposition Table and initialize entries
    void allocateTT(uint64_t size);

    /// @brief clear the TT
    void clearTT();

    void prefetchTT(uint64_t key) const;

    int hashfull() const;
};
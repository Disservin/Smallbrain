#pragma once
#include "types.h"

PACK(struct TEntry {
    U64 key;
    Score score;
    uint16_t move;
    uint8_t depth;
    Flag flag;
});

extern TEntry *TTable;
extern U64 TT_SIZE;

void storeEntry(int depth, Score bestvalue, Flag b, U64 key, uint16_t move);

void probeTT(TEntry &tte, bool &ttHit, U64 key);

uint32_t ttIndex(U64 key);
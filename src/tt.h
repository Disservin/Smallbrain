#pragma once
#include "types.h"

struct TEntry
{
    U64 key;
    Score score;
    uint16_t move;
    uint8_t depth;
    Flag flag;
} __attribute__((packed));

extern TEntry *TTable;
extern U64 TT_SIZE;

void store_entry(int depth, Score bestvalue, Flag b, U64 key, uint16_t move);

void probe_tt(TEntry &tte, bool &ttHit, U64 key);

uint32_t tt_index(U64 key);
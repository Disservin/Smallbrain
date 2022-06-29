#pragma once
#include "types.h"

struct TEntry {
	U64 key;
	int score;
	uint16_t move;
	uint16_t age;
	uint8_t depth;
	Flag flag;
}__attribute__((packed));

extern TEntry* TTable;
extern U64 TT_SIZE;

void store_entry(int depth, int bestvalue,
                 int old_alpha, int beta, U64 key,
                 uint16_t startAge, uint16_t move);

void probe_tt(TEntry &tte, bool &ttHit, U64 key, int depth);
#pragma once
#include "types.h"

struct TEntry {
	U64 key;
	int score;
	uint16_t move;
	uint16_t age;
	uint8_t depth;
	uint8_t flag;
}__attribute__((packed));

extern TEntry* TTable;
extern U64 TT_SIZE;

bool store_entry(U64 index, int depth, int bestvalue,
                 int old_alpha, int beta, U64 key,
                 uint16_t startAge, uint16_t move);
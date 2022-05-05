#pragma once
#include "types.h"

struct TEntry {
	U64 key = 0ULL;
	uint8_t depth = 0;
	Flag flag = NONEBOUND;
	int score = 0;
	uint16_t age = 0;
	Move move{};
};
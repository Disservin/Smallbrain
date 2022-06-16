#pragma once
#include "types.h"

struct TEntry {
	U64 key;
	uint8_t depth;
	uint8_t flag;
	uint16_t move;
	int score;
	uint16_t age;
};
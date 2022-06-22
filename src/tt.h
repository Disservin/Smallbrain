#pragma once
#include "types.h"

struct __attribute__((__packed__)) TEntry {
	U64 key;
	int score;
	uint16_t move;
	uint16_t age;
	uint8_t depth;
	uint8_t flag;
};
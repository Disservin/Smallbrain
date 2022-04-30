#pragma once
#include "Board.h"

class PerfTest {
public:
	Board board = Board();
	U64 allNodes = 0;
	PerfTest(Board brd);
	U64 perft(int depth, int max);
};
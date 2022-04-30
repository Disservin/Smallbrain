#include <chrono>
#include <stdlib.h>

#include "Board.h"
#include "main.h"

PerfTest::PerfTest(Board brd) {
	board = brd;
}

U64 PerfTest::perft(int depth, int max) {
	if (depth == 1) {
		Movelist ml = board.legalmoves();
		return ml.size;
	}
	U64 nodes = 0;
	Movelist ml = board.legalmoves();
	for (int i = 0; i < ml.size; i++) {
		board.makeMove(ml.list[i]);
		nodes += perft(depth - 1, depth);
		board.unmakeMove(ml.list[i]);
		if (depth == max) {
			std::cout << board.printMove(ml.list[i]) << " " << nodes<< std::endl;
			allNodes += nodes;
			nodes = 0;
		}
	}
	return nodes;
}

//int main() {
//	Board board = Board();
//	board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
//	board.print();
//	PerfTest pt = PerfTest(board);
//	int depth = 7;
//	auto t0 = std::chrono::high_resolution_clock::now();
//	pt.perft(depth, depth);
//	auto t1 = std::chrono::high_resolution_clock::now();
//	auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
//	std::cout << std::fixed <<pt.allNodes << " nodes " << pt.allNodes/((time_diff/1000)+0.1) << " nps " << std::endl;
//}
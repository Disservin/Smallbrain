#pragma once
#include <thread>
#include <atomic>

#include "board.h"
#include "search.h"

extern std::atomic<bool> stopped;
extern Search searcher_class;

class ThreadManager {
public:
	void begin(int depth, bool bench = false, int64_t time = 0);
	void stop();
	bool is_searching();
private:
	std::thread threads;
};
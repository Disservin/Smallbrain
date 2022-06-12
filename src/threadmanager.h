#pragma once
#include <thread>
#include <atomic>

#include "board.h"
#include "search.h"
#include "timemanager.h"

extern std::atomic<bool> stopped;
extern Search searcher_class;

class ThreadManager {
public:
	void begin(int depth, uint64_t nodes, Time time);
	void stop();
	bool is_searching();
private:
	std::thread threads;
};
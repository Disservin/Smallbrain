#include <thread>

#include "threadmanager.h"


void ThreadManager::begin(int depth, uint64_t nodes, Time time, int noThreads) {
	if (is_searching()) {
		stop();
	}
	stopped = false;
	threads = std::thread(&start_thinking, searcher_class, noThreads, depth, nodes, time);
}
void ThreadManager::stop() {
	stopped = true;
	if (threads.joinable()) {
		threads.join();
	}
}
bool ThreadManager::is_searching() {
	return threads.joinable();
}
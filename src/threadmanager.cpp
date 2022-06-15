#include <thread>

#include "threadmanager.h"


void ThreadManager::begin(int depth, uint64_t nodes, Time time) {
	if (is_searching()) {
		stop();
	}
	stopped = false;
	threads = std::thread(&Search::iterative_deepening, searcher_class, depth, nodes, time);
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
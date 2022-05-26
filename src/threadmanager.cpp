#include <thread>

#include "threadmanager.h"


void ThreadManager::begin(Board& board, int depth, bool bench, int64_t time) {
	if (is_searching()) {
		stop();
	}
	stopped = false;
	threads = std::thread(&Search::iterative_deepening, searcher_class, depth, bench, time);
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
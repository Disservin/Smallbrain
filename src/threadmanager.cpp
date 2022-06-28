#include <thread>

#include "threadmanager.h"


void ThreadManager::begin(Board board, int workers, int depth, uint64_t nodes, Time time) {
	if (is_searching()) {
		stop();
	}
	stopped = false;
	// threads = std::thread(&Search::start_thinking, searcher_class, board, workers, depth, nodes, time);
	searcher_class.start_thinking(board, workers, depth, nodes, time);
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
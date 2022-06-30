#pragma once

#include <sstream>
#include <map>
#include <atomic>
#include <thread>
#include <algorithm>

#include "board.h"
#include "search.h"
#include "perft.h"

int main(int argc, char** argv);

void signal_callback_handler(int signum);

std::vector<std::string> split_input(std::string fen);

Move convert_uci_to_Move(std::string input);

void stop_threads();

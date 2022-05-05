#pragma once

#include <sstream>
#include <map>
#include <atomic>
#include <thread>
#include <algorithm>

#include "board.h"
#include "search.h"

std::vector<std::string> split_input(std::string fen);

Move convert_uci_to_Move(std::string input, Board& board);

int main(int argc, char** argv);
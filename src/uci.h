#pragma once

#include <sstream>
#include <map>
#include <atomic>
#include <thread>
#include <algorithm>
#include <cstring>
#include <signal.h>
#include <math.h> 
#include <fstream>

#include "board.h"
#include "search.h"
#include "perft.h"

// 57344 MB = 2^32 * 14B / (1024 * 1024)
#define MAXHASH 57344

int main(int argc, char** argv);

void signal_callback_handler(int signum);

std::vector<std::string> split_input(std::string fen);

Move convert_uci_to_Move(std::string input);

void stop_threads();

void allocate_tt();

void reallocate_tt(U64 elements);

void quit();

int find_element(std::string param, std::vector<std::string> tokens);
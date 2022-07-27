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

#define TUNE_INT(x, min, max)                                        \
    do {                                                             \
        extern int x;                                                \
        options.add_int_tune_option(#x, "spin", x, min, max);        \
    } while (0);

#define TUNE_DOUBLE(x, min, max)                                     \
    do {                                                             \
        extern double x;                                             \
        options.add_double_tune_option(#x, "spin", x, min, max);     \
    } while (0);

int main(int argc, char** argv);

void signal_callback_handler(int signum);

std::vector<std::string> split_input(std::string fen);

void stop_threads();

void quit();

int find_element(std::string param, std::vector<std::string> tokens);
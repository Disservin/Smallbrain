#pragma once

#include <algorithm>
#include <atomic>
#include <cstring>
#include <fstream>
#include <map>
#include <math.h>
#include <signal.h>
#include <sstream>
#include <thread>

#include "board.h"
#include "perft.h"
#include "search.h"

#define TUNE_INT(x, min, max)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        extern int x;                                                                                                  \
        options.addIntTuneOption(#x, "spin", x, min, max);                                                             \
    } while (0);

#define TUNE_DOUBLE(x, min, max)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        extern double x;                                                                                               \
        options.addDoubleTuneOption(#x, "spin", x, min, max);                                                          \
    } while (0);

int main(int argc, char **argv);

void signalCallbackHandler(int signum);

std::vector<std::string> splitInput(std::string fen);

void stopThreads();

void quit();

int findElement(std::string param, std::vector<std::string> tokens);
#pragma once
#include <map>

#include "board.h"
#include "neuralnet.h"

extern NNUE nnue;

Score evaluation(Board &board);
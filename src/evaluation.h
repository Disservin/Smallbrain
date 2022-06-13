#pragma once
#include <map>

#include "board.h"
#include "psqt.h"
#include "neuralnet.h"

extern NNUE nnue;
extern bool useNNUE;

int evaluation(Board& board);

int HCE_Eval(Board& board);
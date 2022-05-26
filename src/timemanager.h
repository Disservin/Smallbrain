#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

struct Time{
    int64_t optimum = 0;
    int64_t maximum = 0;
};

Time optimumTime(int64_t avaiableTime, int inc, int ply, int mtg);
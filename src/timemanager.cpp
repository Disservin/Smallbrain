#include <algorithm>

#include "timemanager.h"

Time optimumTime(int64_t availableTime, int inc, int movestogo)
{
    Time time;
    int overhead = 10;

    int mtg = movestogo == 0 ? 50 : movestogo;

    int64_t total = std::max(int64_t(1), (availableTime + mtg * inc / 2 - mtg * overhead));

    if (movestogo == 0)
        time.optimum = (total / 20);
    else
        time.optimum = std::min(availableTime * 0.5, total * 0.9 / movestogo);

    if (time.optimum <= 0)
        time.optimum = static_cast<int64_t>(availableTime * 0.02);

    // dont use more than 50% of total time available
    time.maximum = static_cast<int64_t>(std::min(2.0 * time.optimum, 0.5 * availableTime));

    return time;
}
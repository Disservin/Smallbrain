#include "timemanager.h"

#include <algorithm>

Time optimumTime(int64_t availableTime, int inc, int ply, int movestogo)
{
    Time time;
    int overhead = 10;

    int mtg = movestogo == 0 ? 50 : movestogo;

    time.optimum = std::max(1ll, static_cast<int64_t>((availableTime + mtg * inc - mtg * overhead)));

    if (movestogo == 0)
        time.optimum = static_cast<int64_t>(time.optimum / 20);

    if (time.optimum <= 0)
        time.optimum = static_cast<int64_t>(availableTime * 0.02);

    // dont use more than 80% of total time available
    time.maximum = static_cast<int64_t>(std::min(2.0 * time.optimum, 0.5 * availableTime));

    return time;
}
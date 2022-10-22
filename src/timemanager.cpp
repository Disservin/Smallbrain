#include "timemanager.h"

#include <algorithm>

Time optimumTime(int64_t avaiableTime, int inc, int ply, int mtg)
{
    Time time;
    int overhead = 5;

    if (mtg == 0)
        mtg = 50;

    time.optimum = static_cast<int64_t>((avaiableTime + mtg * inc - mtg * overhead) / 20);

    if (time.optimum >= avaiableTime)
        time.optimum = static_cast<int64_t>(std::max(1.0, avaiableTime / 20.0));

    time.maximum = static_cast<int64_t>((time.optimum * 2));
    if (time.maximum >= avaiableTime)
    {
        time.maximum = time.optimum;
    }

    if (time.maximum == 0 || time.optimum == 0)
    {
        time.maximum = time.optimum = 1;
    }
    return time;
}
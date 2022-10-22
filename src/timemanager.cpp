#include "timemanager.h"

#include <algorithm>

Time optimumTime(int64_t avaiableTime, int inc, int ply, int mtg)
{
    Time time;
    int overhead;

    if (mtg == 0)
    {
        mtg = 50;
        overhead = std::max(10 * (50 - ply / 2), 0);
    }
    else
    {
        overhead = std::max(10 * mtg / 2, 0);
    }

    if (avaiableTime - overhead >= 100)
        avaiableTime -= overhead;

    time.optimum = static_cast<int64_t>((avaiableTime / (double)mtg));

    if (static_cast<int64_t>(avaiableTime / 100.0) >= static_cast<int64_t>(inc))
        time.optimum += inc / 2;

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
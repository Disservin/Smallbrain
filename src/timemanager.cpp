#include "timemanager.h"

Time optimumTime(int64_t avaiableTime, int inc, int ply, int mtg)
{
    Time time;
    int overhead;

    if (mtg == 0)
    {
        mtg = 20;
        overhead = std::max(10 * (mtg - ply / 2), 0);
    }
    else
    {
        overhead = std::max(10 * mtg / 2, 0);
    }

    if (avaiableTime - overhead >= 100)
        avaiableTime -= overhead;

    time.optimum = (int64_t)(avaiableTime / (double)mtg);
    time.optimum += inc / 2;
    if (time.optimum >= avaiableTime)
    {
        time.optimum = (int64_t)std::max(1.0, avaiableTime / 20.0);
    }

    time.maximum = (int64_t)(time.optimum * 1.05);
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
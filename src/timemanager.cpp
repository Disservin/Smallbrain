#include "timemanager.h"

Time optimumTime(int64_t avaiableTime, int inc, int ply, double mtg)
{
    Time time;

    mtg = mtg == 0 ? 50.0 : mtg;

    int overhead = avaiableTime < 200 ? (avaiableTime < 100 ? 0 : 5) : 10;
    avaiableTime -= overhead;

    time.optimum = (int64_t)(avaiableTime / mtg);
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
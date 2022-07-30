#include "timemanager.h"

Time optimumTime(int64_t avaiableTime, int inc, int ply, int mtg, int overhead) {
    Time time;
    time.optimum = avaiableTime / 20;
    
    if (mtg > 0)
        time.optimum = avaiableTime / mtg;
    else
        time.optimum = avaiableTime / 20 ;

    mtg = std::max(mtg, 1);
    time.optimum += inc / 2;
    if (time.optimum >= avaiableTime)
        time.optimum = std::clamp(time.optimum, (int64_t)1, avaiableTime / 20);

    time.maximum = time.optimum * 1.05f;
    if (time.maximum >= avaiableTime)
        time.maximum = time.optimum - overhead / 2 * mtg;
    if (time.maximum == 0|| time.optimum == 0)
        time.maximum = time.optimum = 1;
    return time;
}
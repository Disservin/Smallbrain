#include "timemanager.h"

Time optimumTime(int64_t avaiableTime, int inc, int ply, int mtg) {
    float scale = 0.1f/(log(ply+2));
    int64_t optimum = (avaiableTime + inc / 2) * scale;
    int64_t maximum = avaiableTime * 0.7f;
    if (optimum <= 0.02f * avaiableTime) {
        int denom = mtg > 0 ? mtg : 20;
        optimum = avaiableTime / denom;
        maximum = optimum;
    }
    optimum = std::clamp(optimum, 1LL, avaiableTime);
    maximum = std::clamp(maximum, 1LL, avaiableTime);
    
    Time time;
    time.optimum = optimum;
    time.maximum = maximum;
    return time;
}
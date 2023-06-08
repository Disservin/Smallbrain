#include <algorithm>

#include "timemanager.h"

Time optimumTime(int64_t available_time, int inc, int movestogo) {
    Time time;
    int overhead = 10;

    int mtg = movestogo == 0 ? 50 : movestogo;

    int64_t total = std::max(int64_t(1), (available_time + mtg * inc / 2 - mtg * overhead));

    if (movestogo == 0)
        time.optimum = (total / 20);
    else
        time.optimum = std::min(available_time * 0.5, total * 0.9 / movestogo);

    if (time.optimum <= 0) time.optimum = static_cast<int64_t>(available_time * 0.02);

    // dont use more than 50% of total time available
    time.maximum = static_cast<int64_t>(std::min(2.0 * time.optimum, 0.5 * available_time));

    return time;
}
#include "tt.h"

bool store_entry(U64 index, int depth, int bestvalue,
                 int old_alpha, int beta, U64 key,
                 uint16_t startAge, uint16_t move) {
    TEntry tte = TTable[index];
    if (!(bestvalue >= 19000) && !(bestvalue <= -19000) &&
        (tte.depth < depth || tte.age + 3 <= startAge)) {
        tte.flag = EXACT;
        // Upperbound
        if (bestvalue <= old_alpha) {
            tte.flag = UPPERBOUND;
        }
        // Lowerbound
        else if (bestvalue >= beta) {
            tte.flag = LOWERBOUND;
        }
        tte.depth = depth;
        tte.score = bestvalue;
        tte.age = startAge;
        tte.key = key;
        tte.move = move;
        TTable[index] = tte;
        return true;
    }
    return false;
}
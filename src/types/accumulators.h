#include "../types.h"

#include "../nnue.h"

struct Accumulators {
    Accumulators() { assert(alignof(accumulators) == 32); };

    int size() const {
        assert(index >= 0 && index < MAX_PLY);
        return index;
    }

    void clear() { index = 0; }

    void push() {
        assert(index + 1 < MAX_PLY);
        index++;
        accumulators[index] = accumulators[index - 1];
    }

    void pop() {
        assert(index > 0);
        index--;
    }

    nnue::accumulator &back() {
        assert(index >= 0 && index < MAX_PLY);
        return accumulators[index];
    }

   private:
    alignas(32) std::array<nnue::accumulator, MAX_PLY> accumulators = {};
    int index = 0;
};
#pragma once
#include "tests.h"

namespace tests {
inline void testAllDraw() {
    Board b;

    // KBvkb same colour
    b.applyFen("8/2k1b3/8/8/8/4B3/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isSquareAttacked(~b.side_to_move, b.kingSQ(b.side_to_move), b.all())),
           Result::DRAWN, "KBvkb same colour");

    // KBvkb different colour
    b.applyFen("8/2k1b3/8/8/8/5B2/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isSquareAttacked(~b.side_to_move, b.kingSQ(b.side_to_move), b.all())),
           Result::NONE, "KBvkb different colour");

    // Kvkb
    b.applyFen("8/2k1b3/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isSquareAttacked(~b.side_to_move, b.kingSQ(b.side_to_move), b.all())),
           Result::DRAWN, "Kvkb");

    // KBvk
    b.applyFen("8/2k1B3/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isSquareAttacked(~b.side_to_move, b.kingSQ(b.side_to_move), b.all())),
           Result::DRAWN, "KBvk");

    // KNvk
    b.applyFen("8/2k1N3/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isSquareAttacked(~b.side_to_move, b.kingSQ(b.side_to_move), b.all())),
           Result::DRAWN, "KNvk");

    // Kvkn
    b.applyFen("8/2k1n3/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isSquareAttacked(~b.side_to_move, b.kingSQ(b.side_to_move), b.all())),
           Result::DRAWN, "Kvkn");

    // Kvk
    b.applyFen("8/2k5/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isSquareAttacked(~b.side_to_move, b.kingSQ(b.side_to_move), b.all())),
           Result::DRAWN, "Kvk");
}
}  // namespace tests

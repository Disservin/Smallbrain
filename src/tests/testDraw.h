#pragma once
#include "tests.h"

namespace tests {
inline void testAllDraw() {
    Board b;

    // KBvkb same colour
    b.setFen("8/2k1b3/8/8/8/4B3/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isAttacked(~b.sideToMove(), b.kingSQ(b.sideToMove()), b.all())),
           Result::DRAWN, "KBvkb same colour");

    // KBvkb different colour
    b.setFen("8/2k1b3/8/8/8/5B2/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isAttacked(~b.sideToMove(), b.kingSQ(b.sideToMove()), b.all())),
           Result::NONE, "KBvkb different colour");

    // Kvkb
    b.setFen("8/2k1b3/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isAttacked(~b.sideToMove(), b.kingSQ(b.sideToMove()), b.all())),
           Result::DRAWN, "Kvkb");

    // KBvk
    b.setFen("8/2k1B3/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isAttacked(~b.sideToMove(), b.kingSQ(b.sideToMove()), b.all())),
           Result::DRAWN, "KBvk");

    // KNvk
    b.setFen("8/2k1N3/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isAttacked(~b.sideToMove(), b.kingSQ(b.sideToMove()), b.all())),
           Result::DRAWN, "KNvk");

    // Kvkn
    b.setFen("8/2k1n3/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isAttacked(~b.sideToMove(), b.kingSQ(b.sideToMove()), b.all())),
           Result::DRAWN, "Kvkn");

    // Kvk
    b.setFen("8/2k5/8/8/8/8/2K5/8 w - - 0 1");
    expect(b.isDrawn(b.isAttacked(~b.sideToMove(), b.kingSQ(b.sideToMove()), b.all())),
           Result::DRAWN, "Kvk");
}
}  // namespace tests

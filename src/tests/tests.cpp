#include "tests.h"
#include "testDraw.h"
#include "testFenRepetition.h"
#include "testMoveLegality.h"
#include "testZobristHash.h"

namespace Tests
{

bool testAll()
{
    testAllFenRepetitions();
    testAllZobristHash();
    testAllDraw();
    testAllMoveLegality();

    std::cout << "Tests run successfully" << std::endl;
    return true;
}

} // namespace Tests
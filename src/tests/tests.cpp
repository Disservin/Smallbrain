#include "tests.h"
#include "testDraw.h"
#include "testFenRepetition.h"
#include "testZobristHash.h"

namespace tests {

bool testall() {
    std::cout << "Running tests" << std::endl;
    std::cout << "Running testAllFenRepetitions" << std::endl;
    testAllFenRepetitions();
    std::cout << "Running testAllZobristHash" << std::endl;
    testAllZobristHash();
    std::cout << "Running testAllDraw" << std::endl;
    testAllDraw();

    std::cout << "Tests run successfully" << std::endl;
    return true;
}

}  // namespace tests
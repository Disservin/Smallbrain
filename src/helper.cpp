#include "helper.h"

std::vector<std::string> split_input(std::string fen)
{
    std::stringstream fen_stream(fen);
    std::string segment;
    std::vector<std::string> seglist;

    while (std::getline(fen_stream, segment, ' '))
    {
        seglist.push_back(segment);
    }
    return seglist;
}

// Compiler specific functions, taken from Stockfish https://github.com/official-stockfish/Stockfish
#if defined(__GNUC__)  // GCC, Clang, ICC

Square bsf(U64 b) {
    if (!b) return NO_SQ;
    return Square(__builtin_ctzll(b));
}

Square bsr(U64 b) {
    if (!b) return NO_SQ;
    return Square(63 ^ __builtin_clzll(b));
}

#elif defined(_MSC_VER)  // MSVC

#ifdef _WIN64  // MSVC, WIN64
#include <intrin.h>
Square bsf(U64 b) {

    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (Square)idx;
}

Square bsr(U64 b) {

    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return (Square)idx;
}

#else  // MSVC, WIN32
#include <intrin.h>
Square bsf(U64 b) {

    unsigned long idx;

    if (b & 0xffffffff) {
        _BitScanForward(&idx, int32_t(b));
        return Square(idx);
    }
    else {
        _BitScanForward(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    }
}

Square bsr(U64 b) {

    unsigned long idx;

    if (b >> 32) {
        _BitScanReverse(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    }
    else {
        _BitScanReverse(&idx, int32_t(b));
        return Square(idx);
    }
}

#endif

#else  // Compiler is neither GCC nor MSVC compatible

#error "Compiler not supported."

#endif

uint8_t popcount(U64 b) {
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)

    return (uint8_t)_mm_popcnt_u64(b);

#else // Assumed gcc or compatible compiler

    return __builtin_popcountll(b);

#endif
}

Square poplsb(U64& mask) {
    int8_t s = bsf(mask);
    mask &= mask - 1;
    // mask = _blsr_u64(mask);
    return Square(s);
}

uint8_t diagonal_of(Square sq) {
    return 7 + square_rank(sq) - square_file(sq);
}

uint8_t anti_diagonal_of(Square sq) {
    return square_rank(sq) + square_file(sq);
}

uint8_t manhatten_distance(Square sq1, Square sq2) {
    return std::abs(square_file(sq1) - square_file(sq2)) + std::abs(square_rank(sq1) - square_rank(sq2));
}

bool get_square_color(Square square) {
    if ((square % 8) % 2 == (square / 8) % 2) {
        return false;
    }
    else {
        return true;
    }
}

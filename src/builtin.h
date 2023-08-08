#pragma once

#include "types.h"

namespace builtin {
/// @brief Get the least significant bit of a U64.
/// @param b
/// @return
inline Square lsb(U64 b);

/// @brief Get the most significant bit of a U64.
/// @param b
/// @return
inline Square msb(U64 b);

#if defined(__GNUC__)  // GCC, Clang, ICC
inline Square lsb(U64 b) {
    assert(b);
    return Square(__builtin_ctzll(b));
}

inline Square msb(U64 b) {
    assert(b);
    return Square(63 ^ __builtin_clzll(b));
}

#elif defined(_MSC_VER)  // MSVC

#ifdef _WIN64  // MSVC, WIN64
#include <intrin.h>
inline Square lsb(U64 b) {
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (Square)idx;
}

inline Square msb(U64 b) {
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return (Square)idx;
}

#else  // MSVC, WIN32
#include <intrin.h>
inline Square lsb(U64 b) {
    unsigned long idx;

    if (b & 0xffffffff) {
        _BitScanForward(&idx, int32_t(b));
        return Square(idx);
    } else {
        _BitScanForward(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    }
}

inline Square msb(U64 b) {
    unsigned long idx;

    if (b >> 32) {
        _BitScanReverse(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    } else {
        _BitScanReverse(&idx, int32_t(b));
        return Square(idx);
    }
}

#endif

#else  // Compiler is neither GCC nor MSVC compatible

#error "Compiler not supported."

#endif

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
template <int rw = 0>
inline void prefetch(const void *addr) {
#if defined(__INTEL_COMPILER)
    __asm__("");
#endif
#ifdef __GNUC__
    _mm_prefetch((const char *)addr, _MM_HINT_T0);
#else
    _mm_prefetch((const char *)addr, 0);
#endif
}
#else
template <int rw = 0>
inline void prefetch(const void *addr) {
    __builtin_prefetch((const char *)addr, 0, rw);
}
#endif

/// @brief Get the least significant bit of a U64 and pop it.
/// @param mask
/// @return
inline Square poplsb(Bitboard &mask) {
    Square s = lsb(mask);
    mask &= mask - 1;  // compiler optimizes this to _blsr_u64
    return Square(s);
}

inline int popcount(Bitboard mask) {
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)

    return (uint8_t)_mm_popcnt_u64(mask);

#else  // Assumed gcc or compatible compiler

    return __builtin_popcountll(mask);

#endif
}

}  // namespace builtin
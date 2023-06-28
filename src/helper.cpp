#include <bitset>

#include "helper.h"

namespace builtin {
// Compiler specific functions, taken from Stockfish https://github.com/official-stockfish/Stockfish
#if defined(__GNUC__)  // GCC, Clang, ICC

Square lsb(Bitboard b) {
    assert(b);
    return Square(__builtin_ctzll(b));
}

Square msb(Bitboard b) {
    assert(b);
    return Square(63 ^ __builtin_clzll(b));
}

#elif defined(_MSC_VER)  // MSVC

#ifdef _WIN64  // MSVC, WIN64
#include <intrin.h>
Square lsb(Bitboard b) {
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (Square)idx;
}

Square msb(Bitboard b) {
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return (Square)idx;
}

#else  // MSVC, WIN32
#include <intrin.h>
Square lsb(Bitboard b) {
    unsigned long idx;

    if (b & 0xffffffff) {
        _BitScanForward(&idx, int32_t(b));
        return Square(idx);
    } else {
        _BitScanForward(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    }
}

Square msb(Bitboard b) {
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

int popcount(Bitboard mask) {
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)

    return (uint8_t)_mm_popcnt_u64(mask);

#else  // Assumed gcc or compatible compiler

    return __builtin_popcountll(mask);

#endif
}

Square poplsb(Bitboard &mask) {
    int8_t s = lsb(mask);
    mask &= mask - 1;  // compiler optimizes this to _blsr_u64
    return Square(s);
}

}  // namespace builtin

uint8_t squareDistance(Square a, Square b) {
    return std::max(std::abs(squareFile(a) - squareFile(b)),
                    std::abs(square_rank(a) - square_rank(b)));
}

uint8_t manhattenDistance(Square sq1, Square sq2) {
    return std::abs(squareFile(sq1) - squareFile(sq2)) +
           std::abs(square_rank(sq1) - square_rank(sq2));
}

bool getSquareColor(Square square) {
    if ((square % 8) % 2 == (square / 8) % 2) {
        return false;
    } else {
        return true;
    }
}

void mean_of(int v) {
    means[0]++;
    means[1] += v;
}

void max_of(int v) {
    max[0]++;
    if (v > max[1]) max[1] = v;
}

void min_of(int v) {
    min[0]++;
    if (v < min[1]) min[1] = v;
}

void print_mean() {
    if (means[0])
        std::cout << "Total " << means[0] << " Mean " << (double)means[1] / means[0] << std::endl;
    if (min[0]) std::cout << "Total " << min[0] << " Min " << min[1] << std::endl;
    if (max[0]) std::cout << "Total " << max[0] << " Max " << max[1] << std::endl;
}

std::string outputScore(int score) {
    if (std::abs(score) <= 4) score = 0;

    if (score >= VALUE_MATE_IN_PLY)
        return "mate " + std::to_string(((VALUE_MATE - score) / 2) + ((VALUE_MATE - score) & 1));
    else if (score <= VALUE_MATED_IN_PLY)
        return "mate " + std::to_string(-((VALUE_MATE + score) / 2) + ((VALUE_MATE + score) & 1));
    else
        return "cp " + std::to_string(score);
}

// clang-format off
void uciOutput(int score, int depth, uint8_t seldepth, U64 nodes, U64 tbHits, int time, std::string pv, int hashfull)
{
    std::stringstream ss;

    ss  << "info depth " << signed(depth) 
        << " seldepth "  << signed(seldepth) 
        << " score "     << outputScore(score)
        << " tbhits "    << tbHits 
        << " nodes "     << nodes 
        << " nps "       << signed((nodes / (time + 1)) * 1000)
        << " hashfull "  << hashfull
        << " time "      << time 
        << " pv"         << pv;

    std::cout << ss.str() << std::endl;
}
// clang-format on

Piece makePiece(PieceType type, Color c) {
    if (type == NONETYPE) return NONE;
    return Piece(type + 6 * c);
}

void printBitboard(Bitboard bb) {
    std::bitset<64> b(bb);
    std::string str_bitset = b.to_string();
    for (int i = 0; i < MAX_SQ; i += 8) {
        std::string x = str_bitset.substr(i, 8);
        reverse(x.begin(), x.end());
        std::cout << x << std::endl;
    }
    std::cout << '\n' << std::endl;
}

bool sameColor(int sq1, int sq2) { return ((9 * (sq1 ^ sq2)) & 8) == 0; }
#include "helper.h"

#include <bitset>

std::vector<std::string> splitInput(std::string fen)
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

uint8_t square_distance(Square a, Square b)
{
    return std::max(std::abs(square_file(a) - square_file(b)), std::abs(square_rank(a) - square_rank(b)));
}

// Compiler specific functions, taken from Stockfish https://github.com/official-stockfish/Stockfish
#if defined(__GNUC__) // GCC, Clang, ICC

Square lsb(U64 b)
{
    if (!b)
        return NO_SQ;
    return Square(__builtin_ctzll(b));
}

Square msb(U64 b)
{
    if (!b)
        return NO_SQ;
    return Square(63 ^ __builtin_clzll(b));
}

#elif defined(_MSC_VER) // MSVC

#ifdef _WIN64 // MSVC, WIN64
#include <intrin.h>
Square lsb(U64 b)
{
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (Square)idx;
}

Square msb(U64 b)
{
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return (Square)idx;
}

#else // MSVC, WIN32
#include <intrin.h>
Square lsb(U64 b)
{
    unsigned long idx;

    if (b & 0xffffffff)
    {
        _BitScanForward(&idx, int32_t(b));
        return Square(idx);
    }
    else
    {
        _BitScanForward(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    }
}

Square msb(U64 b)
{
    unsigned long idx;

    if (b >> 32)
    {
        _BitScanReverse(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    }
    else
    {
        _BitScanReverse(&idx, int32_t(b));
        return Square(idx);
    }
}

#endif

#else // Compiler is neither GCC nor MSVC compatible

#error "Compiler not supported."

#endif

uint8_t popcount(U64 mask)
{
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)

    return (uint8_t)_mm_popcnt_u64(mask);

#else // Assumed gcc or compatible compiler

    return __builtin_popcountll(mask);

#endif
}

Square poplsb(U64 &mask)
{
    int8_t s = lsb(mask);
    mask &= mask - 1; // compiler optimizes this to _blsr_u64
    return Square(s);
}

uint8_t manhatten_distance(Square sq1, Square sq2)
{
    return std::abs(square_file(sq1) - square_file(sq2)) + std::abs(square_rank(sq1) - square_rank(sq2));
}

bool get_square_color(Square square)
{
    if ((square % 8) % 2 == (square / 8) % 2)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void prefetch(const void *addr)
{
#if defined(__INTEL_COMPILER)
    __asm__("");
#endif

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
    _mm_prefetch((char *)addr, _MM_HINT_T0);
#else
    __builtin_prefetch(addr);
#endif
}

void mean_of(int v)
{
    means[0]++;
    means[1] += v;
}

void max_of(int v)
{
    max[0]++;
    if (v > max[1])
        max[1] = v;
}

void min_of(int v)
{
    min[0]++;
    if (v < min[1])
        min[1] = v;
}

void print_mean()
{
    if (means[0])
        std::cout << "Total " << means[0] << " Mean " << (double)means[1] / means[0] << std::endl;
    if (min[0])
        std::cout << "Total " << min[0] << " Min " << min[1] << std::endl;
    if (max[0])
        std::cout << "Total " << max[0] << " Max " << max[1] << std::endl;
}

std::string outputScore(int score)
{
    if (std::abs(score) <= 4)
        score = 0;

    if (score >= VALUE_MATE_IN_PLY)
        return "mate " + std::to_string(((VALUE_MATE - score) / 2) + ((VALUE_MATE - score) & 1));
    else if (score <= VALUE_MATED_IN_PLY)
        return "mate " + std::to_string(-((VALUE_MATE + score) / 2) + ((VALUE_MATE + score) & 1));
    else
        return "cp " + std::to_string(score);
}

// clang-format off
void uciOutput(int score, int depth, uint8_t seldepth, U64 nodes, U64 tbHits, int time, std::string pv)
{
    std::stringstream ss;

    ss  << "info depth " << signed(depth) 
        << " seldepth "  << signed(seldepth) 
        << " score "     << outputScore(score)
        << " tbhits "    << tbHits 
        << " nodes "     << nodes 
        << " nps "       << signed((nodes / (time + 1)) * 1000)
        << " time "      << time 
        << " pv"         << pv;

    std::cout << ss.str() << std::endl;
}
// clang-format on

Piece makePiece(PieceType type, Color c)
{
    if (type == NONETYPE)
        return None;
    return Piece(type + 6 * c);
}

void printBitboard(U64 bb)
{
    std::bitset<64> b(bb);
    std::string str_bitset = b.to_string();
    for (int i = 0; i < MAX_SQ; i += 8)
    {
        std::string x = str_bitset.substr(i, 8);
        reverse(x.begin(), x.end());
        std::cout << x << std::endl;
    }
    std::cout << '\n' << std::endl;
}

bool elementInVector(std::string el, std::vector<std::string> haystack)
{
    return std::find(haystack.begin(), haystack.end(), el) != haystack.end();
}

bool contains(std::string needle, std::string haystack)
{
    return haystack.find(needle) != std::string::npos;
}

bool contains(std::vector<std::string> needle, std::string haystack)
{
    for (auto &var : needle)
    {
        if (haystack.find(var) != std::string::npos)
            return true;
    }
    return false;
}
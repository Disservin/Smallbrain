#include "helper.h"

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

uint8_t square_file(Square sq)
{
    return sq & 7;
}

uint8_t square_rank(Square sq)
{
    return sq >> 3;
}

uint8_t square_distance(Square a, Square b)
{
    return std::max(std::abs(square_file(a) - square_file(b)), std::abs(square_rank(a) - square_rank(b)));
}

// Compiler specific functions, taken from Stockfish https://github.com/official-stockfish/Stockfish
#if defined(__GNUC__) // GCC, Clang, ICC

Square bsf(U64 b)
{
    if (!b)
        return NO_SQ;
    return Square(__builtin_ctzll(b));
}

Square bsr(U64 b)
{
    if (!b)
        return NO_SQ;
    return Square(63 ^ __builtin_clzll(b));
}

#elif defined(_MSC_VER) // MSVC

#ifdef _WIN64 // MSVC, WIN64
#include <intrin.h>
Square bsf(U64 b)
{
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (Square)idx;
}

Square bsr(U64 b)
{
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return (Square)idx;
}

#else // MSVC, WIN32
#include <intrin.h>
Square bsf(U64 b)
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

Square bsr(U64 b)
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
    int8_t s = bsf(mask);
    mask &= mask - 1;
    // mask = _blsr_u64(mask);
    return Square(s);
}

uint8_t diagonal_of(Square sq)
{
    return 7 + square_rank(sq) - square_file(sq);
}

uint8_t anti_diagonal_of(Square sq)
{
    return square_rank(sq) + square_file(sq);
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

void prefetch(void *addr)
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

std::string uciRep(Move move)
{
    std::string m = "";
    m += squareToString[from(move)];
    m += squareToString[to(move)];
    if (promoted(move))
        m += PieceTypeToPromPiece[piece(move)];
    return m;
}

void mean_of(int v)
{
    means[0]++;
    means[1] += v;
}

void print_mean()
{
    if (means[0])
        std::cout << "Total " << means[0] << " Mean " << (double)means[1] / means[0] << std::endl;
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

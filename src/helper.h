#pragma once
#include <algorithm>
#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits> // for is_same_v
#include <vector>

#include "types.h"

std::vector<std::string> splitInput(std::string fen);

/// @brief Gets the file index of the square where 0 is the a-file
/// @param sq
/// @return the file of the square
inline constexpr File square_file(Square sq)
{
    return File(sq & 7);
}

/// @brief Gets the rank index of the square where 0 is the first rank.
/// @param sq
/// @return the rank of the square
inline constexpr Rank square_rank(Square sq)
{
    return Rank(sq >> 3);
}

/// @brief makes a square out of rank and file
/// @param f
/// @param r
/// @return
inline constexpr Square file_rank_square(File f, Rank r)
{
    return Square((r << 3) + f);
}

/// @brief  distance between two squares
/// @param a
/// @param b
/// @return
uint8_t square_distance(Square a, Square b);

/// @brief least significant bit instruction
/// @param mask
/// @return the least significant bit as the Square
Square lsb(U64 mask);

/// @brief most significant bit instruction
/// @param mask
/// @return the most significant bit as the Square
Square msb(U64 mask);

/// @brief Counts the set bits
/// @param mask
/// @return the count
uint8_t popcount(U64 mask);

/// @brief remove the lsb and return it
/// @param mask
/// @return the lsb
Square poplsb(U64 &mask);

// returns diagonal of given square
inline constexpr uint8_t diagonal_of(Square sq)
{
    return 7 + square_rank(sq) - square_file(sq);
}

// returns anti diagonal of given square
inline constexpr uint8_t anti_diagonal_of(Square sq)
{
    return square_rank(sq) + square_file(sq);
}

uint8_t manhatten_distance(Square sq1, Square sq2);

/// @brief get the color of the square
/// @param square
/// @return light = true
bool get_square_color(Square square);

/// @brief get the piecetype of a piece
/// @param piece
/// @return the piecetype
inline PieceType type_of_piece(Piece piece)
{
    if (piece == None)
        return NONETYPE;
    return PieceToPieceType[piece];
}

/// @brief prefetches a memory address
/// @param addr
void prefetch(void *addr);

[[maybe_unused]] static std::atomic<int64_t> means[2];
[[maybe_unused]] static std::atomic<int64_t> min[2];
[[maybe_unused]] static std::atomic<int64_t> max[2];

[[maybe_unused]] void mean_of(int v);
[[maybe_unused]] void min_of(int v);
[[maybe_unused]] void max_of(int v);

void print_mean();

/// @brief adjust the outputted score
/// @param score
/// @param beta
/// @return a new score used for uci output
std::string outputScore(int score, Score beta);

/// @brief prints the new uci info
/// @param score
/// @param depth
/// @param seldepth
/// @param nodes
/// @param tbHits
/// @param time
/// @param pv
void uciOutput(int score, int depth, uint8_t seldepth, U64 nodes, U64 tbHits, int time, std::string pv);

/// @brief makes a Piece from only the piece type and color
/// @param type
/// @param c
/// @return new Piece
Piece makePiece(PieceType type, Color c);

/// @brief prints any bitboard
/// @param bb
void printBitboard(U64 bb);

/// @brief elementInVector searches needle in the tokens
/// @param needle
/// @param haystack
/// @return returns false if not found
bool elementInVector(std::string needle, std::vector<std::string> haystack);

/// @brief findElement returns the next value after a needle
/// @param needle
/// @param haystack
/// @return
template <typename T> T findElement(std::string needle, std::vector<std::string> haystack)
{
    int index = std::find(haystack.begin(), haystack.end(), needle) - haystack.begin();
    if constexpr (std::is_same_v<T, int>)
        return std::stoi(haystack[index + 1]);
    else
        return haystack[index + 1];
}

/// @brief
/// @param needle
/// @param haystack the string to search in
/// @return
bool contains(std::string needle, std::string haystack);

/// @brief
/// @param needle
/// @param haystack the vector to search in
/// @return
bool contains(std::vector<std::string> needle, std::string haystack);
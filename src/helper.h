#pragma once

#include <algorithm>
#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>  // for is_same_v
#include <vector>

#include "types.h"

std::vector<std::string> splitInput(const std::string &fen);

/// @brief Gets the file index of the square where 0 is the a-file
/// @param sq
/// @return the file of the square
inline constexpr File square_file(Square sq) { return File(sq & 7); }

/// @brief  distance between two squares
/// @param a
/// @param b
/// @return
uint8_t square_distance(Square a, Square b);

namespace builtin {
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
int popcount(U64 mask);

/// @brief remove the lsb and return it
/// @param mask
/// @return the lsb
Square poplsb(U64 &mask);

/// @brief prefetches a memory address
/// @param addr
void prefetch(const void *addr);
}  // namespace builtin

// returns diagonal of given square
inline constexpr uint8_t diagonal_of(Square sq) { return 7 + square_rank(sq) - square_file(sq); }

// returns anti diagonal of given square
inline constexpr uint8_t anti_diagonal_of(Square sq) { return square_rank(sq) + square_file(sq); }

uint8_t manhatten_distance(Square sq1, Square sq2);

/// @brief get the color of the square
/// @param square
/// @return light = true
bool get_square_color(Square square);

/// @brief get the piecetype of a piece
/// @param piece
/// @return the piecetype
inline constexpr PieceType type_of_piece(const Piece piece) { return PieceToPieceType[piece]; }

[[maybe_unused]] static std::atomic<int64_t> means[2];
[[maybe_unused]] static std::atomic<int64_t> min[2];
[[maybe_unused]] static std::atomic<int64_t> max[2];

[[maybe_unused]] void mean_of(int v);
[[maybe_unused]] void min_of(int v);
[[maybe_unused]] void max_of(int v);

void print_mean();

/// @brief adjust the outputted score
/// @param score
/// @return a new score used for uci output
std::string outputScore(int score);

/// @brief prints the new uci info
/// @param score
/// @param depth
/// @param seldepth
/// @param nodes
/// @param tbHits
/// @param time
/// @param pv
void uciOutput(int score, int depth, uint8_t seldepth, U64 nodes, U64 tbHits, int time,
               std::string pv, int hashfull);

/// @brief makes a Piece from only the piece type and color
/// @param type
/// @param c
/// @return new Piece
Piece makePiece(PieceType type, Color c);

/// @brief prints any bitboard
/// @param bb
void printBitboard(U64 bb);

/// @brief findElement returns the next value after a needle
/// @param needle
/// @param haystack
/// @return
template <typename T>
T findElement(std::string_view needle, const std::vector<std::string> &haystack) {
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
bool contains(std::string_view haystack, std::string_view needle);

/// @brief
/// @param needle
/// @param haystack the vector to search in
/// @return
bool contains(const std::vector<std::string> &haystack, std::string_view needle);

bool sameColor(int sq1, int sq2);

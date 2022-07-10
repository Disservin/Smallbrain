#pragma once
#include <vector>
#include <iostream>
#include <string>
#include <sstream>

#include "types.h"

std::vector<std::string> split_input(std::string fen);

//Gets the file index of the square where 0 is the a-file
inline uint8_t square_file(Square sq) {
    return sq & 7;
}

//Gets the rank index of the square where 0 is the first rank."""
inline uint8_t square_rank(Square sq) {
    return sq >> 3;
}

inline uint8_t square_distance(Square a, Square b) {
    return std::max(std::abs(square_file(a) - square_file(b)), std::abs(square_rank(a) - square_rank(b)));
}

Square bsf(U64 mask);
Square bsr(U64 mask);
uint8_t popcount(U64 mask);
Square poplsb(U64& mask);

// returns diagonal of given square
uint8_t diagonal_of(Square sq);

// returns anti diagonal of given square
uint8_t anti_diagonal_of(Square sq);

uint8_t manhatten_distance(Square sq1, Square sq2);

bool get_square_color(Square square);
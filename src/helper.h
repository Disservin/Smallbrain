#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "types.h"

std::vector<std::string> split_input(std::string fen);

// Gets the file index of the square where 0 is the a-file
uint8_t square_file(Square sq);

// Gets the rank index of the square where 0 is the first rank."""
uint8_t square_rank(Square sq);

uint8_t square_distance(Square a, Square b);

Square bsf(U64 mask);
Square bsr(U64 mask);
uint8_t popcount(U64 mask);
Square poplsb(U64 &mask);

// returns diagonal of given square
uint8_t diagonal_of(Square sq);

// returns anti diagonal of given square
uint8_t anti_diagonal_of(Square sq);

uint8_t manhatten_distance(Square sq1, Square sq2);

bool get_square_color(Square square);

inline PieceType type_of_piece(Piece piece)
{
    if (piece == None)
        return NONETYPE;
    return PieceToPieceType[piece];
}
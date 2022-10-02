#pragma once
#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "types.h"

std::vector<std::string> splitInput(std::string fen);

// Gets the file index of the square where 0 is the a-file
uint8_t square_file(Square sq);

// Gets the rank index of the square where 0 is the first rank."""
uint8_t square_rank(Square sq);

// distance between two squares
uint8_t square_distance(Square a, Square b);

// bitscanforward = least significant bit
Square bsf(U64 mask);

// bitscanreverse = most significant bit
Square bsr(U64 mask);

uint8_t popcount(U64 mask);
Square poplsb(U64 &mask);

// returns diagonal of given square
uint8_t diagonal_of(Square sq);

// returns anti diagonal of given square
uint8_t anti_diagonal_of(Square sq);

uint8_t manhatten_distance(Square sq1, Square sq2);

// ge the color of the square
bool get_square_color(Square square);

// convert piece in piecetype
inline PieceType type_of_piece(Piece piece)
{
    if (piece == None)
        return NONETYPE;
    return PieceToPieceType[piece];
}

void prefetch(void *addr);

// prints a move in uci format
std::string printMove(Move move);

static std::atomic<int64_t> means[2];

void mean_of(int v);

void print_mean();
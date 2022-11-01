#pragma once
#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "types.h"

std::vector<std::string> splitInput(std::string fen);

/// @brief Gets the file index of the square where 0 is the a-file
/// @param sq
/// @return the file of the square
inline constexpr uint8_t square_file(Square sq)
{
    return sq & 7;
}

/// @brief Gets the rank index of the square where 0 is the first rank.
/// @param sq
/// @return the rank of the square
inline constexpr uint8_t square_rank(Square sq)
{
    return sq >> 3;
}

/// @brief  distance between two squares
/// @param a
/// @param b
/// @return
uint8_t square_distance(Square a, Square b);

/// @brief least significant bit instruction
/// @param mask
/// @return the least significant bit as the Square
Square bsf(U64 mask);

/// @brief most significant bit instruction
/// @param mask
/// @return the most significant bit as the Square
Square bsr(U64 mask);

/// @brief Counts the set bits
/// @param mask
/// @return the count
uint8_t popcount(U64 mask);

/// @brief remove the bsf and return it
/// @param mask
/// @return the bsf
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

/// @brief get uci representation of a move
/// @param move
/// @return uci move
std::string uciRep(Move move);

static std::atomic<int64_t> means[2];

void mean_of(int v);

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
#pragma once

#include <algorithm>
#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>  // for is_same_v
#include <vector>

#include "types.h"

/// @brief Gets the file index of the square where 0 is the a-file
/// @param sq
/// @return the file of the square
[[nodiscard]] constexpr File squareFile(Square sq) { return File(sq & 7); }

/// @brief  distance between two squares
/// @param a
/// @param b
/// @return
[[nodiscard]] uint8_t squareDistance(Square a, Square b);

// returns diagonal of given square
[[nodiscard]] constexpr uint8_t diagonalOf(Square sq) {
    return 7 + squareRank(sq) - squareFile(sq);
}

// returns anti diagonal of given square
[[nodiscard]] constexpr uint8_t antiDiagonalOf(Square sq) {
    return squareRank(sq) + squareFile(sq);
}

[[nodiscard]] uint8_t manhattenDistance(Square sq1, Square sq2);

/// @brief get the color of the square
/// @param square
/// @return light = true
[[nodiscard]] bool getSquareColor(Square square);

/// @brief get the piecetype of a piece
/// @param piece
/// @return the piecetype
[[nodiscard]] constexpr PieceType typeOfPiece(Piece piece) { return PIECE_TO_PIECETYPE[piece]; }

[[maybe_unused]] static std::atomic<int64_t> means[2];
[[maybe_unused]] static std::atomic<int64_t> min[2];
[[maybe_unused]] static std::atomic<int64_t> max[2];

[[maybe_unused]] void meanOf(int v);
[[maybe_unused]] void minOf(int v);
[[maybe_unused]] void maxOf(int v);

void printMean();

/// @brief makes a Piece from only the piece type and color
/// @param type
/// @param c
/// @return new Piece
Piece makePiece(PieceType type, Color c);

/// @brief prints any bitboard
/// @param bb
void printBitboard(Bitboard bb);

[[nodiscard]] bool sameColor(int sq1, int sq2);

[[nodiscard]] Square rookCastleSquare(Square to_sq, Square from_sq);
[[nodiscard]] Square kingCastleSquare(Square to_sq, Square from_sq);

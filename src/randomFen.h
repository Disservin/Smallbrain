#pragma once

#include <array>
#include <iostream>
#include <random>

#include "rand.h"
#include "types.h"

class randomFenBoard {
   public:
    std::stringstream generateRandomFen();

   private:
    const int max_pieces[12] = {8, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1};

    const int randValues[13] = {0,    320,  330,  500,  900,    20000, -0,
                                -320, -330, -500, -900, -20000, 0};

    std::random_device rd;
    std::mt19937 e;

    std::array<Piece, 64> board{};
    std::array<int, 12> piece_count{};
    std::array<Bitboard, 12> bitboards{};

    Bitboard Pawns(Color color);
    Bitboard Knights(Color color);
    Bitboard Bishops(Color color);
    Bitboard Rooks(Color color);
    Bitboard Queens(Color color);
    Bitboard Kings(Color color);

    Bitboard all();

    bool isAttacked(int sq, Color c);
};

// get a random fen matching the current config
std::string getRandomfen();
#pragma once

#include <array>
#include <iostream>
#include <random>

#include "types.h"

class randomFenBoard
{
  public:
    std::string generateRandomFen();

  private:
    const char piece_to_char[13] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k', '-'};
    const int max_pieces[12] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    const int piece_values_random[13] = {100, 320, 330, 500, 900, 20000, -100, -320, -330, -500, -900, -20000, 0};

    std::random_device rd;
    std::mt19937 e;

    std::array<Piece, 64> board{};
    std::array<int, 12> piece_count{};
    std::array<U64, 12> bitboards{};

    U64 Pawns(Color color);
    U64 Knights(Color color);
    U64 Bishops(Color color);
    U64 Rooks(Color color);
    U64 Queens(Color color);
    U64 Kings(Color color);

    U64 All();

    bool isAttacked(int sq, Color c);
};

// get a random fen matching the current config
std::string getRandomfen();
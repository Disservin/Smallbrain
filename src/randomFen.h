#pragma once

#include <iostream>
#include <random>

#include "attacks.h"
#include "helper.h"
#include "sliders.hpp"
#include "types.h"

extern std::random_device rd;
extern std::mt19937 e;

inline constexpr char piece_to_char[] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k', '-'};
inline constexpr int max_pieces[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

inline constexpr int piece_values_random[] = {100, 320, 330, 500, 900, 20000, -100, -320, -330, -500, -900, -20000, 0};

class randomFenBoard
{
  public:
    std::string generateRandomFen();

  private:
    Piece board[64];
    int piece_count[12];
    U64 bitboards[12];

    U64 Pawns(Color color);
    U64 Knights(Color color);
    U64 Bishops(Color color);
    U64 Rooks(Color color);
    U64 Queens(Color color);
    U64 Kings(Color color);

    U64 All();

    bool isAttacked(int sq, Color c);

    void printBoard();
};

// get a random fen matching the current config
std::string getRandomfen();
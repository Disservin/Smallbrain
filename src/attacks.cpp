#include "attacks.h"

using namespace Chess_Lookup::Fancy;

uint64_t PawnAttacks(uint8_t sq, Color c)
{
    return PAWN_ATTACKS_TABLE[c][sq];
}

uint64_t KnightAttacks(uint8_t sq)
{
    return KNIGHT_ATTACKS_TABLE[sq];
}

uint64_t BishopAttacks(uint8_t sq, uint64_t occupied)
{
    return GetBishopAttacks(sq, occupied);
}

uint64_t RookAttacks(uint8_t sq, uint64_t occupied)
{
    return GetRookAttacks(sq, occupied);
}

uint64_t QueenAttacks(uint8_t sq, uint64_t occupied)
{
    return GetQueenAttacks(sq, occupied);
}

uint64_t KingAttacks(uint8_t sq)
{
    return KING_ATTACKS_TABLE[sq];
}
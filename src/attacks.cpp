#include "attacks.h"

using namespace Chess_Lookup::Fancy;

U64 PawnAttacks(uint8_t sq, Color c)
{
    return PAWN_ATTACKS_TABLE[c][sq];
}

U64 KnightAttacks(uint8_t sq)
{
    return KNIGHT_ATTACKS_TABLE[sq];
}

U64 BishopAttacks(uint8_t sq, U64 occupied)
{
    return GetBishopAttacks(sq, occupied);
}

U64 RookAttacks(uint8_t sq, U64 occupied)
{
    return GetRookAttacks(sq, occupied);
}

U64 QueenAttacks(uint8_t sq, U64 occupied)
{
    return GetQueenAttacks(sq, occupied);
}

U64 KingAttacks(uint8_t sq)
{
    return KING_ATTACKS_TABLE[sq];
}
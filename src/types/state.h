#pragma once

#include "../types.h"
#include "castling_rights.h"

struct State {
    U64 hash;
    CastlingRights castling;
    Square enpassant;
    uint8_t half_moves;
    Piece captured_piece;

    State(const U64 &hash, const CastlingRights &castling, const Square &enpassant,
          const uint8_t &half_moves, const Piece &captured_piece)
        : hash(hash),
          castling(castling),
          enpassant(enpassant),
          half_moves(half_moves),
          captured_piece(captured_piece) {}
};
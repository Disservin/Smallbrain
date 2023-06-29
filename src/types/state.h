#pragma once

#include "../types.h"
#include "castling_rights.h"

struct State {
    Square en_passant{};
    CastlingRights castling{};
    uint8_t half_move{};
    Piece captured_piece = NONE;

    State(Square en_passant, CastlingRights castling, uint8_t half_move, Piece captured_piece)
        : en_passant(en_passant),
          castling(castling),
          half_move(half_move),
          captured_piece(captured_piece) {}
};
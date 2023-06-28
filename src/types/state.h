#pragma once

#include "../types.h"
#include "castling_rights.h"

struct State {
    Square en_passant{};
    CastlingRights castling{};
    uint8_t half_move{};
    Piece captured_piece = NONE;
    State(Square enpassant_copy = {}, CastlingRights castling_rights_copy = {},
          uint8_t half_move_copy = {}, Piece captured_piece_copy = NONE)
        : en_passant(enpassant_copy),
          castling(castling_rights_copy),
          half_move(half_move_copy),
          captured_piece(captured_piece_copy) {}
};
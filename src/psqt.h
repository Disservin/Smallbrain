#pragma once
#include <map>

extern int piece_values[2][6];

extern int w_pawn_mg[64];
extern int b_pawn_mg[64];
extern int w_knight_mg[64];
extern int b_knight_mg[64];
extern int w_bishop_mg[64];
extern int b_bishop_mg[64];
extern int w_rook_mg[64];
extern int b_rook_mg[64];
extern int w_queen_mg[64];
extern int b_queen_mg[64];
extern int w_king_mg[64];
extern int b_king_mg[64];

extern int w_pawn_eg[64];
extern int b_pawn_eg[64];
extern int w_knight_eg[64];
extern int b_knight_eg[64];
extern int w_bishop_eg[64];
extern int b_bishop_eg[64];
extern int w_rook_eg[64];
extern int b_rook_eg[64];
extern int w_queen_eg[64];
extern int b_queen_eg[64];
extern int w_king_eg[64];
extern int b_king_eg[64];

extern std::map<int, int*> piece_to_mg;

extern std::map<int, int*> piece_to_eg;
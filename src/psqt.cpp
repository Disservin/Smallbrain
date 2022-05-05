#include <map>
#include "psqt.h"

int piece_values[2][6] = { { 126, 781, 825, 1276, 2538, 0}, { 208, 854, 915, 1380,  2682, 0} };

int w_pawn_mg[64]   = { 0, 0, 0, 0, 0, 0, 0, 0, -35, -1, -20, -23, -15, 24, 38, -22, -26, -4, -4, -10, 3, 3, 33, -12, -27, -2, -5, 12, 17, 6, 10, -25, -14, 13, 6, 21, 23, 12, 17, -23, -6, 7, 26, 31, 65, 56, 25, -20, 98, 134, 61, 95, 68, 126, 34, -11, 0, 0, 0, 0, 0, 0, 0, 0};
int b_pawn_mg[64]   = { 0, 0, 0, 0, 0, 0, 0, 0, 98, 134, 61, 95, 68, 126, 34, -11, -6, 7, 26, 31, 65, 56, 25, -20, -14, 13, 6, 21, 23, 12, 17, -23, -27, -2, -5, 12, 17, 6, 10, -25, -26, -4, -4, -10, 3, 3, 33, -12, -35, -1, -20, -23, -15, 24, 38, -22, 0, 0, 0, 0, 0, 0, 0, 0};
int w_knight_mg[64] = { -175, -92, -74, -73, -73, -74, -92, -175, -77, -41, -27, -15, -15, -27, -41, -77, -61, -17, 6, 12, 12, 6, -17, -61, -35, 8, 40, 49, 49, 40, 8, -35, -34, 13, 44, 51, 51, 44, 13, -34, -9, 22, 58, 53, 53, 58, 22, -9, -67, -27, 4, 37, 37, 4, -27, -67, -201, -83, -56, -26, -26, -56, -83, -201 };
int b_knight_mg[64] = { -201, -83, -56, -26, -26, -56, -83, -201, -67, -27, 4, 37, 37, 4, -27, -67, -9, 22, 58, 53, 53, 58, 22, -9, -34, 13, 44, 51, 51, 44, 13, -34, -35, 8, 40, 49, 49, 40, 8, -35, -61, -17, 6, 12, 12, 6, -17, -61, -77, -41, -27, -15, -15, -27, -41, -77, -175, -92, -74, -73, -73, -74, -92, -175 };
int w_bishop_mg[64] = { -37, -4, -6, -16, -16, -6, -4, -37, -11, 6, 13, 3, 3, 13, 6, -11, -5, 15, -4, 12, 12, -4, 15, -5, -4, 8, 18, 27, 27, 18, 8, -4, -8, 20, 15, 22, 22, 15, 20, -8, -11, 4, 1, 8, 8, 1, 4, -11, -12, -10, 4, 0, 0, 4, -10, -12, -34, 1, -10, -16, -16, -10, 1, -34 };
int b_bishop_mg[64] = { -34, 1, -10, -16, -16, -10, 1, -34, -12, -10, 4, 0, 0, 4, -10, -12, -11, 4, 1, 8, 8, 1, 4, -11, -8, 20, 15, 22, 22, 15, 20, -8, -4, 8, 18, 27, 27, 18, 8, -4, -5, 15, -4, 12, 12, -4, 15, -5, -11, 6, 13, 3, 3, 13, 6, -11, -37, -4, -6, -16, -16, -6, -4, -37 };
int w_rook_mg[64]   = { -31, -20, -14, -5, -5, -14, -20, -31, -21, -13, -8, 6, 6, -8, -13, -21, -25, -11, -1, 3, 3, -1, -11, -25, -13, -5, -4, -6, -6, -4, -5, -13, -27, -15, -4, 3, 3, -4, -15, -27, -22, -2, 6, 12, 12, 6, 2, -22, -2, 12, 16, 18, 18, 16, 12, -2, -17, -19, -1, 9, 9, -1, -19, -17 };
int b_rook_mg[64]   = { -17, -19, -1, 9, 9, -1, -19, -17, -2, 12, 16, 18, 18, 16, 12, -2, -22, -2, 6, 12, 12, 6, -2, -22, -27, -15, -4, 3, 3, -4, -15, -27, -13, -5, -4, -6, -6, -4, -5, -13, -25, -11, -1, 3, 3, -1, -11, -25, -21, -13, -8, 6, 6, -8, -13, -21, -31, -20, -14, -5, -5, -14, -20, -31 };
int w_queen_mg[64]  = { 3, -5, -5, 4, 4, -5, -5, 3, -3, 5, 8, 12, 12, 8, 5, -3, -3, 6, 13, 7, 7, 13, 6, -3, 4, 5, 9, 8, 8, 9, 5, 4, 0, 14, 12, 5, 5, 12, 14, 0, -4, 10, 6, 8, 8, 6, 10, -4, -5, 6, 10, 8, 8, 10, 6, -5, -2, -2, 1, -2, -2, 1, -2, -2 };
int b_queen_mg[64]  = { -2, -2, 1, -2, -2, 1, -2, -2, -5, 6, 10, 8, 8, 10, 6, -5, -4, 10, 6, 8, 8, 6, 10, -4, 0, 14, 12, 5, 5, 12, 14, 0, 4, 5, 9, 8, 8, 9, 5, 4, -3, 6, 13, 7, 7, 13, 6, -3, -3, 5, 8, 12, 12, 8, 5, -3, 3, -5, -5, 4, 4, -5, -5, 3 };
int w_king_mg[64]   = { 271, 327, 271, 198, 198, 271, 327, 271, 278, 303, 234, 179, 179, 234, 303, 278, 195, 258, 169, 120, 120, 169, 258, 195, 164, 190, 138, 98, 98, 138, 190, 164, 154, 179, 105, 70, 70, 105, 179, 154, 123, 145, 81, 31, 31, 81, 145, 123, 88, 120, 65, 33, 33, 65, 120, 88, 59, 89, 45, -1, -1, 45, 89, 59 };
int b_king_mg[64]   = { 59, 89, 45, -1, -1, 45, 89, 59, 88, 120, 65, 33, 33, 65, 120, 88, 123, 145, 81, 31, 31, 81, 145, 123, 154, 179, 105, 70, 70, 105, 179, 154, 164, 190, 138, 98, 98, 138, 190, 164, 195, 258, 169, 120, 120, 169, 258, 195, 278, 303, 234, 179, 179, 234, 303, 278, 271, 327, 271, 198, 198, 271, 327, 271 };

int w_pawn_eg[64]   = { 0, 0, 0, 0, 0, 0, 0, 0, 13, 8, 8, 10, 13, 0, 2, -7, 4, 7, -6, 1, 0, -5, -1, -8, 13, 9, -3, -7, -7, -8, 3, -1, 32, 24, 13, 5, -2, 4, 17, 17, 94, 100, 85, 67, 56, 53, 82, 84, 178, 173, 158, 134, 147, 132, 165, 187, 0, 0, 0, 0, 0, 0, 0, 0};
int b_pawn_eg[64]   = { 0, 0, 0, 0, 0, 0, 0, 0, 178, 173, 158, 134, 147, 132, 165, 187, 94, 100, 85, 67, 56, 53, 82, 84, 32, 24, 13, 5, -2, 4, 17, 17, 13, 9, -3, -7, -7, -8, 3, -1, 4, 7, -6, 1, 0, -5, -1, -8, 13, 8, 8, 10, 13, 0, 2, -7, 0, 0, 0, 0, 0, 0, 0, 0};
int w_knight_eg[64] = { -96, -65, -49, -21, -21, -49, -65, -96, -67, -54, -18, 8, 8, -18, -54, -67, -40, -27, -8, 29, 29, -8, -27, -40, -35, -2, 13, 28, 28, 13, -2, -35, -45, -16, 9, 39, 39, 9, -16, -45, -51, -44, -16, 17, 17, -16, -44, -51, -69, -50, -51, 12, 12, -51, -50, -69, -100, -88, -56, -17, -17, -56, -88, -100 };
int b_knight_eg[64] = { -100, -88, -56, -17, -17, -56, -88, -100, -69, -50, -51, 12, 12, -51, -50, -69, -51, -44, -16, 17, 17, -16, -44, -51, -45, -16, 9, 39, 39, 9, -16, -45, -35, -2, 13, 28, 28, 13, -2, -35, -40, -27, -8, 29, 29, -8, -27, -40, -67, -54, -18, 8, 8, -18, -54, -67, -96, -65, -49, -21, -21, -49, -65, -96 };
int w_bishop_eg[64] = { -40, -21, -26, -8, -8, -26, -21, -40, -26, -9, -12, 1, 1, -12, -9, -26, -11, -1, -1, 7, 7, -1, -1, -11, -14, -4, 0, 12, 12, 0, -4, -14, -12, -1, -10, 11, 11, -10, -1, -12, -21, 4, 3, 4, 4, 3, 4, -21, -22, -14, -1, 1, 1, -1, -14, -22, -32, -29, -26, -17, -17, -26, -29, -32 };
int b_bishop_eg[64] = { -32, -29, -26, -17, -17, -26, -29, -32, -22, -14, -1, 1, 1, -1, -14, -22, -21, 4, 3, 4, 4, 3, 4, -21, -12, -1, -10, 11, 11, -10, -1, -12, -14, -4, 0, 12, 12, 0, -4, -14, -11, -1, -1, 7, 7, -1, -1, -11, -26, -9, -12, 1, 1, -12, -9, -26, -40, -21, -26, -8, -8, -26, -21, -40 };
int w_rook_eg[64]   = { -9, -13, -10, -9, -9, -10, -13, -9, -12, -9, -1, -2, -2, -1, -9, -12, 6, -8, -2, -6, -6, -2, -8, 6, -6, 1, -9, 7, 7, -9, 1, -6, -5, 8, 7, -6, -6, 7, 8, -5, 6, 1, -7, 10, 10, -7, 1, 6, 4, 5, 20, -5, -5, 20, 5, 4, 18, 0, 19, 13, 13, 19, 0, 18 };
int b_rook_eg[64]   = { 18, 0, 19, 13, 13, 19, 0, 18, 4, 5, 20, -5, -5, 20, 5, 4, 6, 1, -7, 10, 10, -7, 1, 6, -5, 8, 7, -6, -6, 7, 8, -5, -6, 1, -9, 7, 7, -9, 1, -6, 6, -8, -2, -6, -6, -2, -8, 6, -12, -9, -1, -2, -2, -1, -9, -12, -9, -13, -10, -9, -9, -10, -13, -9 };
int w_queen_eg[64]  = { -69, -57, -47, -26, -26, -47, -57, -69, -54, -31, -22, -4, -4, -22, -31, -54, -39, -18, -9, 3, 3, -9, -18, -39, -23, -3, 13, 24, 24, 13, -3, -23, -29, -6, 9, 21, 21, 9, -6, -29, -38, -18, -11, 1, 1, -11, -18, -38, -50, -27, -24, -8, -8, -24, -27, -50, -74, -52, -43, -34, -34, -43, -52, -74 };
int b_queen_eg[64]  = { -74, -52, -43, -34, -34, -43, -52, -74, -50, -27, -24, -8, -8, -24, -27, -50, -38, -18, -11, 1, 1, -11, -18, -38, -29, -6, 9, 21, 21, 9, -6, -29, -23, -3, 13, 24, 24, 13, -3, -23, -39, -18, -9, 3, 3, -9, -18, -39, -54, -31, -22, -4, -4, -22, -31, -54, -69, -57, -47, -26, -26, -47, -57, -69 };
int w_king_eg[64]   = { 1, 45, 85, 76, 76, 85, 45, 1, 53, 100, 133, 135, 135, 133, 100, 53, 88, 130, 169, 175, 175, 169, 130, 88, 103, 156, 172, 172, 172, 172, 156, 103, 96, 166, 199, 199, 199, 199, 166, 96, 92, 172, 184, 191, 191, 184, 172, 92, 47, 121, 116, 131, 131, 116, 121, 47, 11, 59, 73, 78, 78, 73, 59, 11 };
int b_king_eg[64]   = { 11, 59, 73, 78, 78, 73, 59, 11, 47, 121, 116, 131, 131, 116, 121, 47, 92, 172, 184, 191, 191, 184, 172, 92, 96, 166, 199, 199, 199, 199, 166, 96, 103, 156, 172, 172, 172, 172, 156, 103, 88, 130, 169, 175, 175, 169, 130, 88, 53, 100, 133, 135, 135, 133, 100, 53, 1, 45, 85, 76, 76, 85, 45, 1 };

std::map<int, int*> piece_to_mg =
{
{ 0,   w_pawn_mg},
{ 1,   w_knight_mg},
{ 2,   w_bishop_mg},
{ 3,   w_rook_mg },
{ 4,   w_queen_mg },
{ 5,   w_king_mg },
{ 6,   b_pawn_mg},
{ 7,   b_knight_mg},
{ 8,   b_bishop_mg },
{ 9,   b_rook_mg },
{ 10,  b_queen_mg },
{ 11,  b_king_mg },
};

std::map<int, int*> piece_to_eg =
{
{ 0,   w_pawn_eg},
{ 1,   w_knight_eg},
{ 2,   w_bishop_eg},
{ 3,   w_rook_eg },
{ 4,   w_queen_eg },
{ 5,   w_king_eg },
{ 6,   b_pawn_eg},
{ 7,   b_knight_eg},
{ 8,   b_bishop_eg },
{ 9,   b_rook_eg },
{ 10,  b_queen_eg },
{ 11,  b_king_eg },
};
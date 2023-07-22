#pragma once

#include <cassert>
#include <chrono>
#include <iostream>
#include <sstream>
#include <unordered_map>

#define DEFAULT_POS std::string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

static constexpr int MAX_PLY = 120;
static constexpr int MAX_MOVES = 128;
static constexpr int MAX_SQ = 64;
static constexpr int N_PIECES = 12;

/********************
 * Type Definitions
 *******************/

using TimePoint = std::chrono::high_resolution_clock;

// desinction between an unsigned 64 bit integer and a bitboard

using U64 = uint64_t;
using Bitboard = uint64_t;

using Score = int16_t;

/********************
 * Enum Definitions
 *******************/

enum Color : uint8_t { WHITE, BLACK, NO_COLOR };

enum Piece : uint8_t {
    WHITEPAWN,
    WHITEKNIGHT,
    WHITEBISHOP,
    WHITEROOK,
    WHITEQUEEN,
    WHITEKING,
    BLACKPAWN,
    BLACKKNIGHT,
    BLACKBISHOP,
    BLACKROOK,
    BLACKQUEEN,
    BLACKKING,
    NONE
};

enum PieceType : uint8_t { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONETYPE };

// clang-format off
enum Square : uint8_t {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    NO_SQ
};

// clang-format on

enum {
    VALUE_MATE = 32000,
    VALUE_INFINITE = 32001,
    VALUE_NONE = 32002,

    VALUE_MATE_IN_PLY = VALUE_MATE - MAX_PLY,
    VALUE_MATED_IN_PLY = -VALUE_MATE_IN_PLY,

    VALUE_TB_WIN = VALUE_MATE_IN_PLY,
    VALUE_TB_LOSS = -VALUE_TB_WIN,
    VALUE_TB_WIN_IN_MAX_PLY = VALUE_TB_WIN - MAX_PLY,
    VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY
};

enum Flag : uint8_t {
    NONEBOUND,
    UPPERBOUND,
    LOWERBOUND,
    EXACTBOUND,
};

enum CastlingRight : uint8_t { WK = 1, WQ = 2, BK = 4, BQ = 8 };

enum Direction : int8_t {
    NORTH = 8,
    WEST = -1,
    SOUTH = -8,
    EAST = 1,
    NORTH_EAST = 9,
    NORTH_WEST = 7,
    SOUTH_WEST = -9,
    SOUTH_EAST = -7
};

enum Node { NONPV, PV, ROOT };

enum Rank { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };

enum File { NO_FILE = -1, FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };

enum class Movetype : uint8_t { ALL, CAPTURE, QUIET };

enum class Result { NONE, DRAWN, LOST };

/********************
 * Overloading of operators
 *******************/

constexpr Color operator~(Color C) { return Color(C ^ BLACK); }

#define INCR_OP_ON(T)                                                                            \
    constexpr inline T &operator++(T &p) { return p = static_cast<T>(static_cast<int>(p) + 1); } \
    constexpr inline T operator++(T &p, int) {                                                   \
        auto old = p;                                                                            \
        ++p;                                                                                     \
        return old;                                                                              \
    }

INCR_OP_ON(Piece)
INCR_OP_ON(Square)
INCR_OP_ON(PieceType)
INCR_OP_ON(File)
INCR_OP_ON(Rank)

#undef INCR_OP_ON

#define BASE_OP_ON(N, T)                                           \
    constexpr N operator+(N s, T d) { return N(int(s) + int(d)); } \
    constexpr N operator-(N s, T d) { return N(int(s) - int(d)); } \
    constexpr N &operator+=(N &s, T d) { return s = s + d; }       \
    constexpr N &operator-=(N &s, T d) { return s = s - d; }

BASE_OP_ON(Square, Direction)
BASE_OP_ON(Rank, File)

#undef BASE_OP_ON

/********************
 * Constant definitions
 *******************/

// clang-format off
const std::string SQUARE_TO_STRING[64] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
};

// clang-format on

static constexpr PieceType PIECE_TO_PIECETYPE[13] = {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONETYPE};

// file masks
static constexpr Bitboard MASK_FILE[8] = {
    0x101010101010101,  0x202020202020202,  0x404040404040404,  0x808080808080808,
    0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080,
};

// rank masks
static constexpr Bitboard MASK_RANK[8] = {
    0xff,         0xff00,         0xff0000,         0xff000000,
    0xff00000000, 0xff0000000000, 0xff000000000000, 0xff00000000000000};

static constexpr Bitboard DEFAULT_CHECKMASK = 18446744073709551615ULL;

static constexpr Piece FLIPPED_PIECE[] = {
    BLACKPAWN, BLACKKNIGHT, BLACKBISHOP, BLACKROOK, BLACKQUEEN, BLACKKING,
    WHITEPAWN, WHITEKNIGHT, WHITEBISHOP, WHITEROOK, WHITEQUEEN, WHITEKING,
};

/********************
 * Various other definitions
 *******************/

struct Time {
    int64_t optimum = 0;
    int64_t maximum = 0;
};

struct Limits {
    Time time;
    U64 nodes = 0;
    int depth = MAX_PLY - 1;
    bool infinite = false;
};

/********************
 * Maps
 *******************/

static std::unordered_map<Piece, char> PIECE_TO_CHAR({{WHITEPAWN, 'P'},
                                                      {WHITEKNIGHT, 'N'},
                                                      {WHITEBISHOP, 'B'},
                                                      {WHITEROOK, 'R'},
                                                      {WHITEQUEEN, 'Q'},
                                                      {WHITEKING, 'K'},
                                                      {BLACKPAWN, 'p'},
                                                      {BLACKKNIGHT, 'n'},
                                                      {BLACKBISHOP, 'b'},
                                                      {BLACKROOK, 'r'},
                                                      {BLACKQUEEN, 'q'},
                                                      {BLACKKING, 'k'},
                                                      {NONE, '.'}});

static std::unordered_map<char, Piece> CHAR_TO_PIECE({{'P', WHITEPAWN},
                                                      {'N', WHITEKNIGHT},
                                                      {'B', WHITEBISHOP},
                                                      {'R', WHITEROOK},
                                                      {'Q', WHITEQUEEN},
                                                      {'K', WHITEKING},
                                                      {'p', BLACKPAWN},
                                                      {'n', BLACKKNIGHT},
                                                      {'b', BLACKBISHOP},
                                                      {'r', BLACKROOK},
                                                      {'q', BLACKQUEEN},
                                                      {'k', BLACKKING},
                                                      {'.', NONE}});

static std::unordered_map<PieceType, char> PIECETYPE_TO_CHAR(
    {{KNIGHT, 'n'}, {BISHOP, 'b'}, {ROOK, 'r'}, {QUEEN, 'q'}});

static std::unordered_map<char, PieceType> CHAR_TO_PIECETYPE({{'n', KNIGHT},
                                                              {'b', BISHOP},
                                                              {'r', ROOK},
                                                              {'q', QUEEN},
                                                              {'N', KNIGHT},
                                                              {'B', BISHOP},
                                                              {'R', ROOK},
                                                              {'Q', QUEEN}});

/// @brief Gets the rank index of the square where 0 is the first rank.
/// @param sq
/// @return the rank of the square
constexpr Rank squareRank(Square sq) { return Rank(sq >> 3); }

/// @brief makes a square out of rank and file
/// @param f
/// @param r
/// @return
constexpr Square fileRankSquare(File f, Rank r) { return Square((r << 3) + f); }
/********************
 * Packed structures
 *******************/

#ifdef __GNUC__
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#endif

/********************
 * Move Logic
 *******************/

enum Move : uint16_t {
    NO_MOVE = 0,
    NULL_MOVE = 65,
    PROMOTION = 1 << 14,
    ENPASSANT = 2 << 14,
    CASTLING = 3 << 14
};

template <uint16_t MoveType = 0>
[[nodiscard]] constexpr Move make(Square source, Square target, PieceType pt = PieceType::KNIGHT) {
    return Move(MoveType +
                ((static_cast<uint16_t>(pt) - static_cast<uint16_t>(PieceType::KNIGHT)) << 12) +
                static_cast<uint16_t>(source << 6) + static_cast<uint16_t>(target));
}

[[nodiscard]] constexpr Square from(Move move) { return static_cast<Square>((move >> 6) & 0x3F); }

[[nodiscard]] constexpr Square to(Move move) { return static_cast<Square>(move & 0x3F); }

[[nodiscard]] constexpr uint16_t typeOf(Move move) {
    return static_cast<uint16_t>(move & (3 << 14));
}

[[nodiscard]] constexpr PieceType promotionType(Move move) {
    return static_cast<PieceType>(((move >> 12) & 3) + static_cast<int>(PieceType::KNIGHT));
}

inline std::ostream &operator<<(std::ostream &os, const Move move) {
    Square from_sq = from(move);
    Square to_sq = to(move);

    os << SQUARE_TO_STRING[from_sq] << SQUARE_TO_STRING[to_sq];
    if (typeOf(move) == Move::PROMOTION) {
        os << PIECETYPE_TO_CHAR.at(promotionType(move));
    }
    return os;
}

static constexpr int PIECE_VALUES_TUNED[2][7] = {{98, 337, 365, 477, 1025, 0, 0},
                                                 {114, 281, 297, 512, 936, 0, 0}};
static constexpr int PIECE_VALUES_CLASSICAL[7] = {100, 320, 330, 500, 900, 0, 0};

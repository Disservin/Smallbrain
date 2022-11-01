#pragma once
#include <chrono>
#include <iostream>
#include <unordered_map>

#define U64 uint64_t
#define Score int16_t
#define TimePoint std::chrono::high_resolution_clock
#define MAX_SQ 64
#define DEFAULT_POS std::string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

static constexpr int MAX_PLY = 120;
static constexpr int MAX_MOVES = 128;

/********************
 * Enum Definitions
 *******************/

enum class Movetype : uint8_t
{
    ALL,
    CAPTURE,
    QUIET,
    CHECK
};

enum Color : uint8_t
{
    White,
    Black,
    NO_COLOR
};

enum Phase : int
{
    MG,
    EG
};

enum Piece : uint8_t
{
    WhitePawn,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,
    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing,
    None
};

enum PieceType : uint8_t
{
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    NONETYPE
};

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

enum Value : int16_t
{
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

enum Flag : uint8_t
{
    UPPERBOUND,
    LOWERBOUND,
    EXACT,
    NONEBOUND
};

enum CastlingRight : uint8_t
{
    wk = 1,
    wq = 2,
    bk = 4,
    bq = 8
};

enum Direction : int8_t
{
    NORTH = 8,
    WEST = -1,
    SOUTH = -8,
    EAST = 1,
    NORTH_EAST = 9,
    NORTH_WEST = 7,
    SOUTH_WEST = -9,
    SOUTH_EAST = -7
};

enum MoveScores : int
{
    TT_MOVE_SCORE = 10'000'000,
    PROMOTION_SCORE = 9'000'000,
    CAPTURE_SCORE = 7'000'000,
    KILLER_ONE_SCORE = 6'000'000,
    KILLER_TWO_SCORE = 5'000'000
};

enum Staging
{
    TT_MOVE,
    EVAL_OTHER,
    OTHER
};

enum Node
{
    NonPV,
    PV,
    Root
};

/********************
 * Overloading of operators
 *******************/

constexpr Color operator~(Color C)
{
    return Color(C ^ Black);
}

#define INCR_OP_ON(T)                                                                                                  \
    constexpr inline T &operator++(T &p)                                                                               \
    {                                                                                                                  \
        return p = static_cast<T>(static_cast<int>(p) + 1);                                                            \
    }                                                                                                                  \
    constexpr inline T operator++(T &p, int)                                                                           \
    {                                                                                                                  \
        auto old = p;                                                                                                  \
        ++p;                                                                                                           \
        return old;                                                                                                    \
    }

INCR_OP_ON(Piece)
INCR_OP_ON(Square)
INCR_OP_ON(PieceType)
INCR_OP_ON(Staging)

#undef INCR_OP_ON

#define BASE_OP_ON(T)                                                                                                  \
    inline constexpr Square operator+(Square s, T d)                                                                   \
    {                                                                                                                  \
        return Square(int(s) + int(d));                                                                                \
    }                                                                                                                  \
    inline constexpr Square operator-(Square s, T d)                                                                   \
    {                                                                                                                  \
        return Square(int(s) - int(d));                                                                                \
    }                                                                                                                  \
    inline constexpr Square &operator+=(Square &s, T d)                                                                \
    {                                                                                                                  \
        return s = s + d;                                                                                              \
    }                                                                                                                  \
    inline constexpr Square &operator-=(Square &s, T d)                                                                \
    {                                                                                                                  \
        return s = s - d;                                                                                              \
    }

BASE_OP_ON(Direction)

#undef BASE_OP_ON

/********************
 * Constant definitions
 *******************/

// clang-format off
const std::string squareToString[64] = {
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

static constexpr PieceType PieceToPieceType[12] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
                                                   PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};

// file masks
static constexpr U64 MASK_FILE[8] = {
    0x101010101010101,  0x202020202020202,  0x404040404040404,  0x808080808080808,
    0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080,
};

// rank masks
static constexpr U64 MASK_RANK[8] = {0xff,         0xff00,         0xff0000,         0xff000000,
                                     0xff00000000, 0xff0000000000, 0xff000000000000, 0xff00000000000000};

// diagonal masks
static constexpr U64 MASK_DIAGONAL[15] = {
    0x80,
    0x8040,
    0x804020,
    0x80402010,
    0x8040201008,
    0x804020100804,
    0x80402010080402,
    0x8040201008040201,
    0x4020100804020100,
    0x2010080402010000,
    0x1008040201000000,
    0x804020100000000,
    0x402010000000000,
    0x201000000000000,
    0x100000000000000,
};

// anti-diagonal masks
static constexpr U64 MASK_ANTI_DIAGONAL[15] = {0x1,
                                               0x102,
                                               0x10204,
                                               0x1020408,
                                               0x102040810,
                                               0x10204081020,
                                               0x1020408102040,
                                               0x102040810204080,
                                               0x204081020408000,
                                               0x408102040800000,
                                               0x810204080000000,
                                               0x1020408000000000,
                                               0x2040800000000000,
                                               0x4080000000000000,
                                               0x8000000000000000};

static constexpr U64 WK_CASTLE_MASK = (1ULL << SQ_F1) | (1ULL << SQ_G1);
static constexpr U64 WQ_CASTLE_MASK = (1ULL << SQ_D1) | (1ULL << SQ_C1) | (1ULL << SQ_B1);

static constexpr U64 BK_CASTLE_MASK = (1ULL << SQ_F8) | (1ULL << SQ_G8);
static constexpr U64 BQ_CASTLE_MASK = (1ULL << SQ_D8) | (1ULL << SQ_C8) | (1ULL << SQ_B8);

static constexpr U64 DEFAULT_CHECKMASK = 18446744073709551615ULL;

/********************
 * Various other definitions
 *******************/

struct Time
{
    int64_t optimum = 0;
    int64_t maximum = 0;
};

struct Limits
{
    Time time;
    U64 nodes = 0;
    int depth = MAX_PLY;
};

inline Score mate_in(int ply)
{
    return (VALUE_MATE - ply);
}

inline Score mated_in(int ply)
{
    return (ply - VALUE_MATE);
}

inline Score scoreToTT(Score s, int plies)
{
    return (s >= VALUE_TB_WIN_IN_MAX_PLY ? s + plies : s <= VALUE_TB_LOSS_IN_MAX_PLY ? s - plies : s);
}

inline Score scoreFromTT(Score s, int plies)
{
    return (s >= VALUE_TB_WIN_IN_MAX_PLY ? s - plies : s <= VALUE_TB_LOSS_IN_MAX_PLY ? s + plies : s);
}

/********************
 * Maps
 *******************/

static std::unordered_map<Piece, char> pieceToChar({{WhitePawn, 'P'},
                                                    {WhiteKnight, 'N'},
                                                    {WhiteBishop, 'B'},
                                                    {WhiteRook, 'R'},
                                                    {WhiteQueen, 'Q'},
                                                    {WhiteKing, 'K'},
                                                    {BlackPawn, 'p'},
                                                    {BlackKnight, 'n'},
                                                    {BlackBishop, 'b'},
                                                    {BlackRook, 'r'},
                                                    {BlackQueen, 'q'},
                                                    {BlackKing, 'k'},
                                                    {None, '.'}});

static std::unordered_map<char, Piece> charToPiece({{'P', WhitePawn},
                                                    {'N', WhiteKnight},
                                                    {'B', WhiteBishop},
                                                    {'R', WhiteRook},
                                                    {'Q', WhiteQueen},
                                                    {'K', WhiteKing},
                                                    {'p', BlackPawn},
                                                    {'n', BlackKnight},
                                                    {'b', BlackBishop},
                                                    {'r', BlackRook},
                                                    {'q', BlackQueen},
                                                    {'k', BlackKing},
                                                    {'.', None}});

static std::unordered_map<PieceType, char> PieceTypeToPromPiece(
    {{KNIGHT, 'n'}, {BISHOP, 'b'}, {ROOK, 'r'}, {QUEEN, 'q'}});

static std::unordered_map<char, PieceType> pieceToInt(
    {{'n', KNIGHT}, {'b', BISHOP}, {'r', ROOK}, {'q', QUEEN}, {'N', KNIGHT}, {'B', BISHOP}, {'R', ROOK}, {'Q', QUEEN}});

static std::unordered_map<Square, CastlingRight> castlingMapRook({{SQ_A1, wq}, {SQ_H1, wk}, {SQ_A8, bq}, {SQ_H8, bk}});

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

enum Move : uint16_t
{
    NO_MOVE = 0,
    NULL_MOVE = 65
};

constexpr inline Square from(Move move)
{
    return Square(move & 0b111111);
}

constexpr inline Square to(Move move)
{
    return Square((move & 0b111111000000) >> 6);
}

constexpr inline PieceType piece(Move move)
{
    return PieceType((move & 0b111000000000000) >> 12);
}

constexpr inline bool promoted(Move move)
{
    return bool((move & 0b1000000000000000) >> 15);
}

constexpr inline Move make(PieceType piece = NONETYPE, Square source = NO_SQ, Square target = NO_SQ,
                           bool promoted = false)
{
    return Move((uint16_t)source | (uint16_t)target << 6 | (uint16_t)piece << 12 | (uint16_t)promoted << 15);
}

template <PieceType piece, bool promoted> Move make(Square source = NO_SQ, Square target = NO_SQ)
{
    return Move((uint16_t)source | (uint16_t)target << 6 | (uint16_t)piece << 12 | (uint16_t)promoted << 15);
}
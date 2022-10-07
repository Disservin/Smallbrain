#include "randomFen.h"

#include <cstring> //memset

U64 randomFenBoard::Pawns(Color color)
{
    return bitboards[color == Black ? BlackPawn : WhitePawn];
}

U64 randomFenBoard::Knights(Color color)
{
    return bitboards[color == Black ? BlackKnight : WhiteKnight];
}

U64 randomFenBoard::Bishops(Color color)
{
    return bitboards[color == Black ? BlackBishop : WhiteBishop];
}

U64 randomFenBoard::Rooks(Color color)
{
    return bitboards[color == Black ? BlackRook : WhiteRook];
}

U64 randomFenBoard::Queens(Color color)
{
    return bitboards[color == Black ? BlackQueen : WhiteQueen];
}

U64 randomFenBoard::Kings(Color color)
{
    return bitboards[color == Black ? BlackKing : WhiteKing];
}

U64 randomFenBoard::All()
{
    return bitboards[WhitePawn] | bitboards[WhiteKnight] | bitboards[WhiteBishop] | bitboards[WhiteRook] |
           bitboards[WhiteQueen] | bitboards[WhiteKing] | bitboards[BlackPawn] | bitboards[BlackKnight] |
           bitboards[BlackBishop] | bitboards[BlackRook] | bitboards[BlackQueen] | bitboards[BlackKing];
}

bool randomFenBoard::isAttacked(int sq, Color c)
{
    if (Pawns(c) & PawnAttacks(sq, ~c))
        return true;
    if (Knights(c) & KnightAttacks(sq))
        return true;
    if ((Bishops(c) | Queens(c)) & BishopAttacks(sq, All()))
        return true;
    if ((Rooks(c) | Queens(c)) & RookAttacks(sq, All()))
        return true;
    if (Kings(c) & KING_ATTACKS_TABLE[sq])
        return true;
    return false;
}

std::string randomFenBoard::generateRandomFen()
{
    std::memset(piece_count, 0, sizeof(piece_count));
    std::memset(board, 0, sizeof(board));
    std::memset(bitboards, 0, sizeof(bitboards));

    std::uniform_int_distribution<int> distPieceCount{4, 32};
    std::uniform_int_distribution<int> distPiece{0, 30};
    std::uniform_int_distribution<int> distSquare{0, 63};
    std::uniform_int_distribution<int> distStm{0, 1};

    std::string fen = "";
    int placedPieces = 0;
    // int allowedPieces = distPieceCount(e);
    int allowedPieces = 6;

    int emptySquares = 0;
    int i = 56;
    int end = 64;

    int WhiteKingSq = distSquare(e);
    int BlackKingSq = distSquare(e);

    board[WhiteKingSq] = WhiteKing;
    board[BlackKingSq] = BlackKing;

    bitboards[WhiteKing] |= 1ULL << WhiteKingSq;
    bitboards[BlackKing] |= 1ULL << BlackKingSq;

    int matScore = 0;

    for (; i <= end && i >= 0; i++)
    {
        int num = distPiece(e);

        if (BlackKingSq == i || WhiteKingSq == i)
        {
            if (emptySquares != 0)
            {
                fen += std::to_string(emptySquares);
                emptySquares = 0;
            }

            fen += piece_to_char[board[i]];
            matScore += piece_values_random[board[i]];

            goto skip;
        }

        while (num == WhiteKing || num == BlackKing || num == WhitePawn || num == BlackPawn)
        {
            num = distPiece(e);
        }

        if (num < 12 && placedPieces < allowedPieces && piece_count[num] < max_pieces[num] &&
            !((num == WhitePawn || num == BlackPawn) && (i >= 55 || i <= 8)) &&
            (popcount(MASK_FILE[i] & bitboards[num]) == 0))
        {
            if (emptySquares != 0)
            {
                fen += std::to_string(emptySquares);
                emptySquares = 0;
            }

            board[i] = static_cast<Piece>(num);
            piece_count[num]++;
            placedPieces++;

            bitboards[num] |= 1ULL << i;
            fen += piece_to_char[num];
            matScore += piece_values_random[board[i]];
        }
        else
        {
            board[i] = None;
            emptySquares++;
        }

    skip:
        if (square_file(Square(i)) == 7)
        {
            if (emptySquares != 0)
                fen += std::to_string(emptySquares);

            emptySquares = 0;
            fen += i != 7 ? "/" : "";

            i -= 16;
            end -= 8;
        }
    }

    if (isAttacked(WhiteKingSq, Black))
        return "";
    if (isAttacked(BlackKingSq, White))
        return "";
    if (std::abs(matScore) > 1500)
        return "";
    // if (std::abs(piece_count[WhitePawn] + piece_count[BlackPawn]) < 4)
    //     return "";
    // if (square_rank(WhiteKingSq) == square_rank(BlackKingSq))
    //     return "";
    // if (square_rank(WhiteKingSq) >= 3)
    //     return "";
    // if (square_rank(BlackKingSq) <= 4)
    //     return "";

    static constexpr char stm[] = {'w', 'b'};
    fen += " ";
    fen += stm[distStm(e)];
    fen += " - -";
    return fen;
}

std::string getRandomfen()
{
    randomFenBoard b;
    while (true)
    {
        std::string fen = b.generateRandomFen();
        if (fen != "")
            return fen;
    }
    return "";
}
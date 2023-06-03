#include "randomFen.h"
#include "attacks.h"
#include "helper.h"
#include "sliders.hpp"

/*
This was written very poorly but it does the job.
I actually have no idea how the random_device behaves when multiple threads access
it...
*/
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
    if (Pawns(c) & Attacks::Pawn(sq, ~c))
        return true;
    if (Knights(c) & Attacks::Knight(sq))
        return true;
    if ((Bishops(c) | Queens(c)) & Attacks::Bishop(sq, All()))
        return true;
    if ((Rooks(c) | Queens(c)) & Attacks::Rook(sq, All()))
        return true;
    if (Kings(c) & KING_ATTACKS_TABLE[sq])
        return true;
    return false;
}

std::stringstream randomFenBoard::generateRandomFen()
{
    board.fill({});
    piece_count.fill({});
    bitboards.fill({});

    std::uniform_int_distribution<> distPiece{0, 25};
    std::uniform_int_distribution<> distSquare{0, 63};
    std::uniform_int_distribution<> distStm{0, 1};

    std::stringstream ss;

    int placedPieces = 2;
    int allowedPieces = 32;
    int emptySquares = 0;
    int matScore = 0;

    int WhiteKingSq = distSquare(Random::generator);
    int BlackKingSq = distSquare(Random::generator);

    board[WhiteKingSq] = WhiteKing;
    board[BlackKingSq] = BlackKing;

    bitboards[WhiteKing] |= 1ULL << WhiteKingSq;
    bitboards[BlackKing] |= 1ULL << BlackKingSq;

    int i = 56;
    int end = 64;

    for (; i <= end && i >= 0; i++)
    {
        int num = distPiece(Random::generator);

        if (BlackKingSq == i || WhiteKingSq == i)
        {
            // write previous empty squares to fen
            if (emptySquares != 0)
            {
                ss << std::to_string(emptySquares);
                emptySquares = 0;
            }

            // add peice to fen
            ss << pieceToChar[Piece(board[i])];
            matScore += randValues[Piece(board[i])];

            goto skip;
        }

        // we already placed kigns at the start
        while (num == WhiteKing || num == BlackKing || // dont place pawns on the first or last rank
               ((num == WhitePawn || num == BlackPawn) && (i >= 55 || i <= 8)))
        {
            num = distPiece(Random::generator);
        }

        // clang-format off
        // check if piece is valid
        if (    num < None 
            &&  placedPieces <= allowedPieces 
            &&  piece_count[num] < max_pieces[num] 
            )
        {
            // clang-format on
            if (emptySquares != 0)
            {
                ss << std::to_string(emptySquares);
                emptySquares = 0;
            }

            placedPieces++;
            piece_count[num]++;

            board[i] = Piece(num);
            bitboards[num] |= 1ULL << i;

            matScore += randValues[Piece(board[i])];

            ss << pieceToChar[Piece(num)];
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
                ss << std::to_string(emptySquares);

            emptySquares = 0;
            ss << (i != 7 ? "/" : "");

            i -= 16;
            end -= 8;
        }
    }

    if (std::abs(matScore) > 1500)
    {
        return std::stringstream("");
    }
    if (isAttacked(WhiteKingSq, Black))
    {
        return std::stringstream("");
    }

    if (isAttacked(BlackKingSq, White))
    {
        return std::stringstream("");
    }

    static constexpr char stm[] = {'w', 'b'};
    ss << " ";
    ss << stm[distStm(Random::generator)];
    ss << " - -";

    return ss;
}

std::string getRandomfen()
{
    randomFenBoard b;
    std::stringstream fen = b.generateRandomFen();

    while (fen.str() == "")
    {
        fen = b.generateRandomFen();
    }

    return fen.str();
}
#include "randomFen.h"
#include "attacks.h"
#include "helper.h"
#include "sliders.hpp"

/*
This was written very poorly but it does the job.
I actually have no idea how the random_device behaves when multiple threads access
it...
*/
Bitboard randomFenBoard::Pawns(Color color) {
    return bitboards[color == BLACK ? BLACKPAWN : WHITEPAWN];
}

Bitboard randomFenBoard::Knights(Color color) {
    return bitboards[color == BLACK ? BLACKKNIGHT : WHITEKNIGHT];
}

Bitboard randomFenBoard::Bishops(Color color) {
    return bitboards[color == BLACK ? BLACKBISHOP : WHITEBISHOP];
}

Bitboard randomFenBoard::Rooks(Color color) {
    return bitboards[color == BLACK ? BLACKROOK : WHITEROOK];
}

Bitboard randomFenBoard::Queens(Color color) {
    return bitboards[color == BLACK ? BLACKQUEEN : WHITEQUEEN];
}

Bitboard randomFenBoard::Kings(Color color) {
    return bitboards[color == BLACK ? BLACKKING : WHITEKING];
}

Bitboard randomFenBoard::all() {
    return bitboards[WHITEPAWN] | bitboards[WHITEKNIGHT] | bitboards[WHITEBISHOP] |
           bitboards[WHITEROOK] | bitboards[WHITEQUEEN] | bitboards[WHITEKING] |
           bitboards[BLACKPAWN] | bitboards[BLACKKNIGHT] | bitboards[BLACKBISHOP] |
           bitboards[BLACKROOK] | bitboards[BLACKQUEEN] | bitboards[BLACKKING];
}

bool randomFenBoard::isAttacked(int sq, Color c) {
    if (Pawns(c) & attacks::Pawn(sq, ~c)) return true;
    if (Knights(c) & attacks::Knight(sq)) return true;
    if ((Bishops(c) | Queens(c)) & attacks::Bishop(sq, all())) return true;
    if ((Rooks(c) | Queens(c)) & attacks::Rook(sq, all())) return true;
    if (Kings(c) & KING_ATTACKS_TABLE[sq]) return true;
    return false;
}

std::stringstream randomFenBoard::generateRandomFen() {
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

    int WhiteKingSq = distSquare(rand_gen::generator);
    int BlackKingSq = distSquare(rand_gen::generator);

    board[WhiteKingSq] = WHITEKING;
    board[BlackKingSq] = BLACKKING;

    bitboards[WHITEKING] |= 1ULL << WhiteKingSq;
    bitboards[BLACKKING] |= 1ULL << BlackKingSq;

    int i = 56;
    int end = 64;

    for (; i <= end && i >= 0; i++) {
        int num = distPiece(rand_gen::generator);

        if (BlackKingSq == i || WhiteKingSq == i) {
            // write previous empty squares to fen
            if (emptySquares != 0) {
                ss << std::to_string(emptySquares);
                emptySquares = 0;
            }

            // add peice to fen
            ss << pieceToChar[Piece(board[i])];
            matScore += randValues[Piece(board[i])];

            goto skip;
        }

        // we already placed kigns at the start
        while (num == WHITEKING ||
               num == BLACKKING ||  // dont place pawns on the first or last rank
               ((num == WHITEPAWN || num == BLACKPAWN) && (i >= 55 || i <= 8))) {
            num = distPiece(rand_gen::generator);
        }

        // clang-format off
        // check if piece is valid
        if (    num < NONE 
            &&  placedPieces <= allowedPieces 
            &&  piece_count[num] < max_pieces[num] 
            )
        {
            // clang-format on
            if (emptySquares != 0) {
                ss << std::to_string(emptySquares);
                emptySquares = 0;
            }

            placedPieces++;
            piece_count[num]++;

            board[i] = Piece(num);
            bitboards[num] |= 1ULL << i;

            matScore += randValues[Piece(board[i])];

            ss << pieceToChar[Piece(num)];
        } else {
            board[i] = NONE;
            emptySquares++;
        }

    skip:
        if (squareFile(Square(i)) == 7) {
            if (emptySquares != 0) ss << std::to_string(emptySquares);

            emptySquares = 0;
            ss << (i != 7 ? "/" : "");

            i -= 16;
            end -= 8;
        }
    }

    if (std::abs(matScore) > 1500) {
        return std::stringstream("");
    }
    if (isAttacked(WhiteKingSq, BLACK)) {
        return std::stringstream("");
    }

    if (isAttacked(BlackKingSq, WHITE)) {
        return std::stringstream("");
    }

    static constexpr char stm[] = {'w', 'b'};
    ss << " ";
    ss << stm[distStm(rand_gen::generator)];
    ss << " - -";

    return ss;
}

std::string getRandomfen() {
    randomFenBoard b;
    std::stringstream fen = b.generateRandomFen();

    while (fen.str() == "") {
        fen = b.generateRandomFen();
    }

    return fen.str();
}
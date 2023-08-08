#include <bitset>

#include "helper.h"

uint8_t squareDistance(Square a, Square b) {
    return std::max(std::abs(squareFile(a) - squareFile(b)),
                    std::abs(squareRank(a) - squareRank(b)));
}

uint8_t manhattenDistance(Square sq1, Square sq2) {
    return std::abs(squareFile(sq1) - squareFile(sq2)) +
           std::abs(squareRank(sq1) - squareRank(sq2));
}

bool getSquareColor(Square square) {
    if ((square % 8) % 2 == (square / 8) % 2) {
        return false;
    } else {
        return true;
    }
}

void meanOf(int v) {
    means[0]++;
    means[1] += v;
}

void maxOf(int v) {
    max[0]++;
    if (v > max[1]) max[1] = v;
}

void minOf(int v) {
    min[0]++;
    if (v < min[1]) min[1] = v;
}

void printMean() {
    if (means[0])
        std::cout << "Total " << means[0] << " Mean " << (double)means[1] / means[0] << std::endl;
    if (min[0]) std::cout << "Total " << min[0] << " Min " << min[1] << std::endl;
    if (max[0]) std::cout << "Total " << max[0] << " Max " << max[1] << std::endl;
}

Piece makePiece(PieceType type, Color c) {
    if (type == NONETYPE) return NONE;
    return Piece(type + 6 * c);
}

void printBitboard(Bitboard bb) {
    std::bitset<64> b(bb);
    std::string str_bitset = b.to_string();
    for (int i = 0; i < MAX_SQ; i += 8) {
        std::string x = str_bitset.substr(i, 8);
        reverse(x.begin(), x.end());
        std::cout << x << std::endl;
    }
    std::cout << '\n' << std::endl;
}

bool sameColor(int sq1, int sq2) { return ((9 * (sq1 ^ sq2)) & 8) == 0; }

Square rookCastleSquare(Square to_sq, Square from_sq) {
    return fileRankSquare(to_sq > from_sq ? FILE_F : FILE_D, squareRank(from_sq));
}

Square kingCastleSquare(Square to_sq, Square from_sq) {
    return fileRankSquare(to_sq > from_sq ? FILE_G : FILE_C, squareRank(from_sq));
}

#pragma once

#include "../types.h"
#include "bitfield.h"

enum class CastleSide : uint8_t { KING_SIDE, QUEEN_SIDE };

class CastlingRights {
   public:
    template <Color color, CastleSide castle, File rook_file>
    void setCastlingRight() {
        int file = static_cast<uint16_t>(rook_file) + 1;

        castling_rights_.setGroupValue(2 * static_cast<int>(color) + static_cast<int>(castle),
                                       static_cast<uint16_t>(file));
    }

    void setCastlingRight(Color color, CastleSide castle, File rook_file) {
        int file = static_cast<uint16_t>(rook_file) + 1;

        castling_rights_.setGroupValue(2 * static_cast<int>(color) + static_cast<int>(castle),
                                       static_cast<uint16_t>(file));
    }

    void clearAllCastlingRights() { castling_rights_.clear(); }

    void clearCastlingRight(Color color, CastleSide castle) {
        castling_rights_.setGroupValue(2 * static_cast<int>(color) + static_cast<int>(castle), 0);
    }

    void clearCastlingRight(Color color) {
        castling_rights_.setGroupValue(2 * static_cast<int>(color), 0);
        castling_rights_.setGroupValue(2 * static_cast<int>(color) + 1, 0);
    }

    [[nodiscard]] bool isEmpty() const { return castling_rights_.get() == 0; }

    [[nodiscard]] bool hasCastlingRight(Color color) const {
        return castling_rights_.getGroup(2 * static_cast<int>(color)) != 0 ||
               castling_rights_.getGroup(2 * static_cast<int>(color) + 1) != 0;
    }

    [[nodiscard]] bool hasCastlingRight(Color color, CastleSide castle) const {
        return castling_rights_.getGroup(2 * static_cast<int>(color) + static_cast<int>(castle)) !=
               0;
    }

    [[nodiscard]] File getRookFile(Color color, CastleSide castle) const {
        return static_cast<File>(
            castling_rights_.getGroup(2 * static_cast<int>(color) + static_cast<int>(castle)) - 1);
    }

    [[nodiscard]] int getHashIndex() const {
        return hasCastlingRight(WHITE, CastleSide::KING_SIDE) +
               2 * hasCastlingRight(WHITE, CastleSide::QUEEN_SIDE) +
               4 * hasCastlingRight(BLACK, CastleSide::KING_SIDE) +
               8 * hasCastlingRight(BLACK, CastleSide::QUEEN_SIDE);
    }

   private:
    /*
     denotes the file of the rook that we castle to
     1248 1248 1248 1248
     0000 0000 0000 0000
     BQ   BK   WQ   WK
     3    2    1    0
     */
    BitField16 castling_rights_;
};
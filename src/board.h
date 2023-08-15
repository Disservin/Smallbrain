#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

#include "types/accumulators.h"
#include "types/castling_rights.h"
#include "types/state.h"

#include "attacks.h"
#include "builtin.h"
#include "helper.h"
#include "nnue.h"
#include "tt.h"
#include "types.h"
#include "zobrist.h"

extern TranspositionTable TTable;

class Board {
   public:
    /// @brief constructor for the board, loads startpos
    explicit Board(const std::string &fen = DEFAULT_POS);

    Board(const Board &other);

    Board &operator=(const Board &other);

    [[nodiscard]] U64 hash() const { return hash_key_; }

    [[nodiscard]] std::string getCastleString() const;

    [[nodiscard]] uint8_t halfmoves() const { return half_move_clock_; }

    [[nodiscard]] int ply() const { return full_move_number_ * 2; }

    [[nodiscard]] Color sideToMove() const { return side_to_move_; }

    [[nodiscard]] Square enPassant() const { return enpassant_sq_; }

    [[nodiscard]] const CastlingRights &castlingRights() const { return castling_rights_; }

    [[nodiscard]] nnue::accumulator &getAccumulator() { return accumulators_->back(); }

    void refreshNNUE(nnue::accumulator &acc) const;

    template <typename T = Piece>
    [[nodiscard]] T at(Square sq) const {
        if constexpr (std::is_same_v<T, PieceType>) {
            return typeOfPiece(board_[sq]);
        } else {
            return board_[sq];
        }
    }

    void setFen(const std::string &fen, bool update_acc = true);

    [[nodiscard]] std::string getFen() const;

    /// @brief detects if the position is a repetition by default 1, fide would be 3
    /// @param draw
    /// @return true for repetition otherwise false
    [[nodiscard]] bool isRepetition(int draw = 1) const;

    [[nodiscard]] Result isDrawn(bool in_check) const;

    /// @brief only pawns + king = true else false
    /// @param c
    /// @return
    [[nodiscard]] bool nonPawnMat(Color c) const;

    [[nodiscard]] Square kingSQ(Color c) const;

    [[nodiscard]] Bitboard us(Color c) const;

    template <Color c>
    [[nodiscard]] Bitboard us() const {
        return pieces_bb_[PAWN + c * 6] | pieces_bb_[KNIGHT + c * 6] | pieces_bb_[BISHOP + c * 6] |
               pieces_bb_[ROOK + c * 6] | pieces_bb_[QUEEN + c * 6] | pieces_bb_[KING + c * 6];
    }

    [[nodiscard]] Bitboard all() const;

    /// @brief Returns the square of the king for a certain color
    /// @param color
    /// @return
    [[nodiscard]] Square kingSq(Color color) const {
        assert(pieces(PieceType::KING, color) != 0);
        return builtin::lsb(pieces(PieceType::KING, color));
    }

    // Gets individual piece bitboards

    [[nodiscard]] constexpr Bitboard pieces(Piece piece) const { return pieces_bb_[piece]; }

    template <PieceType piece_type, Color c>
    [[nodiscard]] constexpr Bitboard pieces() const {
        return pieces_bb_[piece_type + c * 6];
    }

    [[nodiscard]] constexpr Bitboard pieces(PieceType piece_type, Color c) const {
        return pieces_bb_[piece_type + c * 6];
    }

    [[nodiscard]] constexpr Bitboard pieces(PieceType piece_type) const {
        return pieces_bb_[piece_type] | pieces_bb_[piece_type + 6];
    }

    /// @brief returns the color of a piece at a square
    /// @param loc
    /// @return
    [[nodiscard]] Color colorOf(Square loc) const;

    /// @brief  Checks if a square is attacked by the given color.
    /// @param c
    /// @param sq
    /// @param occ
    /// @return
    [[nodiscard]] bool isAttacked(Color c, Square sq, Bitboard occ) const;

    void updateHash(Move move);

    template <bool updateNNUE>
    void makeMove(Move move);

    template <bool updateNNUE>
    void unmakeMove(Move move);

    void makeNullMove();

    void unmakeNullMove();

    // update the internal board representation

    template <bool updateNNUE>
    void removePiece(Piece piece, Square sq, Square ksq_white = SQ_A1, Square ksq_black = SQ_A1);

    template <bool updateNNUE>
    void placePiece(Piece piece, Square sq, Square ksq_white = SQ_A1, Square ksq_black = SQ_A1);

    template <bool updateNNUE>
    void movePiece(Piece piece, Square from_sq, Square to_sq, Square ksq_white = SQ_A1,
                   Square ksq_black = SQ_A1);

    [[nodiscard]] U64 zobrist() const;

    void clearStacks();

    friend std::ostream &operator<<(std::ostream &os, const Board &b);

    bool chess960 = false;

   private:
    std::unique_ptr<Accumulators> accumulators_ = std::make_unique<Accumulators>();

    std::vector<State> state_history_;

    std::array<Bitboard, 12> pieces_bb_ = {};
    std::array<Piece, MAX_SQ> board_{};

    Bitboard occupancy_bb_ = 0ULL;

    // current hashkey
    U64 hash_key_{};

    CastlingRights castling_rights_;

    // full moves start at 1
    uint16_t full_move_number_;

    // halfmoves start at 0
    uint8_t half_move_clock_;

    Color side_to_move_;

    // NO_SQ when enpassant is not possible
    Square enpassant_sq_;
};

template <bool updateNNUE>
void Board::removePiece(Piece piece, Square sq, Square ksq_white, Square ksq_black) {
    pieces_bb_[piece] &= ~(1ULL << sq);
    board_[sq] = NONE;

    occupancy_bb_ &= ~(1ULL << sq);

    if constexpr (updateNNUE) {
        nnue::deactivate(getAccumulator(), sq, piece, ksq_white, ksq_black);
    }
}

template <bool updateNNUE>
void Board::placePiece(Piece piece, Square sq, Square ksq_white, Square ksq_black) {
    pieces_bb_[piece] |= (1ULL << sq);
    board_[sq] = piece;

    occupancy_bb_ |= (1ULL << sq);

    if constexpr (updateNNUE) {
        nnue::activate(getAccumulator(), sq, piece, ksq_white, ksq_black);
    }
}

template <bool updateNNUE>
void Board::movePiece(Piece piece, Square from_sq, Square to_sq, Square ksq_white,
                      Square ksq_black) {
    pieces_bb_[piece] &= ~(1ULL << from_sq);
    pieces_bb_[piece] |= (1ULL << to_sq);
    board_[from_sq] = NONE;
    board_[to_sq] = piece;

    occupancy_bb_ &= ~(1ULL << from_sq);
    occupancy_bb_ |= (1ULL << to_sq);

    if constexpr (updateNNUE) {
        if (typeOfPiece(piece) == KING && nnue::KING_BUCKET[from_sq] != nnue::KING_BUCKET[to_sq]) {
            refreshNNUE(getAccumulator());
        } else {
            nnue::move(getAccumulator(), from_sq, to_sq, piece, ksq_white, ksq_black);
        }
    }
}

inline void Board::updateHash(Move move) {
    const auto piece_type = at<PieceType>(move.from());
    const Piece piece = makePiece(piece_type, side_to_move_);
    const Square from_sq = move.from();
    const Square to_sq = move.to();
    const Piece capture = board_[to_sq];
    const Rank rank = squareRank(to_sq);

    if (enpassant_sq_ != NO_SQ) {
        hash_key_ ^= zobrist::enpassant(squareFile(enpassant_sq_));
    }

    enpassant_sq_ = NO_SQ;

    hash_key_ ^= zobrist::castling(castling_rights_.getHashIndex());

    if (piece_type == KING) {
        castling_rights_.clearCastlingRight(side_to_move_);

        if (move.typeOf() == Move::CASTLING) {
            assert(at<PieceType>(to_sq) == ROOK);

            const Piece rook = makePiece(ROOK, side_to_move_);
            const Square rook_sq = rookCastleSquare(to_sq, from_sq);
            const Square king_to_sq = kingCastleSquare(to_sq, from_sq);

            hash_key_ ^= zobrist::piece(rook, to_sq);
            hash_key_ ^= zobrist::piece(rook, rook_sq);
            hash_key_ ^= zobrist::piece(piece, from_sq);
            hash_key_ ^= zobrist::piece(piece, king_to_sq);

            hash_key_ ^= zobrist::sideToMove();
            hash_key_ ^= zobrist::castling(castling_rights_.getHashIndex());

            return;
        }
    } else if (piece_type == ROOK &&
               ((squareRank(from_sq) == Rank::RANK_8 && side_to_move_ == BLACK) ||
                (squareRank(from_sq) == Rank::RANK_1 && side_to_move_ == WHITE))) {
        const auto king_sq = builtin::lsb(pieces(KING, side_to_move_));
        const auto side = from_sq > king_sq ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE;

        if (castling_rights_.getRookFile(side_to_move_, side) == squareFile(from_sq)) {
            castling_rights_.clearCastlingRight(side_to_move_, side);
        }
    } else if (piece_type == PAWN) {
        const auto ep_sq = Square(to_sq ^ 8);

        half_move_clock_ = 0;

        if (move.typeOf() == Move::ENPASSANT) {
            hash_key_ ^= zobrist::piece(makePiece(PAWN, ~side_to_move_), ep_sq);
        } else if (std::abs(from_sq - to_sq) == 16) {
            Bitboard ep_mask = attacks::pawn(ep_sq, side_to_move_);

            if (ep_mask & pieces(PAWN, ~side_to_move_)) {
                enpassant_sq_ = ep_sq;
                hash_key_ ^= zobrist::enpassant(squareFile(enpassant_sq_));

                assert(at(enpassant_sq_) == NONE);
            }
        }
    }

    if (capture != Piece::NONE) {
        half_move_clock_ = 0;

        hash_key_ ^= zobrist::piece(capture, to_sq);

        if (typeOfPiece(capture) == ROOK && ((rank == Rank::RANK_1 && side_to_move_ == BLACK) ||
                                             (rank == Rank::RANK_8 && side_to_move_ == WHITE))) {
            const auto king_sq = builtin::lsb(pieces(KING, ~side_to_move_));
            const auto side = to_sq > king_sq ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE;

            if (castling_rights_.getRookFile(~side_to_move_, side) == squareFile(to_sq)) {
                castling_rights_.clearCastlingRight(~side_to_move_, side);
            }
        }
    }

    if (move.typeOf() == Move::PROMOTION) {
        hash_key_ ^= zobrist::piece(makePiece(PAWN, side_to_move_), from_sq);
        hash_key_ ^= zobrist::piece(makePiece(move.promotionType(), side_to_move_), to_sq);
    } else {
        hash_key_ ^= zobrist::piece(piece, from_sq);
        hash_key_ ^= zobrist::piece(piece, to_sq);
    }

    hash_key_ ^= zobrist::sideToMove();
    hash_key_ ^= zobrist::castling(castling_rights_.getHashIndex());
}

/// @brief
/// @tparam updateNNUE
/// @param move
template <bool updateNNUE>
void Board::makeMove(const Move move) {
    const auto capture = at(move.to()) != Piece::NONE && move.typeOf() != Move::CASTLING;
    const auto captured = at(move.to());
    const auto pt = at<PieceType>(move.from());

    state_history_.emplace_back(hash_key_, castling_rights_, enpassant_sq_, half_move_clock_,
                                captured);

    half_move_clock_++;
    full_move_number_++;

    if (enpassant_sq_ != NO_SQ) hash_key_ ^= zobrist::enpassant(squareFile(enpassant_sq_));
    enpassant_sq_ = NO_SQ;

    if (capture) {
        half_move_clock_ = 0;

        removePiece<updateNNUE>(captured, move.to());
        hash_key_ ^= zobrist::piece(captured, move.to());

        const auto rank = squareRank(move.to());

        if (typeOfPiece(captured) == PieceType::ROOK &&
            ((rank == Rank::RANK_1 && side_to_move_ == Color::BLACK) ||
             (rank == Rank::RANK_8 && side_to_move_ == Color::WHITE))) {
            const auto king_sq = kingSq(~side_to_move_);
            const auto file = move.to() > king_sq ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE;

            if (castling_rights_.getRookFile(~side_to_move_, file) == squareFile(move.to())) {
                const auto idx = castling_rights_.clearCastlingRight(~side_to_move_, file);
                hash_key_ ^= zobrist::castlingIndex(idx);
            }
        }
    }

    if (pt == PieceType::KING && castling_rights_.hasCastlingRight(side_to_move_)) {
        hash_key_ ^= zobrist::castling(castling_rights_.getHashIndex());

        castling_rights_.clearCastlingRight(side_to_move_);

        hash_key_ ^= zobrist::castling(castling_rights_.getHashIndex());
    } else if (pt == PieceType::ROOK && ourBackRank(move.from(), side_to_move_)) {
        const auto king_sq = kingSq(side_to_move_);
        const auto file = move.from() > king_sq ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE;

        if (castling_rights_.getRookFile(side_to_move_, file) == squareFile(move.from())) {
            const auto idx = castling_rights_.clearCastlingRight(side_to_move_, file);
            hash_key_ ^= zobrist::castlingIndex(idx);
        }
    } else if (pt == PieceType::PAWN) {
        half_move_clock_ = 0;

        const auto possible_ep = static_cast<Square>(move.to() ^ 8);
        if (std::abs(int(move.to()) - int(move.from())) == 16) {
            U64 ep_mask = attacks::pawn(possible_ep, side_to_move_);

            if (ep_mask & pieces(PieceType::PAWN, ~side_to_move_)) {
                enpassant_sq_ = possible_ep;

                hash_key_ ^= zobrist::enpassant(squareFile(enpassant_sq_));
                assert(at(enpassant_sq_) == Piece::NONE);
            }
        }
    }

    if (move.typeOf() == Move::CASTLING) {
        assert(at<PieceType>(move.from()) == PieceType::KING);
        assert(at<PieceType>(move.to()) == PieceType::ROOK);

        bool king_side = move.to() > move.from();
        auto rookTo = relativeSquare(side_to_move_, king_side ? Square::SQ_F1 : Square::SQ_D1);
        auto kingTo = relativeSquare(side_to_move_, king_side ? Square::SQ_G1 : Square::SQ_C1);

        const auto king = at(move.from());
        const auto rook = at(move.to());

        removePiece<updateNNUE>(king, move.from());
        removePiece<updateNNUE>(rook, move.to());

        assert(king == makePiece(side_to_move_, PieceType::KING));
        assert(rook == makePiece(side_to_move_, PieceType::ROOK));

        placePiece<updateNNUE>(king, kingTo);
        placePiece<updateNNUE>(rook, rookTo);

        if (updateNNUE && nnue::KING_BUCKET[move.from()] != nnue::KING_BUCKET[kingTo]) {
            refreshNNUE(getAccumulator());
        }

        hash_key_ ^= zobrist::piece(king, move.from()) ^ zobrist::piece(king, kingTo);
        hash_key_ ^= zobrist::piece(rook, move.to()) ^ zobrist::piece(rook, rookTo);
    } else if (move.typeOf() == Move::PROMOTION) {
        const auto piece_pawn = makePiece(PieceType::PAWN, side_to_move_);
        const auto piece_prom = makePiece(move.promotionType(), side_to_move_);

        removePiece<updateNNUE>(piece_pawn, move.from());
        placePiece<updateNNUE>(piece_prom, move.to());

        hash_key_ ^=
            zobrist::piece(piece_pawn, move.from()) ^ zobrist::piece(piece_prom, move.to());
    } else {
        assert(at(move.from()) != Piece::NONE);
        assert(at(move.to()) == Piece::NONE);
        const auto piece = at(move.from());

        removePiece<updateNNUE>(piece, move.from());
        placePiece<updateNNUE>(piece, move.to());

        hash_key_ ^= zobrist::piece(piece, move.from()) ^ zobrist::piece(piece, move.to());
    }

    if (move.typeOf() == Move::ENPASSANT) {
        assert(at<PieceType>(move.to() ^ 8) == PieceType::PAWN);

        const auto piece = makePiece(PieceType::PAWN, ~side_to_move_);

        removePiece<updateNNUE>(piece, Square(int(move.to()) ^ 8));

        hash_key_ ^= zobrist::piece(piece, Square(int(move.to()) ^ 8));
    }

    hash_key_ ^= zobrist::sideToMove();

    side_to_move_ = ~side_to_move_;
}

// template <bool updateNNUE>
// void Board::makeMove(const Move move) {
//     assert(move.from() >= 0 && move.from() < 64);
//     assert(move.to() >= 0 && move.to() < 64);
//     assert(typeOfPiece(at(move.to())) != KING);
//     assert(at<PieceType>(move.from()) != NONETYPE);
//     assert(at(move.from()) != NONE);
//     assert((move.typeOf() == Move::PROMOTION &&
//             (move.promotionType() != PAWN && move.promotionType() != KING)) ||
//            move.typeOf() != Move::PROMOTION);

//     const Square from_sq = move.from();
//     const Square to_sq = move.to();

//     const auto piece_type = at<PieceType>(from_sq);

//     const Piece piece = at(from_sq);
//     const Piece capture = at(to_sq);

//     const bool ep = to_sq == enpassant_sq_,;

//     // *****************************
//     // STORE STATE HISTORY
//     // *****************************

//     state_history_.emplace_back(hash_key_, castling_rights_, enpassant_sq_,,
//     half_move_clock_,
//                                 capture);

//     if constexpr (updateNNUE) accumulators_->push();

//     half_move_clock_++;
//     full_move_number_++;

//     // *****************************
//     // UPDATE HASH
//     // *****************************

//     updateHash(move);

//     TTable.prefetch(hash_key_);

//     const Square ksq_white = builtin::lsb(pieces<KING, WHITE>());
//     const Square ksq_black = builtin::lsb(pieces<KING, BLACK>());

//     // *****************************
//     // UPDATE PIECES AND NNUE
//     // *****************************

//     if (move.typeOf() == Move::CASTLING) {
//         const Piece rook = makePiece(ROOK, side_to_move_);
//         Square rook_to_sq = rookCastleSquare(to_sq, from_sq);
//         Square king_to_sq = kingCastleSquare(to_sq, from_sq);

//         if (updateNNUE && nnue::KING_BUCKET[from_sq] != nnue::KING_BUCKET[king_to_sq]) {
//             removePiece<false>(piece, from_sq, ksq_white, ksq_black);
//             removePiece<false>(rook, to_sq, ksq_white, ksq_black);

//             placePiece<false>(piece, king_to_sq, ksq_white, ksq_black);
//             placePiece<false>(rook, rook_to_sq, ksq_white, ksq_black);

//             refreshNNUE(getAccumulator());
//         } else {
//             removePiece<updateNNUE>(piece, from_sq, ksq_white, ksq_black);
//             removePiece<updateNNUE>(rook, to_sq, ksq_white, ksq_black);

//             placePiece<updateNNUE>(piece, king_to_sq, ksq_white, ksq_black);
//             placePiece<updateNNUE>(rook, rook_to_sq, ksq_white, ksq_black);
//         }

//         side_to_move_ = ~side_to_move_;

//         return;
//     } else if (piece_type == PAWN && ep) {
//         const auto ep_sq = Square(to_sq ^ 8);

//         assert(at<PieceType>(ep_sq) == PAWN);
//         removePiece<updateNNUE>(makePiece(PAWN, ~side_to_move_), ep_sq, ksq_white, ksq_black);
//     } else if (capture != Piece::NONE) {
//         assert(at(to_sq) != Piece::NONE);
//         removePiece<updateNNUE>(capture, to_sq, ksq_white, ksq_black);
//     }

//     // The move is differently encoded for promotions to it requires some special care.
//     if (move.typeOf() == Move::PROMOTION) {
//         // Captured piece is already removed
//         assert(at(to_sq) == Piece::NONE);

//         removePiece<updateNNUE>(makePiece(PAWN, side_to_move_), from_sq, ksq_white, ksq_black);
//         placePiece<updateNNUE>(makePiece(move.promotionType(), side_to_move_), to_sq, ksq_white,
//                                ksq_black);
//     } else {
//         assert(at(to_sq) == Piece::NONE);

//         movePiece<updateNNUE>(piece, from_sq, to_sq, ksq_white, ksq_black);
//     }

//     side_to_move_ = ~side_to_move_;
// }

template <bool updateNNUE>
void Board::unmakeMove(Move move) {
    const State restore = state_history_.back();
    const Square from_sq = move.from();
    const Square to_sq = move.to();

    const auto piece_type = at<PieceType>(to_sq);

    const Piece piece = makePiece(piece_type, ~side_to_move_);
    const Piece capture = restore.captured_piece;

    const bool promotion = move.typeOf() == Move::PROMOTION;

    if (accumulators_->size()) {
        accumulators_->pop();
    }

    hash_key_ = restore.hash;
    enpassant_sq_ = restore.enpassant;
    castling_rights_ = restore.castling;
    half_move_clock_ = restore.half_moves;

    state_history_.pop_back();

    full_move_number_--;
    side_to_move_ = ~side_to_move_;

    if (move.typeOf() == Move::CASTLING) {
        const Piece rook = makePiece(ROOK, side_to_move_);

        const Square rook_from_sq = rookCastleSquare(to_sq, from_sq);
        const Square king_to_sq = kingCastleSquare(to_sq, from_sq);

        // We need to remove both pieces first and then place them back.
        removePiece<updateNNUE>(rook, rook_from_sq);
        removePiece<updateNNUE>(makePiece(KING, side_to_move_), king_to_sq);

        placePiece<updateNNUE>(makePiece(KING, side_to_move_), from_sq);
        placePiece<updateNNUE>(rook, to_sq);

        return;
    } else if (promotion) {
        removePiece<updateNNUE>(makePiece(move.promotionType(), side_to_move_), to_sq);
        placePiece<updateNNUE>(makePiece(PAWN, side_to_move_), from_sq);

        if (capture != NONE) placePiece<updateNNUE>(capture, to_sq);
        return;
    } else {
        movePiece<updateNNUE>(piece, to_sq, from_sq);
    }

    if (to_sq == enpassant_sq_ && piece_type == PAWN) {
        const auto ep_sq = Square(enpassant_sq_ ^ 8);
        placePiece<updateNNUE>(makePiece(PAWN, ~side_to_move_), ep_sq);
    } else if (capture != NONE) {
        placePiece<updateNNUE>(capture, to_sq);
    }
}

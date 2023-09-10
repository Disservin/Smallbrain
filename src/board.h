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

    [[nodiscard]] int fullMoveNumber() const { return 1 + plies_played_ / 2; }

    [[nodiscard]] int ply() const { return plies_played_ + 1; }

    [[nodiscard]] Color sideToMove() const { return side_to_move_; }

    [[nodiscard]] Square enPassant() const { return en_passant_square_; }

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
    uint16_t plies_played_;

    // halfmoves start at 0
    uint8_t half_move_clock_;

    Color side_to_move_;

    // NO_SQ when enpassant is not possible
    Square en_passant_square_;
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
    const auto piece_type = at<PieceType>(from(move));
    const Piece piece = makePiece(piece_type, side_to_move_);
    const Square from_sq = from(move);
    const Square to_sq = to(move);
    const Piece capture = board_[to_sq];
    const Rank rank = squareRank(to_sq);

    if (en_passant_square_ != NO_SQ) {
        hash_key_ ^= zobrist::enpassant(squareFile(en_passant_square_));
    }

    en_passant_square_ = NO_SQ;

    hash_key_ ^= zobrist::castling(castling_rights_.getHashIndex());

    if (piece_type == KING) {
        castling_rights_.clearCastlingRight(side_to_move_);

        if (typeOf(move) == CASTLING) {
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

        if (typeOf(move) == ENPASSANT) {
            hash_key_ ^= zobrist::piece(makePiece(PAWN, ~side_to_move_), ep_sq);
        } else if (std::abs(from_sq - to_sq) == 16) {
            Bitboard ep_mask = attacks::pawn(ep_sq, side_to_move_);

            if (ep_mask & pieces(PAWN, ~side_to_move_)) {
                en_passant_square_ = ep_sq;
                hash_key_ ^= zobrist::enpassant(squareFile(en_passant_square_));

                assert(at(en_passant_square_) == NONE);
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

    if (typeOf(move) == PROMOTION) {
        hash_key_ ^= zobrist::piece(makePiece(PAWN, side_to_move_), from_sq);
        hash_key_ ^= zobrist::piece(makePiece(promotionType(move), side_to_move_), to_sq);
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
    assert(from(move) >= 0 && from(move) < 64);
    assert(to(move) >= 0 && to(move) < 64);
    assert(typeOfPiece(at(to(move))) != KING);
    assert(at<PieceType>(from(move)) != NONETYPE);
    assert(at(from(move)) != NONE);
    assert((typeOf(move) == PROMOTION &&
            (promotionType(move) != PAWN && promotionType(move) != KING)) ||
           typeOf(move) != PROMOTION);

    const Square from_sq = from(move);
    const Square to_sq = to(move);

    const auto piece_type = at<PieceType>(from_sq);

    const Piece piece = at(from_sq);
    const Piece capture = at(to_sq);

    const bool ep = to_sq == en_passant_square_;

    // *****************************
    // STORE STATE HISTORY
    // *****************************

    state_history_.emplace_back(hash_key_, castling_rights_, en_passant_square_, half_move_clock_,
                                capture);

    if constexpr (updateNNUE) accumulators_->push();

    half_move_clock_++;
    plies_played_++;

    // *****************************
    // UPDATE HASH
    // *****************************

    updateHash(move);

    TTable.prefetch(hash_key_);

    const Square ksq_white = builtin::lsb(pieces<KING, WHITE>());
    const Square ksq_black = builtin::lsb(pieces<KING, BLACK>());

    // *****************************
    // UPDATE PIECES AND NNUE
    // *****************************

    if (typeOf(move) == CASTLING) {
        const Piece rook = makePiece(ROOK, side_to_move_);
        Square rook_to_sq = rookCastleSquare(to_sq, from_sq);
        Square king_to_sq = kingCastleSquare(to_sq, from_sq);

        if (updateNNUE && nnue::KING_BUCKET[from_sq] != nnue::KING_BUCKET[king_to_sq]) {
            removePiece<false>(piece, from_sq, ksq_white, ksq_black);
            removePiece<false>(rook, to_sq, ksq_white, ksq_black);

            placePiece<false>(piece, king_to_sq, ksq_white, ksq_black);
            placePiece<false>(rook, rook_to_sq, ksq_white, ksq_black);

            refreshNNUE(getAccumulator());
        } else {
            removePiece<updateNNUE>(piece, from_sq, ksq_white, ksq_black);
            removePiece<updateNNUE>(rook, to_sq, ksq_white, ksq_black);

            placePiece<updateNNUE>(piece, king_to_sq, ksq_white, ksq_black);
            placePiece<updateNNUE>(rook, rook_to_sq, ksq_white, ksq_black);
        }

        side_to_move_ = ~side_to_move_;

        return;
    } else if (piece_type == PAWN && ep) {
        const auto ep_sq = Square(to_sq ^ 8);

        assert(at<PieceType>(ep_sq) == PAWN);
        removePiece<updateNNUE>(makePiece(PAWN, ~side_to_move_), ep_sq, ksq_white, ksq_black);
    } else if (capture != Piece::NONE) {
        assert(at(to_sq) != Piece::NONE);
        removePiece<updateNNUE>(capture, to_sq, ksq_white, ksq_black);
    }

    // The move is differently encoded for promotions to it requires some special care.
    if (typeOf(move) == PROMOTION) {
        // Captured piece is already removed
        assert(at(to_sq) == Piece::NONE);

        removePiece<updateNNUE>(makePiece(PAWN, side_to_move_), from_sq, ksq_white, ksq_black);
        placePiece<updateNNUE>(makePiece(promotionType(move), side_to_move_), to_sq, ksq_white,
                               ksq_black);
    } else {
        assert(at(to_sq) == Piece::NONE);

        movePiece<updateNNUE>(piece, from_sq, to_sq, ksq_white, ksq_black);
    }

    side_to_move_ = ~side_to_move_;
}

template <bool updateNNUE>
void Board::unmakeMove(Move move) {
    const State restore = state_history_.back();
    const Square from_sq = from(move);
    const Square to_sq = to(move);

    const auto piece_type = at<PieceType>(to_sq);

    const Piece piece = makePiece(piece_type, ~side_to_move_);
    const Piece capture = restore.captured_piece;

    const bool promotion = typeOf(move) == PROMOTION;

    if (accumulators_->size()) {
        accumulators_->pop();
    }

    hash_key_ = restore.hash;
    en_passant_square_ = restore.enpassant;
    castling_rights_ = restore.castling;
    half_move_clock_ = restore.half_moves;

    state_history_.pop_back();

    plies_played_--;
    side_to_move_ = ~side_to_move_;

    if (typeOf(move) == CASTLING) {
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
        removePiece<updateNNUE>(makePiece(promotionType(move), side_to_move_), to_sq);
        placePiece<updateNNUE>(makePiece(PAWN, side_to_move_), from_sq);

        if (capture != NONE) placePiece<updateNNUE>(capture, to_sq);
        return;
    } else {
        movePiece<updateNNUE>(piece, to_sq, from_sq);
    }

    if (to_sq == en_passant_square_ && piece_type == PAWN) {
        const auto ep_sq = Square(en_passant_square_ ^ 8);
        placePiece<updateNNUE>(makePiece(PAWN, ~side_to_move_), ep_sq);
    } else if (capture != NONE) {
        placePiece<updateNNUE>(capture, to_sq);
    }
}

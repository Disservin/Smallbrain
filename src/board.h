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

    U64 keyAfter(Move move) const;

    template <bool updateNNUE>
    void makeMove(const Move &move);

    template <bool updateNNUE>
    void unmakeMove(const Move &move);

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
        if ((piece == WHITEKING || piece == BLACKKING) &&
            nnue::KING_BUCKET[from_sq] != nnue::KING_BUCKET[to_sq]) {
            refreshNNUE(getAccumulator());
        } else {
            nnue::move(getAccumulator(), from_sq, to_sq, piece, ksq_white, ksq_black);
        }
    }
}

inline U64 Board::keyAfter(Move move) const {
    Square from = move.from();
    Square to = move.to();
    Piece pc = at(from);
    Piece captured = at(to);
    U64 k = hash_key_ ^ zobrist::sideToMove();

    if (captured) k ^= zobrist::piece(captured, to);

    return k ^ zobrist::piece(pc, to) ^ zobrist::piece(pc, from);
}

/// @brief
/// @tparam updateNNUE
/// @param move
template <bool updateNNUE>
void Board::makeMove(const Move &move) {
    assert(move.from() >= 0 && move.from() < 64);
    assert(move.to() >= 0 && move.to() < 64);
    assert(typeOfPiece(at(move.to())) != KING);
    assert(at<PieceType>(move.from()) != NONETYPE);
    assert(at(move.from()) != NONE);
    assert((move.typeOf() == Move::PROMOTION &&
            (move.promotionType() != PAWN && move.promotionType() != KING)) ||
           move.typeOf() != Move::PROMOTION);

    const auto from_sq = move.from();
    const auto to_sq = move.to();

    const auto piece = at(from_sq);
    const auto captured = at(to_sq);

    U64 k = hash_key_ ^ zobrist::sideToMove();

    if (captured) k ^= zobrist::piece(captured, to_sq);

    TTable.prefetch(k ^ zobrist::piece(piece, to_sq) ^ zobrist::piece(piece, from_sq));

    const auto move_type = move.typeOf();
    const auto rank = squareRank(to_sq);
    const auto piece_type = at<PieceType>(from_sq);
    const auto capture = at(move.to()) != Piece::NONE && move_type != Move::CASTLING;

    // *****************************
    // STORE STATE HISTORY
    // *****************************

    state_history_.emplace_back(hash_key_, castling_rights_, en_passant_square_, half_move_clock_,
                                capture ? captured : Piece::NONE);

    if constexpr (updateNNUE) accumulators_->push();

    half_move_clock_++;
    full_move_number_++;

    const Square ksq_white = builtin::lsb(pieces<KING, WHITE>());
    const Square ksq_black = builtin::lsb(pieces<KING, BLACK>());

    const Square ksq = side_to_move_ == WHITE ? ksq_white : ksq_black;
    const Square ksq_opp = side_to_move_ == WHITE ? ksq_black : ksq_white;

    // *****************************
    // UPDATE HASH
    // *****************************

    if (en_passant_square_ != NO_SQ) {
        hash_key_ ^= zobrist::enpassant(squareFile(en_passant_square_));
    }

    en_passant_square_ = NO_SQ;

    // *****************************
    // UPDATE PIECES AND NNUE
    // *****************************

    if (capture || piece_type == PAWN) {
        half_move_clock_ = 0;
    }

    if (capture) {
        removePiece<updateNNUE>(captured, to_sq, ksq_white, ksq_black);
        hash_key_ ^= zobrist::piece(captured, to_sq);
    }

    if (capture && typeOfPiece(captured) == ROOK &&
        ((rank == Rank::RANK_1 && side_to_move_ == BLACK) ||
         (rank == Rank::RANK_8 && side_to_move_ == WHITE))) {
        const auto file = move.to() > ksq_opp ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE;

        if (castling_rights_.getRookFile(~side_to_move_, file) == squareFile(move.to()) &&
            castling_rights_.hasCastlingRight(~side_to_move_, file)) {
            hash_key_ ^=
                zobrist::castlingIndex(castling_rights_.clearCastlingRight(~side_to_move_, file));
        }
    }

    if (piece_type == KING && castling_rights_.hasCastlingRight(side_to_move_)) {
        hash_key_ ^= zobrist::castling(castling_rights_.getHashIndex());
        castling_rights_.clearCastlingRight(side_to_move_);
        hash_key_ ^= zobrist::castling(castling_rights_.getHashIndex());
    }

    if (piece_type == PieceType::ROOK && ourBackRank(move.from(), side_to_move_)) {
        const auto file = move.from() > ksq ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE;

        if (castling_rights_.getRookFile(side_to_move_, file) == squareFile(move.from()) &&
            castling_rights_.hasCastlingRight(side_to_move_, file)) {
            hash_key_ ^=
                zobrist::castlingIndex(castling_rights_.clearCastlingRight(side_to_move_, file));
        }
    }

    if (piece_type == PieceType::PAWN && std::abs(from_sq - to_sq) == 16) {
        const auto ep_sq = Square(to_sq ^ 8);

        if (attacks::pawn(ep_sq, side_to_move_) & pieces(PAWN, ~side_to_move_)) {
            hash_key_ ^= zobrist::enpassant(squareFile(ep_sq));

            en_passant_square_ = ep_sq;
            assert(at(en_passant_square_) == NONE);
        }
    }

    if (move_type == Move::CASTLING) {
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

        hash_key_ ^= zobrist::piece(rook, to_sq);
        hash_key_ ^= zobrist::piece(rook, rook_to_sq);
        hash_key_ ^= zobrist::piece(piece, from_sq);
        hash_key_ ^= zobrist::piece(piece, king_to_sq);

        hash_key_ ^= zobrist::sideToMove();

        side_to_move_ = ~side_to_move_;

        return;
    } else if (move_type == Move::PROMOTION) {
        const auto prom_pawn = makePiece(PAWN, side_to_move_);
        const auto prom_piece = makePiece(move.promotionType(), side_to_move_);

        removePiece<updateNNUE>(prom_pawn, from_sq, ksq_white, ksq_black);
        placePiece<updateNNUE>(prom_piece, to_sq, ksq_white, ksq_black);

        hash_key_ ^= zobrist::piece(prom_pawn, from_sq);
        hash_key_ ^= zobrist::piece(prom_piece, to_sq);
    } else {
        assert(at(to_sq) == Piece::NONE);

        movePiece<updateNNUE>(piece, from_sq, to_sq, ksq_white, ksq_black);

        hash_key_ ^= zobrist::piece(piece, from_sq);
        hash_key_ ^= zobrist::piece(piece, to_sq);
    }

    if (move_type == Move::ENPASSANT) {
        const auto ep_sq = Square(to_sq ^ 8);
        const auto ep_pawn = makePiece(PAWN, ~side_to_move_);

        removePiece<updateNNUE>(ep_pawn, ep_sq, ksq_white, ksq_black);

        hash_key_ ^= zobrist::piece(ep_pawn, ep_sq);
    }

    hash_key_ ^= zobrist::sideToMove();

    side_to_move_ = ~side_to_move_;
}

template <bool updateNNUE>
void Board::unmakeMove(const Move &move) {
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
    en_passant_square_ = restore.enpassant;
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

    if (to_sq == en_passant_square_ && piece_type == PAWN) {
        const auto ep_sq = Square(en_passant_square_ ^ 8);
        placePiece<updateNNUE>(makePiece(PAWN, ~side_to_move_), ep_sq);
    } else if (capture != NONE) {
        placePiece<updateNNUE>(capture, to_sq);
    }
}

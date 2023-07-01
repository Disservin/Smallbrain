#pragma once

#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"

#include <array>
#include <iostream>
#include <string>
#include <memory>
#include <type_traits>

#include "types/accumulators.h"
#include "types/castling_rights.h"
#include "types/state.h"

#include "attacks.h"
#include "helper.h"
#include "nnue.h"
#include "tt.h"
#include "types.h"

extern TranspositionTable TTable;

class Board {
   public:
    /// @brief constructor for the board, loads startpos
    Board(std::string fen = DEFAULT_POS);

    Board(const Board &other);

    Board &operator=(const Board &other);

    [[nodiscard]] U64 hash() const { return hash_key_; }
    [[nodiscard]] std::string getCastleString() const;
    [[nodiscard]] uint8_t halfmoves() const { return half_move_clock_; }
    [[nodiscard]] Color sideToMove() const { return side_to_move_; }
    [[nodiscard]] Square enPassant() const { return en_passant_square_; }
    [[nodiscard]] const CastlingRights &castlingRights() const { return castling_rights_; }

    /// @brief reload the entire nnue
    void refresh();

    /// @brief Finds what piece is on the square using the board (more performant)
    /// @param sq
    /// @return found piece otherwise NONE
    template <typename T = Piece>
    [[nodiscard]] T at(Square sq) const {
        if constexpr (std::is_same_v<T, PieceType>) {
            return typeOfPiece(board_[sq]);
        } else {
            return board_[sq];
        }
    }

    /// @brief applys a new Fen to the board and also reload the entire nnue
    /// @param fen
    /// @param update_acc
    void setFen(const std::string &fen, bool update_acc = true);

    /// @brief returns a Fen string of the current board
    /// @return fen string
    [[nodiscard]] std::string getFen() const;

    /// @brief detects if the position is a repetition by default 1, fide would be 3
    /// @param draw
    /// @return true for repetition otherwise false
    [[nodiscard]] bool isRepetition(int draw = 1) const;

    [[nodiscard]] Result isDrawn(bool in_check);

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

    /// @brief
    /// @param c
    /// @param sq
    /// @param occ
    /// @return
    [[nodiscard]] bool isAttacked(Color c, Square sq, Bitboard occ) const;

    void updateHash(Move move);

    /// @brief plays the move on the internal board
    /// @tparam updateNNUE update true = update nnue
    /// @param move
    template <bool updateNNUE>
    void makeMove(Move move);

    /// @brief unmake a move played on the internal board
    /// @tparam updateNNUE update true = update nnue
    /// @param move
    template <bool updateNNUE>
    void unmakeMove(Move move);

    /// @brief make a nullmove
    void makeNullMove();

    /// @brief unmake a nullmove
    void unmakeNullMove();

    // update the internal board representation

    /// @brief Remove a Piece from the board
    /// @tparam update true = update nnue
    /// @param piece
    /// @param sq
    template <bool updateNNUE>
    void removePiece(Piece piece, Square sq, Square ksq_white = SQ_A1, Square ksq_black = SQ_A1);

    /// @brief Place a Piece on the board
    /// @tparam update
    /// @param piece
    /// @param sq
    template <bool updateNNUE>
    void placePiece(Piece piece, Square sq, Square ksq_white = SQ_A1, Square ksq_black = SQ_A1);

    /// @brief Move a piece on the board
    /// @tparam updateNNUE
    /// @param piece
    /// @param from_sq
    /// @param to_sq
    template <bool updateNNUE>
    void movePiece(Piece piece, Square from_sq, Square to_sq, Square ksq_white = SQ_A1,
                   Square ksq_black = SQ_A1);

    void clearStacks();

    void clearHash() { hash_history_.clear(); }

    friend std::ostream &operator<<(std::ostream &os, const Board &b);

    /// @brief calculate the current zobrist hash from scratch
    /// @return
    [[nodiscard]] U64 zobrist() const;

    [[nodiscard]] nnue::accumulator &getAccumulator() { return accumulators_->back(); }

    bool chess960 = false;

   private:
    // update the hash

    U64 updateKeyPiece(Piece piece, Square sq) const;
    U64 updateKeyCastling() const;
    U64 updateKeyEnPassant(Square sq) const;
    U64 updateKeySideToMove() const;

    std::unique_ptr<Accumulators> accumulators_ = std::make_unique<Accumulators>();

    std::vector<State> state_history_;

    // keeps track of previous hashes, used for
    // repetition detection
    std::vector<U64> hash_history_;

    std::array<Bitboard, 12> pieces_bb_ = {};
    std::array<Piece, MAX_SQ> board_;

    Bitboard occupancy_bb_ = 0ULL;

    // current hashkey
    U64 hash_key_;

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
        if (typeOfPiece(piece) == KING && nnue::KING_BUCKET[from_sq] != nnue::KING_BUCKET[to_sq]) {
            refresh();
        } else {
            nnue::move(getAccumulator(), from_sq, to_sq, piece, ksq_white, ksq_black);
        }
    }
}

inline void Board::updateHash(Move move) {
    const PieceType piece_type = at<PieceType>(from(move));
    const Piece piece = makePiece(piece_type, side_to_move_);
    const Square from_sq = from(move);
    const Square to_sq = to(move);
    const Piece capture = board_[to_sq];
    const Rank rank = square_rank(to_sq);

    hash_history_.emplace_back(hash_key_);

    if (en_passant_square_ != NO_SQ) hash_key_ ^= updateKeyEnPassant(en_passant_square_);
    en_passant_square_ = NO_SQ;

    hash_key_ ^= updateKeyCastling();

    if (piece_type == KING) {
        castling_rights_.clearCastlingRight(side_to_move_);

        if (typeOf(move) == CASTLING) {
            const Piece rook = side_to_move_ == WHITE ? WHITEROOK : BLACKROOK;
            const Square rookSQ = rookCastleSquare(to_sq, from_sq);
            const Square kingToSq = kingCastleSquare(to_sq, from_sq);

            assert(at<PieceType>(to_sq) == ROOK);

            hash_key_ ^= updateKeyPiece(rook, to_sq);
            hash_key_ ^= updateKeyPiece(rook, rookSQ);
            hash_key_ ^= updateKeyPiece(piece, from_sq);
            hash_key_ ^= updateKeyPiece(piece, kingToSq);

            hash_key_ ^= updateKeySideToMove();
            hash_key_ ^= updateKeyCastling();

            return;
        }
    } else if (piece_type == ROOK &&
               ((square_rank(from_sq) == Rank::RANK_8 && side_to_move_ == BLACK) ||
                (square_rank(from_sq) == Rank::RANK_1 && side_to_move_ == WHITE))) {
        const auto king_sq = builtin::lsb(pieces(KING, side_to_move_));

        castling_rights_.clearCastlingRight(
            side_to_move_, from_sq > king_sq ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE);
    } else if (piece_type == PAWN) {
        half_move_clock_ = 0;
        if (typeOf(move) == ENPASSANT) {
            hash_key_ ^= updateKeyPiece(makePiece(PAWN, ~side_to_move_), Square(to_sq ^ 8));
        } else if (std::abs(from_sq - to_sq) == 16) {
            Bitboard epMask = attacks::Pawn(Square(to_sq ^ 8), side_to_move_);
            if (epMask & pieces(PAWN, ~side_to_move_)) {
                en_passant_square_ = Square(to_sq ^ 8);
                hash_key_ ^= updateKeyEnPassant(en_passant_square_);

                assert(at(en_passant_square_) == NONE);
            }
        }
    }

    if (capture != NONE) {
        half_move_clock_ = 0;
        hash_key_ ^= updateKeyPiece(capture, to_sq);
        if (typeOfPiece(capture) == ROOK && ((rank == Rank::RANK_1 && side_to_move_ == BLACK) ||
                                             (rank == Rank::RANK_8 && side_to_move_ == WHITE))) {
            const auto king_sq = builtin::lsb(pieces(KING, ~side_to_move_));

            castling_rights_.clearCastlingRight(
                ~side_to_move_, to_sq > king_sq ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE);
        }
    }

    if (typeOf(move) == PROMOTION) {
        half_move_clock_ = 0;

        hash_key_ ^= updateKeyPiece(makePiece(PAWN, side_to_move_), from_sq);
        hash_key_ ^= updateKeyPiece(makePiece(promotionType(move), side_to_move_), to_sq);
    } else {
        hash_key_ ^= updateKeyPiece(piece, from_sq);
        hash_key_ ^= updateKeyPiece(piece, to_sq);
    }

    hash_key_ ^= updateKeySideToMove();
    hash_key_ ^= updateKeyCastling();
}

/// @brief
/// @tparam updateNNUE
/// @param move
template <bool updateNNUE>
void Board::makeMove(Move move) {
    const Square from_sq = from(move);
    const Square to_sq = to(move);

    const PieceType piece_type = at<PieceType>(from_sq);

    const Piece piece = at(from_sq);
    const Piece capture = at(to_sq);

    const bool ep = to_sq == en_passant_square_;

    assert(from_sq >= 0 && from_sq < 64);
    assert(to_sq >= 0 && to_sq < 64);
    assert(typeOfPiece(capture) != KING);
    assert(piece_type != NONETYPE);
    assert(piece != NONE);
    assert((typeOf(move) == PROMOTION &&
            (promotionType(move) != PAWN && promotionType(move) != KING)) ||
           typeOf(move) != PROMOTION);

    // *****************************
    // STORE STATE HISTORY
    // *****************************

    state_history_.emplace_back(en_passant_square_, castling_rights_, half_move_clock_, capture);

    if constexpr (updateNNUE) accumulators_->push();

    half_move_clock_++;
    full_move_number_++;

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
        Square kingToSq = kingCastleSquare(to_sq, from_sq);

        if (updateNNUE && nnue::KING_BUCKET[from_sq] != nnue::KING_BUCKET[kingToSq]) {
            removePiece<false>(piece, from_sq, ksq_white, ksq_black);
            removePiece<false>(rook, to_sq, ksq_white, ksq_black);

            placePiece<false>(piece, kingToSq, ksq_white, ksq_black);
            placePiece<false>(rook, rook_to_sq, ksq_white, ksq_black);

            refresh();
        } else {
            removePiece<updateNNUE>(piece, from_sq, ksq_white, ksq_black);
            removePiece<updateNNUE>(rook, to_sq, ksq_white, ksq_black);

            placePiece<updateNNUE>(piece, kingToSq, ksq_white, ksq_black);
            placePiece<updateNNUE>(rook, rook_to_sq, ksq_white, ksq_black);
        }

        side_to_move_ = ~side_to_move_;

        return;
    } else if (piece_type == PAWN && ep) {
        const Square ep_sq = Square(to_sq ^ 8);

        assert(at<PieceType>(ep_sq) == PAWN);
        removePiece<updateNNUE>(makePiece(PAWN, ~side_to_move_), ep_sq, ksq_white, ksq_black);
    } else if (capture != NONE) {
        assert(at(to_sq) != NONE);
        removePiece<updateNNUE>(capture, to_sq, ksq_white, ksq_black);
    }

    // The move is differently encoded for promotions to it requires some special care.
    if (typeOf(move) == PROMOTION) {
        // Captured piece is already removed
        assert(at(to_sq) == NONE);

        removePiece<updateNNUE>(makePiece(PAWN, side_to_move_), from_sq, ksq_white, ksq_black);
        placePiece<updateNNUE>(makePiece(promotionType(move), side_to_move_), to_sq, ksq_white,
                               ksq_black);
    } else {
        assert(at(to_sq) == NONE);

        movePiece<updateNNUE>(piece, from_sq, to_sq, ksq_white, ksq_black);
    }

    side_to_move_ = ~side_to_move_;
}

template <bool updateNNUE>
void Board::unmakeMove(Move move) {
    side_to_move_ = ~side_to_move_;

    const State restore = state_history_.back();

    const Square from_sq = from(move);
    const Square to_sq = to(move);

    const PieceType piece_type = at<PieceType>(to_sq);

    const Piece piece = makePiece(piece_type, side_to_move_);
    const Piece capture = restore.captured_piece;

    const bool promotion = typeOf(move) == PROMOTION;

    state_history_.pop_back();

    if (accumulators_->size()) {
        accumulators_->pop();
    }

    hash_key_ = hash_history_.back();
    hash_history_.pop_back();

    en_passant_square_ = restore.en_passant;
    castling_rights_ = restore.castling;
    half_move_clock_ = restore.half_move;

    full_move_number_--;

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
        const Square ep_sq = Square(en_passant_square_ ^ 8);
        placePiece<updateNNUE>(makePiece(PAWN, ~side_to_move_), ep_sq);
    } else if (capture != NONE) {
        placePiece<updateNNUE>(capture, to_sq);
    }
}

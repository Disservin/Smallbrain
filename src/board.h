#pragma once

#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"

#include <array>
#include <iostream>
#include <string>

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

    std::string getCastleString() const;

    /// @brief reload the entire nnue
    void refresh(nnue::accumulator &accumulator) const;

    /// @brief Finds what piece is on the square using the board (more performant)
    /// @param sq
    /// @return found piece otherwise None
    Piece pieceAtB(Square sq) const;

    /// @brief applys a new Fen to the board and also reload the entire nnue
    /// @param fen
    /// @param update_acc
    void applyFen(const std::string &fen, bool update_acc = true);

    /// @brief returns a Fen string of the current board
    /// @return fen string
    std::string getFen() const;

    /// @brief detects if the position is a repetition by default 1, fide would be 3
    /// @param draw
    /// @return true for repetition otherwise false
    bool isRepetition(int draw = 1) const;

    Result isDrawn(bool in_check);

    /// @brief only pawns + king = true else false
    /// @param c
    /// @return
    bool nonPawnMat(Color c) const;

    Square kingSQ(Color c) const;

    U64 enemy(Color c) const;

    U64 us(Color c) const;
    template <Color c>
    U64 us() const {
        return pieces_bb[PAWN + c * 6] | pieces_bb[KNIGHT + c * 6] | pieces_bb[BISHOP + c * 6] |
               pieces_bb[ROOK + c * 6] | pieces_bb[QUEEN + c * 6] | pieces_bb[KING + c * 6];
    }

    U64 all() const;

    // Gets individual piece bitboards

    constexpr U64 pieces(Piece p) const { return pieces_bb[p]; }

    template <PieceType p, Color c>
    constexpr U64 pieces() const {
        return pieces_bb[p + c * 6];
    }

    constexpr U64 pieces(PieceType p, Color c) const { return pieces_bb[p + c * 6]; }

    /// @brief returns the color of a piece at a square
    /// @param loc
    /// @return
    Color colorOf(Square loc) const;

    /// @brief
    /// @param c
    /// @param sq
    /// @param occ
    /// @return
    bool isSquareAttacked(Color c, Square sq, U64 occ) const;

    // attackers used for SEE
    U64 allAttackers(Square sq, U64 occupied_bb) const;
    U64 attackersForSide(Color attacker_color, Square sq, U64 occupied_bb) const;

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

    U64 attacksByPiece(PieceType pt, Square sq, Color c, U64 occupied);

    void clearStacks();

    friend std::ostream &operator<<(std::ostream &os, const Board &b);

    /// @brief calculate the current zobrist hash from scratch
    /// @return
    U64 zobristHash() const;

    [[nodiscard]] nnue::accumulator &getAccumulator() { return accumulators_.back(); }

    bool chess960 = false;

    Color side_to_move;

    // NO_SQ when enpassant is not possible
    Square en_passant_square;

    CastlingRights castling_rights;

    // halfmoves start at 0
    uint8_t half_move_clock;

    // full moves start at 1
    uint16_t full_move_number;

    // keeps track of previous hashes, used for
    // repetition detection
    std::vector<U64> hash_history;

    // current hashkey
    U64 hash_key;

   private:
    // update the hash

    U64 updateKeyPiece(Piece piece, Square sq) const;
    U64 updateKeyCastling() const;
    U64 updateKeyEnPassant(Square sq) const;
    U64 updateKeySideToMove() const;

    Accumulators accumulators_;
    std::vector<State> state_history;

    U64 pieces_bb[12] = {};
    std::array<Piece, MAX_SQ> board;
};

template <bool updateNNUE>
void Board::removePiece(Piece piece, Square sq, Square ksq_white, Square ksq_black) {
    pieces_bb[piece] &= ~(1ULL << sq);
    board[sq] = None;

    if constexpr (updateNNUE) {
        nnue::deactivate(getAccumulator(), sq, piece, ksq_white, ksq_black);
    }
}

template <bool updateNNUE>
void Board::placePiece(Piece piece, Square sq, Square ksq_white, Square ksq_black) {
    pieces_bb[piece] |= (1ULL << sq);
    board[sq] = piece;

    if constexpr (updateNNUE) {
        nnue::activate(getAccumulator(), sq, piece, ksq_white, ksq_black);
    }
}

template <bool updateNNUE>
void Board::movePiece(Piece piece, Square from_sq, Square to_sq, Square ksq_white,
                      Square ksq_black) {
    pieces_bb[piece] &= ~(1ULL << from_sq);
    pieces_bb[piece] |= (1ULL << to_sq);
    board[from_sq] = None;
    board[to_sq] = piece;

    if constexpr (updateNNUE) {
        if (type_of_piece(piece) == KING &&
            nnue::KING_BUCKET[from_sq] != nnue::KING_BUCKET[to_sq]) {
            refresh(getAccumulator());
        } else {
            nnue::move(getAccumulator(), from_sq, to_sq, piece, ksq_white, ksq_black);
        }
    }
}

inline void Board::updateHash(Move move) {
    PieceType pt = type_of_piece(pieceAtB(from(move)));
    Piece p = makePiece(pt, side_to_move);
    Square from_sq = from(move);
    Square to_sq = to(move);
    Piece capture = board[to_sq];
    const auto rank = square_rank(to_sq);

    hash_history.emplace_back(hash_key);

    if (en_passant_square != NO_SQ) hash_key ^= updateKeyEnPassant(en_passant_square);

    hash_key ^= updateKeyCastling();

    en_passant_square = NO_SQ;

    if (pt == KING) {
        castling_rights.clearCastlingRight(side_to_move);

        if (typeOf(move) == CASTLING) {
            const Piece rook = side_to_move == White ? WhiteRook : BlackRook;
            const Square rookSQ =
                file_rank_square(to_sq > from_sq ? FILE_F : FILE_D, square_rank(from_sq));
            const Square kingToSq =
                file_rank_square(to_sq > from_sq ? FILE_G : FILE_C, square_rank(from_sq));

            assert(type_of_piece(pieceAtB(to_sq)) == ROOK);

            hash_key ^= updateKeyPiece(rook, to_sq);
            hash_key ^= updateKeyPiece(rook, rookSQ);
            hash_key ^= updateKeyPiece(p, from_sq);
            hash_key ^= updateKeyPiece(p, kingToSq);

            hash_key ^= updateKeySideToMove();
            hash_key ^= updateKeyCastling();

            return;
        }
    } else if (pt == ROOK && ((square_rank(from_sq) == Rank::RANK_8 && side_to_move == Black) ||
                              (square_rank(from_sq) == Rank::RANK_1 && side_to_move == White))) {
        const auto king_sq = builtin::lsb(pieces(KING, side_to_move));

        castling_rights.clearCastlingRight(
            side_to_move, from_sq > king_sq ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE);
    } else if (pt == PAWN) {
        half_move_clock = 0;
        if (typeOf(move) == ENPASSANT) {
            hash_key ^= updateKeyPiece(makePiece(PAWN, ~side_to_move), Square(to_sq ^ 8));
        } else if (std::abs(from_sq - to_sq) == 16) {
            U64 epMask = attacks::Pawn(Square(to_sq ^ 8), side_to_move);
            if (epMask & pieces(PAWN, ~side_to_move)) {
                en_passant_square = Square(to_sq ^ 8);
                hash_key ^= updateKeyEnPassant(en_passant_square);

                assert(pieceAtB(en_passant_square) == None);
            }
        }
    }

    if (capture != None) {
        half_move_clock = 0;
        hash_key ^= updateKeyPiece(capture, to_sq);
        if (type_of_piece(capture) == ROOK && ((rank == Rank::RANK_1 && side_to_move == Black) ||
                                               (rank == Rank::RANK_8 && side_to_move == White))) {
            const auto king_sq = builtin::lsb(pieces(KING, ~side_to_move));

            castling_rights.clearCastlingRight(
                ~side_to_move, to_sq > king_sq ? CastleSide::KING_SIDE : CastleSide::QUEEN_SIDE);
        }
    }

    if (typeOf(move) == PROMOTION) {
        half_move_clock = 0;

        hash_key ^= updateKeyPiece(makePiece(PAWN, side_to_move), from_sq);
        hash_key ^= updateKeyPiece(makePiece(promotionType(move), side_to_move), to_sq);
    } else {
        hash_key ^= updateKeyPiece(p, from_sq);
        hash_key ^= updateKeyPiece(p, to_sq);
    }

    hash_key ^= updateKeySideToMove();
    hash_key ^= updateKeyCastling();
}

/// @brief
/// @tparam updateNNUE
/// @param move
template <bool updateNNUE>
void Board::makeMove(Move move) {
    PieceType pt = type_of_piece(pieceAtB(from(move)));
    Piece p = pieceAtB(from(move));
    Square from_sq = from(move);
    Square to_sq = to(move);
    Piece capture = board[to_sq];

    assert(from_sq >= 0 && from_sq < 64);
    assert(to_sq >= 0 && to_sq < 64);
    assert(type_of_piece(capture) != KING);
    assert(pt != NONETYPE);
    assert(p != None);
    assert((typeOf(move) == PROMOTION &&
            (promotionType(move) != PAWN && promotionType(move) != KING)) ||
           typeOf(move) != PROMOTION);

    // *****************************
    // STORE STATE HISTORY
    // *****************************

    state_history.emplace_back(en_passant_square, castling_rights, half_move_clock, capture);

    if constexpr (updateNNUE) accumulators_.push();

    half_move_clock++;
    full_move_number++;

    const bool ep = to_sq == en_passant_square;

    // Castling is encoded as king captures rook

    // *****************************
    // UPDATE HASH
    // *****************************

    updateHash(move);

    TTable.prefetch(hash_key);

    const Square ksq_white = builtin::lsb(pieces<KING, White>());
    const Square ksq_black = builtin::lsb(pieces<KING, Black>());

    // *****************************
    // UPDATE PIECES AND NNUE
    // *****************************

    if (typeOf(move) == CASTLING) {
        const Piece rook = side_to_move == White ? WhiteRook : BlackRook;
        Square rookToSq = file_rank_square(to_sq > from_sq ? FILE_F : FILE_D, square_rank(from_sq));
        Square kingToSq = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C, square_rank(from_sq));

        if (updateNNUE && nnue::KING_BUCKET[from_sq] != nnue::KING_BUCKET[kingToSq]) {
            removePiece<false>(p, from_sq, ksq_white, ksq_black);
            removePiece<false>(rook, to_sq, ksq_white, ksq_black);

            placePiece<false>(p, kingToSq, ksq_white, ksq_black);
            placePiece<false>(rook, rookToSq, ksq_white, ksq_black);

            refresh(getAccumulator());
        } else {
            removePiece<updateNNUE>(p, from_sq, ksq_white, ksq_black);
            removePiece<updateNNUE>(rook, to_sq, ksq_white, ksq_black);

            placePiece<updateNNUE>(p, kingToSq, ksq_white, ksq_black);
            placePiece<updateNNUE>(rook, rookToSq, ksq_white, ksq_black);
        }

        side_to_move = ~side_to_move;

        return;
    } else if (pt == PAWN && ep) {
        assert(pieceAtB(Square(to_sq ^ 8)) != None);

        removePiece<updateNNUE>(makePiece(PAWN, ~side_to_move), Square(to_sq ^ 8), ksq_white,
                                ksq_black);
    } else if (capture != None) {
        assert(pieceAtB(to_sq) != None);

        removePiece<updateNNUE>(capture, to_sq, ksq_white, ksq_black);
    }

    // The move is differently encoded for promotions to it requires some special care.
    if (typeOf(move) == PROMOTION) {
        assert(pieceAtB(to_sq) == None);

        removePiece<updateNNUE>(makePiece(PAWN, side_to_move), from_sq, ksq_white, ksq_black);
        placePiece<updateNNUE>(makePiece(promotionType(move), side_to_move), to_sq, ksq_white,
                               ksq_black);
    } else {
        assert(pieceAtB(to_sq) == None);

        movePiece<updateNNUE>(p, from_sq, to_sq, ksq_white, ksq_black);
    }

    side_to_move = ~side_to_move;
}

template <bool updateNNUE>
void Board::unmakeMove(Move move) {
    const State restore = state_history.back();
    state_history.pop_back();

    if (accumulators_.size()) {
        accumulators_.pop();
    }

    hash_key = hash_history.back();
    hash_history.pop_back();

    en_passant_square = restore.en_passant;
    castling_rights = restore.castling;
    half_move_clock = restore.half_move;
    Piece capture = restore.captured_piece;

    full_move_number--;

    Square from_sq = from(move);
    Square to_sq = to(move);
    bool promotion = typeOf(move) == PROMOTION;

    side_to_move = ~side_to_move;
    PieceType pt = type_of_piece(pieceAtB(to_sq));
    Piece p = makePiece(pt, side_to_move);

    if (typeOf(move) == CASTLING) {
        Square rookToSq = to_sq;
        Piece rook = side_to_move == White ? WhiteRook : BlackRook;
        Square rookFromSq =
            file_rank_square(to_sq > from_sq ? FILE_F : FILE_D, square_rank(from_sq));
        to_sq = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C, square_rank(from_sq));

        p = makePiece(KING, side_to_move);
        // We need to remove both pieces first and then place them back.
        removePiece<updateNNUE>(rook, rookFromSq);
        removePiece<updateNNUE>(p, to_sq);

        placePiece<updateNNUE>(p, from_sq);
        placePiece<updateNNUE>(rook, rookToSq);

        return;
    } else if (promotion) {
        removePiece<updateNNUE>(makePiece(promotionType(move), side_to_move), to_sq);
        placePiece<updateNNUE>(makePiece(PAWN, side_to_move), from_sq);
        if (capture != None) placePiece<updateNNUE>(capture, to_sq);

        return;
    } else {
        movePiece<updateNNUE>(p, to_sq, from_sq);
    }

    if (to_sq == en_passant_square && pt == PAWN) {
        placePiece<updateNNUE>(makePiece(PAWN, ~side_to_move), Square(en_passant_square ^ 8));
    } else if (capture != None) {
        placePiece<updateNNUE>(capture, to_sq);
    }
}

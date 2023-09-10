#include "board.h"
#include "movegen.h"
#include "str_utils.h"
#include "zobrist.h"

Board::Board(const std::string &fen) {
    state_history_.reserve(MAX_PLY);

    side_to_move_ = WHITE;
    en_passant_square_ = NO_SQ;
    castling_rights_.clearAllCastlingRights();
    half_move_clock_ = 0;
    plies_played_ = 0;

    board_.fill(NONE);

    setFen(fen, true);
}

Board::Board(const Board &other) {
    chess960 = other.chess960;

    if (other.accumulators_) {
        accumulators_ = std::make_unique<Accumulators>(*other.accumulators_);
    }

    state_history_ = other.state_history_;

    pieces_bb_ = other.pieces_bb_;
    board_ = other.board_;

    occupancy_bb_ = other.occupancy_bb_;

    hash_key_ = other.hash_key_;

    castling_rights_ = other.castling_rights_;

    plies_played_ = other.plies_played_;

    half_move_clock_ = other.half_move_clock_;

    side_to_move_ = other.side_to_move_;

    en_passant_square_ = other.en_passant_square_;
}

Board &Board::operator=(const Board &other) {
    if (this == &other) return *this;

    chess960 = other.chess960;

    if (other.accumulators_) {
        accumulators_ = std::make_unique<Accumulators>(*other.accumulators_);
    }

    state_history_ = other.state_history_;

    pieces_bb_ = other.pieces_bb_;
    board_ = other.board_;

    occupancy_bb_ = other.occupancy_bb_;

    hash_key_ = other.hash_key_;

    castling_rights_ = other.castling_rights_;

    plies_played_ = other.plies_played_;

    half_move_clock_ = other.half_move_clock_;

    side_to_move_ = other.side_to_move_;

    en_passant_square_ = other.en_passant_square_;

    return *this;
}

std::string Board::getCastleString() const {
    std::stringstream ss;

    if (chess960) {
        // loop to cleanup
        if (castling_rights_.hasCastlingRight(WHITE, CastleSide::KING_SIDE))
            ss << char(char(castling_rights_.getRookFile(WHITE, CastleSide::KING_SIDE)) + 65);
        if (castling_rights_.hasCastlingRight(WHITE, CastleSide::QUEEN_SIDE))
            ss << char(char(castling_rights_.getRookFile(WHITE, CastleSide::QUEEN_SIDE)) + 65);
        if (castling_rights_.hasCastlingRight(BLACK, CastleSide::KING_SIDE))
            ss << char(char(castling_rights_.getRookFile(BLACK, CastleSide::KING_SIDE)) + 97);
        if (castling_rights_.hasCastlingRight(BLACK, CastleSide::QUEEN_SIDE))
            ss << char(char(castling_rights_.getRookFile(BLACK, CastleSide::QUEEN_SIDE)) + 97);
    } else {
        if (castling_rights_.hasCastlingRight(WHITE, CastleSide::KING_SIDE)) ss << "K";
        if (castling_rights_.hasCastlingRight(WHITE, CastleSide::QUEEN_SIDE)) ss << "Q";
        if (castling_rights_.hasCastlingRight(BLACK, CastleSide::KING_SIDE)) ss << "k";
        if (castling_rights_.hasCastlingRight(BLACK, CastleSide::QUEEN_SIDE)) ss << "q";
    }

    return ss.str();
}

void Board::refreshNNUE(nnue::accumulator &acc) const {
    for (int i = 0; i < N_HIDDEN_SIZE; i++) {
        acc[WHITE][i] = HIDDEN_BIAS[i];
        acc[BLACK][i] = HIDDEN_BIAS[i];
    }

    const Square ksq_white = builtin::lsb(pieces<KING, WHITE>());
    const Square ksq_black = builtin::lsb(pieces<KING, BLACK>());

    for (Square i = SQ_A1; i < NO_SQ; i++) {
        Piece p = board_[i];
        if (p == NONE) continue;

        nnue::activate(acc, i, p, ksq_white, ksq_black);
    }
}

void Board::setFen(const std::string &fen, bool update_acc) {
    std::vector<std::string> params = str_util::splitString(fen, ' ');

    const std::string position = params[0];
    const std::string move_right = params[1];
    const std::string castling = params[2];
    const std::string en_passant = params[3];

    half_move_clock_ = std::stoi(params.size() > 4 ? params[4] : "0");
    plies_played_ = std::stoi(params.size() > 5 ? params[5] : "1") * 2 - 2;

    board_.fill(NONE);
    pieces_bb_.fill(0ULL);

    side_to_move_ = (move_right == "w") ? WHITE : BLACK;

    if (side_to_move_ == BLACK) {
        plies_played_++;
    }

    occupancy_bb_ = 0ULL;

    auto square = Square(56);
    for (char curr : position) {
        if (CHAR_TO_PIECE.find(curr) != CHAR_TO_PIECE.end()) {
            const Piece piece = CHAR_TO_PIECE[curr];
            placePiece<false>(piece, square);

            square = Square(square + 1);
        } else if (curr == '/')
            square = Square(square - 16);
        else if (isdigit(curr)) {
            square = Square(square + (curr - '0'));
        }
    }

    if (update_acc) {
        refreshNNUE(getAccumulator());
    }

    castling_rights_.clearAllCastlingRights();

    for (char i : castling) {
        if (!chess960) {
            if (i == 'K')
                castling_rights_.setCastlingRight<WHITE, CastleSide::KING_SIDE, File::FILE_H>();
            if (i == 'Q')
                castling_rights_.setCastlingRight<WHITE, CastleSide::QUEEN_SIDE, File::FILE_A>();
            if (i == 'k')
                castling_rights_.setCastlingRight<BLACK, CastleSide::KING_SIDE, File::FILE_H>();
            if (i == 'q')
                castling_rights_.setCastlingRight<BLACK, CastleSide::QUEEN_SIDE, File::FILE_A>();
        } else {
            const auto color = isupper(i) ? WHITE : BLACK;
            const auto king_sq = builtin::lsb(pieces(KING, color));
            const auto file = static_cast<File>(tolower(i) - 97);
            const auto side = int(file) > int(squareFile(king_sq)) ? CastleSide::KING_SIDE
                                                                   : CastleSide::QUEEN_SIDE;
            castling_rights_.setCastlingRight(color, side, file);
        }
    }

    if (en_passant == "-") {
        en_passant_square_ = NO_SQ;
    } else {
        char letter = en_passant[0];
        int file = letter - 96;
        int rank = en_passant[1] - 48;
        en_passant_square_ = Square((rank - 1) * 8 + file - 1);
    }

    state_history_.clear();
    accumulators_->clear();

    hash_key_ = zobrist();
}

std::string Board::getFen() const {
    std::stringstream ss;

    // Loop through the ranks of the board in reverse order
    for (int rank = 7; rank >= 0; rank--) {
        int free_space = 0;

        // Loop through the files of the board
        for (int file = 0; file < 8; file++) {
            // Calculate the square index
            int sq = rank * 8 + file;

            // Get the piece at the current square
            Piece piece = at(Square(sq));

            // If there is a piece at the current square
            if (piece != NONE) {
                // If there were any empty squares before this piece,
                // append the number of empty squares to the FEN string
                if (free_space) {
                    ss << free_space;
                    free_space = 0;
                }

                // Append the character representing the piece to the FEN string
                ss << PIECE_TO_CHAR[piece];
            } else {
                // If there is no piece at the current square, increment the
                // counter for the number of empty squares
                free_space++;
            }
        }

        // If there are any empty squares at the end of the rank,
        // append the number of empty squares to the FEN string
        if (free_space != 0) {
            ss << free_space;
        }

        // Append a "/" character to the FEN string, unless this is the last rank
        ss << (rank > 0 ? "/" : "");
    }

    // Append " w " or " b " to the FEN string, depending on which player's turn it is
    ss << (side_to_move_ == WHITE ? " w " : " b ");

    // Append the appropriate characters to the FEN string to indicate
    // whether or not castling is allowed for each player
    ss << getCastleString();
    if (castling_rights_.isEmpty()) ss << "-";

    // Append information about the en passant square (if any)
    // and the halfmove clock and fullmove number to the FEN string
    if (en_passant_square_ == NO_SQ)
        ss << " - ";
    else
        ss << " " << SQUARE_TO_STRING[en_passant_square_] << " ";

    ss << halfmoves() << " " << fullMoveNumber();

    // Return the resulting FEN string
    return ss.str();
}

bool Board::isRepetition(int draw) const {
    uint8_t c = 0;

    for (int i = static_cast<int>(state_history_.size()) - 2;
         i >= 0 && i >= static_cast<int>(state_history_.size()) - half_move_clock_ - 1; i -= 2) {
        if (state_history_[i].hash == hash_key_) c++;
        if (c == draw) return true;
    }

    return false;
}

Result Board::isDrawn(bool in_check) const {
    assert(kingSQ(WHITE) != NO_SQ && kingSQ(BLACK) != NO_SQ);

    if (half_move_clock_ >= 100) {
        Movelist movelist;
        movegen::legalmoves<Movetype::ALL>(*this, movelist);
        if (in_check && movelist.size == 0) return Result::LOST;
        return Result::DRAWN;
    }

    const auto count = builtin::popcount(all());

    if (count == 2) return Result::DRAWN;

    if (count == 3) {
        if (pieces(BISHOP)) return Result::DRAWN;
        if (pieces(KNIGHT)) return Result::DRAWN;
    }

    if (count == 4) {
        if (pieces(WHITEBISHOP) && pieces(BLACKBISHOP) &&
            sameColor(builtin::lsb(pieces(WHITEBISHOP)), builtin::lsb(pieces(BLACKBISHOP))))
            return Result::DRAWN;
    }

    return Result::NONE;
}

bool Board::nonPawnMat(Color c) const {
    return pieces(KNIGHT, c) | pieces(BISHOP, c) | pieces(ROOK, c) | pieces(QUEEN, c);
}

Square Board::kingSQ(Color c) const { return builtin::lsb(pieces(KING, c)); }

Bitboard Board::us(Color c) const {
    return pieces(PAWN, c) | pieces(KNIGHT, c) | pieces(BISHOP, c) | pieces(ROOK, c) |
           pieces(QUEEN, c) | pieces(KING, c);
}

Bitboard Board::all() const {
    assert((us<WHITE>() | us<BLACK>()) == occupancy_bb_);
    return occupancy_bb_;
}

Color Board::colorOf(Square loc) const { return Color((at(loc) / 6)); }

bool Board::isAttacked(Color c, Square sq, Bitboard occ) const {
    if (pieces(PAWN, c) & attacks::pawn(sq, ~c)) return true;
    if (pieces(KNIGHT, c) & attacks::knight(sq)) return true;
    if ((pieces(BISHOP, c) | pieces(QUEEN, c)) & attacks::bishop(sq, occ)) return true;
    if ((pieces(ROOK, c) | pieces(QUEEN, c)) & attacks::rook(sq, occ)) return true;
    if (pieces(KING, c) & attacks::king(sq)) return true;
    return false;
}

void Board::makeNullMove() {
    state_history_.emplace_back(hash_key_, castling_rights_, en_passant_square_, half_move_clock_,
                                NONE);
    // Update the hash key
    hash_key_ ^= zobrist::sideToMove();
    if (en_passant_square_ != NO_SQ)
        hash_key_ ^= zobrist::enpassant(squareFile(en_passant_square_));

    TTable.prefetch(hash_key_);

    en_passant_square_ = NO_SQ;

    plies_played_++;
    side_to_move_ = ~side_to_move_;
}

void Board::unmakeNullMove() {
    const State restore = state_history_.back();
    state_history_.pop_back();

    en_passant_square_ = restore.enpassant;

    hash_key_ ^= zobrist::sideToMove();
    if (en_passant_square_ != NO_SQ)
        hash_key_ ^= zobrist::enpassant(squareFile(en_passant_square_));

    castling_rights_ = restore.castling;
    half_move_clock_ = restore.half_moves;
    plies_played_--;
    side_to_move_ = ~side_to_move_;
}

void Board::clearStacks() {
    accumulators_->clear();
    state_history_.clear();
}

std::ostream &operator<<(std::ostream &os, const Board &b) {
    for (int i = 63; i >= 0; i -= 8) {
        os << " " << PIECE_TO_CHAR[b.board_[i - 7]] << " " << PIECE_TO_CHAR[b.board_[i - 6]] << " "
           << PIECE_TO_CHAR[b.board_[i - 5]] << " " << PIECE_TO_CHAR[b.board_[i - 4]] << " "
           << PIECE_TO_CHAR[b.board_[i - 3]] << " " << PIECE_TO_CHAR[b.board_[i - 2]] << " "
           << PIECE_TO_CHAR[b.board_[i - 1]] << " " << PIECE_TO_CHAR[b.board_[i]] << " \n";
    }
    os << "\n\n";
    os << "Fen: " << b.getFen() << "\n";
    os << "Side to move: " << static_cast<int>(b.side_to_move_) << "\n";
    os << "Castling: " << b.getCastleString() << "\n";
    os << "Halfmoves: " << b.halfmoves() << "\n";
    os << "Fullmoves: " << b.fullMoveNumber() << "\n";
    os << "EP: " << static_cast<int>(b.en_passant_square_) << "\n";
    os << "Hash: " << b.hash_key_ << "\n";
    os << "Chess960: " << b.chess960;

    os << std::endl;
    return os;
}

U64 Board::zobrist() const {
    U64 hash = 0ULL;
    U64 w_pieces = us<WHITE>();
    U64 b_pieces = us<BLACK>();

    // Piece hashes
    while (w_pieces) {
        Square sq = builtin::poplsb(w_pieces);
        hash ^= zobrist::piece(at(sq), sq);
    }

    while (b_pieces) {
        Square sq = builtin::poplsb(b_pieces);
        hash ^= zobrist::piece(at(sq), sq);
    }

    // Ep hash
    U64 ep_hash = 0ULL;

    if (en_passant_square_ != NO_SQ) {
        ep_hash = zobrist::enpassant(squareFile(en_passant_square_));
    }

    U64 turn_hash = side_to_move_ == WHITE ? zobrist::sideToMove() : 0;
    U64 cast_hash = zobrist::castling(castling_rights_.getHashIndex());

    return hash ^ cast_hash ^ turn_hash ^ ep_hash;
}

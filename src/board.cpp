#include "board.h"
#include "movegen.h"
#include "zobrist.h"
#include "str_utils.h"

Board::Board(std::string fen) {
    state_history_.reserve(MAX_PLY);
    hash_history.reserve(512);

    side_to_move = WHITE;
    en_passant_square = NO_SQ;
    castling_rights.clearAllCastlingRights();
    half_move_clock = 0;
    full_move_number = 1;

    board_.fill(NONE);

    setFen(fen, true);
}

Board::Board(const Board &other) {
    chess960 = other.chess960;
    en_passant_square = other.en_passant_square;
    castling_rights = other.castling_rights;
    half_move_clock = other.half_move_clock;
    full_move_number = other.full_move_number;

    side_to_move = other.side_to_move;

    hash_history = other.hash_history;

    hash_key = other.hash_key;

    state_history_ = other.state_history_;

    std::copy(std::begin(other.board_), std::end(other.board_), std::begin(board_));

    pieces_bb_ = other.pieces_bb_;

    if (other.accumulators_) {
        accumulators_ = std::make_unique<Accumulators>(*other.accumulators_);
    }
}

Board &Board::operator=(const Board &other) {
    if (this == &other) return *this;

    chess960 = other.chess960;
    en_passant_square = other.en_passant_square;
    castling_rights = other.castling_rights;
    half_move_clock = other.half_move_clock;
    full_move_number = other.full_move_number;

    side_to_move = other.side_to_move;

    hash_history = other.hash_history;

    hash_key = other.hash_key;

    state_history_ = other.state_history_;

    std::copy(std::begin(other.board_), std::end(other.board_), std::begin(board_));

    pieces_bb_ = other.pieces_bb_;

    if (other.accumulators_) {
        accumulators_ = std::make_unique<Accumulators>(*other.accumulators_);
    }

    return *this;
}

std::string Board::getCastleString() const {
    std::stringstream ss;

    if (chess960) {
        // loop to cleanup
        if (castling_rights.hasCastlingRight(WHITE, CastleSide::KING_SIDE))
            ss << char(char(castling_rights.getRookFile(WHITE, CastleSide::KING_SIDE)) + 65);
        if (castling_rights.hasCastlingRight(WHITE, CastleSide::QUEEN_SIDE))
            ss << char(char(castling_rights.getRookFile(WHITE, CastleSide::QUEEN_SIDE)) + 65);
        if (castling_rights.hasCastlingRight(BLACK, CastleSide::KING_SIDE))
            ss << char(char(castling_rights.getRookFile(BLACK, CastleSide::KING_SIDE)) + 97);
        if (castling_rights.hasCastlingRight(BLACK, CastleSide::QUEEN_SIDE))
            ss << char(char(castling_rights.getRookFile(BLACK, CastleSide::QUEEN_SIDE)) + 97);
    } else {
        if (castling_rights.hasCastlingRight(WHITE, CastleSide::KING_SIDE)) ss << "K";
        if (castling_rights.hasCastlingRight(WHITE, CastleSide::QUEEN_SIDE)) ss << "Q";
        if (castling_rights.hasCastlingRight(BLACK, CastleSide::KING_SIDE)) ss << "k";
        if (castling_rights.hasCastlingRight(BLACK, CastleSide::QUEEN_SIDE)) ss << "q";
    }

    return ss.str();
}

void Board::refresh() {
    for (int i = 0; i < N_HIDDEN_SIZE; i++) {
        getAccumulator()[WHITE][i] = HIDDEN_BIAS[i];
        getAccumulator()[BLACK][i] = HIDDEN_BIAS[i];
    }

    const Square ksq_white = builtin::lsb(pieces<KING, WHITE>());
    const Square ksq_black = builtin::lsb(pieces<KING, BLACK>());

    for (Square i = SQ_A1; i < NO_SQ; i++) {
        Piece p = board_[i];
        if (p == NONE) continue;

        nnue::activate(getAccumulator(), i, p, ksq_white, ksq_black);
    }
}

void Board::setFen(const std::string &fen, bool update_acc) {
    for (Piece p = WHITEPAWN; p < NONE; p++) {
        pieces_bb_[p] = 0ULL;
    }

    occupancy_bb_ = 0ULL;

    std::vector<std::string> params = str_util::splitString(fen, ' ');

    const std::string position = params[0];
    const std::string move_right = params[1];
    const std::string castling = params[2];
    const std::string en_passant = params[3];

    half_move_clock = std::stoi(params.size() > 4 ? params[4] : "0");
    full_move_number = std::stoi(params.size() > 4 ? params[5] : "1") * 2;

    side_to_move = (move_right == "w") ? WHITE : BLACK;

    board_.fill(NONE);

    Square square = Square(56);
    for (int index = 0; index < static_cast<int>(position.size()); index++) {
        char curr = position[index];
        if (charToPiece.find(curr) != charToPiece.end()) {
            const Piece piece = charToPiece[curr];
            placePiece<false>(piece, square);

            square = Square(square + 1);
        } else if (curr == '/')
            square = Square(square - 16);
        else if (isdigit(curr)) {
            square = Square(square + (curr - '0'));
        }
    }

    if (update_acc) {
        refresh();
    }

    castling_rights.clearAllCastlingRights();

    for (char i : castling) {
        if (!chess960) {
            if (i == 'K')
                castling_rights.setCastlingRight<WHITE, CastleSide::KING_SIDE, File::FILE_H>();
            if (i == 'Q')
                castling_rights.setCastlingRight<WHITE, CastleSide::QUEEN_SIDE, File::FILE_A>();
            if (i == 'k')
                castling_rights.setCastlingRight<BLACK, CastleSide::KING_SIDE, File::FILE_H>();
            if (i == 'q')
                castling_rights.setCastlingRight<BLACK, CastleSide::QUEEN_SIDE, File::FILE_A>();
        } else {
            const auto color = isupper(i) ? WHITE : BLACK;
            const auto king_sq = builtin::lsb(pieces(KING, color));
            const auto file = static_cast<File>(tolower(i) - 97);
            const auto side = int(file) > int(squareFile(king_sq)) ? CastleSide::KING_SIDE
                                                                   : CastleSide::QUEEN_SIDE;
            castling_rights.setCastlingRight(color, side, file);
        }
    }

    if (en_passant == "-") {
        en_passant_square = NO_SQ;
    } else {
        char letter = en_passant[0];
        int file = letter - 96;
        int rank = en_passant[1] - 48;
        en_passant_square = Square((rank - 1) * 8 + file - 1);
    }

    state_history_.clear();
    hash_history.clear();
    accumulators_->clear();

    hash_key = zobrist();
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
                ss << pieceToChar[piece];
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
    ss << (side_to_move == WHITE ? " w " : " b ");

    // Append the appropriate characters to the FEN string to indicate
    // whether or not castling is allowed for each player
    ss << getCastleString();
    if (castling_rights.isEmpty()) ss << "-";

    // Append information about the en passant square (if any)
    // and the halfmove clock and fullmove number to the FEN string
    if (en_passant_square == NO_SQ)
        ss << " - ";
    else
        ss << " " << squareToString[en_passant_square] << " ";

    ss << int(half_move_clock) << " " << int(full_move_number / 2);

    // Return the resulting FEN string
    return ss.str();
}

bool Board::isRepetition(int draw) const {
    uint8_t c = 0;

    for (int i = static_cast<int>(hash_history.size()) - 2;
         i >= 0 && i >= static_cast<int>(hash_history.size()) - half_move_clock - 1; i -= 2) {
        if (hash_history[i] == hash_key) c++;
        if (c == draw) return true;
    }

    return false;
}

Result Board::isDrawn(bool in_check) {
    if (half_move_clock >= 100) {
        Movelist movelist;
        movegen::legalmoves<Movetype::ALL>(*this, movelist);
        if (in_check && movelist.size == 0) return Result::LOST;
        return Result::DRAWN;
    }

    assert(kingSQ(WHITE) != NO_SQ && kingSQ(BLACK) != NO_SQ);

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
    if (pieces(PAWN, c) & attacks::Pawn(sq, ~c)) return true;
    if (pieces(KNIGHT, c) & attacks::Knight(sq)) return true;
    if ((pieces(BISHOP, c) | pieces(QUEEN, c)) & attacks::Bishop(sq, occ)) return true;
    if ((pieces(ROOK, c) | pieces(QUEEN, c)) & attacks::Rook(sq, occ)) return true;
    if (pieces(KING, c) & attacks::King(sq)) return true;
    return false;
}

void Board::makeNullMove() {
    state_history_.emplace_back(en_passant_square, castling_rights, half_move_clock, NONE);

    // Update the hash key
    hash_key ^= updateKeySideToMove();
    if (en_passant_square != NO_SQ) hash_key ^= updateKeyEnPassant(en_passant_square);

    TTable.prefetch(hash_key);

    side_to_move = ~side_to_move;

    // Set the en passant square to NO_SQ and increment the full move number
    en_passant_square = NO_SQ;
    full_move_number++;
}

void Board::unmakeNullMove() {
    const State restore = state_history_.back();
    state_history_.pop_back();

    en_passant_square = restore.en_passant;

    hash_key ^= updateKeySideToMove();
    if (en_passant_square != NO_SQ) hash_key ^= updateKeyEnPassant(en_passant_square);

    full_move_number--;
    side_to_move = ~side_to_move;
    castling_rights = restore.castling;
    half_move_clock = restore.half_move;
}

void Board::clearStacks() {
    accumulators_->clear();
    state_history_.clear();
}

std::ostream &operator<<(std::ostream &os, const Board &b) {
    for (int i = 63; i >= 0; i -= 8) {
        os << " " << pieceToChar[b.board_[i - 7]] << " " << pieceToChar[b.board_[i - 6]] << " "
           << pieceToChar[b.board_[i - 5]] << " " << pieceToChar[b.board_[i - 4]] << " "
           << pieceToChar[b.board_[i - 3]] << " " << pieceToChar[b.board_[i - 2]] << " "
           << pieceToChar[b.board_[i - 1]] << " " << pieceToChar[b.board_[i]] << " \n";
    }
    os << "\n\n";
    os << "Fen: " << b.getFen() << "\n";
    os << "Side to move: " << static_cast<int>(b.side_to_move) << "\n";
    os << "Castling: " << b.getCastleString() << "\n";
    os << "Halfmoves: " << static_cast<int>(b.half_move_clock) << "\n";
    os << "Fullmoves: " << static_cast<int>(b.full_move_number) / 2 << "\n";
    os << "EP: " << static_cast<int>(b.en_passant_square) << "\n";
    os << "Hash: " << b.hash_key << "\n";
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
        hash ^= updateKeyPiece(at(sq), sq);
    }

    while (b_pieces) {
        Square sq = builtin::poplsb(b_pieces);
        hash ^= updateKeyPiece(at(sq), sq);
    }

    // Ep hash
    U64 ep_hash = 0ULL;

    if (en_passant_square != NO_SQ) {
        ep_hash = updateKeyEnPassant(en_passant_square);
    }

    U64 turn_hash = side_to_move == WHITE ? RANDOM_ARRAY[780] : 0;
    U64 cast_hash = updateKeyCastling();

    return hash ^ cast_hash ^ turn_hash ^ ep_hash;
}

U64 Board::updateKeyPiece(Piece piece, Square sq) const {
    assert(piece < NONE);
    assert(sq < NO_SQ);
    return RANDOM_ARRAY[64 * hash_piece[piece] + sq];
}

U64 Board::updateKeyEnPassant(Square sq) const { return RANDOM_ARRAY[772 + squareFile(sq)]; }

U64 Board::updateKeyCastling() const { return castlingKey[castling_rights.getHashIndex()]; }

U64 Board::updateKeySideToMove() const { return RANDOM_ARRAY[780]; }

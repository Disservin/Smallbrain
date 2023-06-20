#include "board.h"
#include "movegen.h"
#include "zobrist.h"

Board::Board(std::string fen) {
    state_history.reserve(MAX_PLY);
    hash_history.reserve(512);

    side_to_move = White;
    en_passant_square = NO_SQ;
    castling_rights.clearAllCastlingRights();
    half_move_clock = 0;
    full_move_number = 1;

    std::fill(std::begin(board), std::end(board), None);

    applyFen(fen, true);
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

    state_history = other.state_history;

    std::copy(std::begin(other.board), std::end(other.board), std::begin(board));

    pieces_bb = other.pieces_bb;

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

    state_history = other.state_history;

    std::copy(std::begin(other.board), std::end(other.board), std::begin(board));

    pieces_bb = other.pieces_bb;

    if (other.accumulators_) {
        accumulators_ = std::make_unique<Accumulators>(*other.accumulators_);
    }

    return *this;
}

std::string Board::getCastleString() const {
    std::stringstream ss;

    if (chess960) {
        // loop to cleanup
        if (castling_rights.hasCastlingRight(White, CastleSide::KING_SIDE))
            ss << char(char(castling_rights.getRookFile(White, CastleSide::KING_SIDE)) + 65);
        if (castling_rights.hasCastlingRight(White, CastleSide::QUEEN_SIDE))
            ss << char(char(castling_rights.getRookFile(White, CastleSide::QUEEN_SIDE)) + 65);
        if (castling_rights.hasCastlingRight(Black, CastleSide::KING_SIDE))
            ss << char(char(castling_rights.getRookFile(Black, CastleSide::KING_SIDE)) + 97);
        if (castling_rights.hasCastlingRight(Black, CastleSide::QUEEN_SIDE))
            ss << char(char(castling_rights.getRookFile(Black, CastleSide::QUEEN_SIDE)) + 97);
    } else {
        if (castling_rights.hasCastlingRight(White, CastleSide::KING_SIDE)) ss << "K";
        if (castling_rights.hasCastlingRight(White, CastleSide::QUEEN_SIDE)) ss << "Q";
        if (castling_rights.hasCastlingRight(Black, CastleSide::KING_SIDE)) ss << "k";
        if (castling_rights.hasCastlingRight(Black, CastleSide::QUEEN_SIDE)) ss << "q";
    }

    return ss.str();
}

void Board::refresh() {
    for (int i = 0; i < N_HIDDEN_SIZE; i++) {
        getAccumulator()[White][i] = HIDDEN_BIAS[i];
        getAccumulator()[Black][i] = HIDDEN_BIAS[i];
    }

    const Square ksq_white = builtin::lsb(pieces<KING, White>());
    const Square ksq_black = builtin::lsb(pieces<KING, Black>());

    for (Square i = SQ_A1; i < NO_SQ; i++) {
        Piece p = board[i];
        if (p == None) continue;
        nnue::activate(getAccumulator(), i, p, ksq_white, ksq_black);
    }
}

Piece Board::pieceAtB(Square sq) const { return board[sq]; }

void Board::applyFen(const std::string &fen, bool update_acc) {
    for (Piece p = WhitePawn; p < None; p++) {
        pieces_bb[p] = 0ULL;
    }

    std::vector<std::string> params = splitInput(fen);

    const std::string position = params[0];
    const std::string move_right = params[1];
    const std::string castling = params[2];
    const std::string en_passant = params[3];

    half_move_clock = std::stoi(params.size() > 4 ? params[4] : "0");
    full_move_number = std::stoi(params.size() > 4 ? params[5] : "1") * 2;

    side_to_move = (move_right == "w") ? White : Black;

    std::fill(std::begin(board), std::end(board), None);

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
                castling_rights.setCastlingRight<White, CastleSide::KING_SIDE, File::FILE_H>();
            if (i == 'Q')
                castling_rights.setCastlingRight<White, CastleSide::QUEEN_SIDE, File::FILE_A>();
            if (i == 'k')
                castling_rights.setCastlingRight<Black, CastleSide::KING_SIDE, File::FILE_H>();
            if (i == 'q')
                castling_rights.setCastlingRight<Black, CastleSide::QUEEN_SIDE, File::FILE_A>();
        } else {
            const auto color = isupper(i) ? White : Black;
            const auto king_sq = builtin::lsb(pieces(KING, color));
            const auto file = static_cast<File>(tolower(i) - 97);
            const auto side = int(file) > int(square_file(king_sq)) ? CastleSide::KING_SIDE
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

    state_history.clear();
    hash_history.clear();
    accumulators_->clear();

    hash_key = zobristHash();
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
            Piece piece = pieceAtB(Square(sq));

            // If there is a piece at the current square
            if (piece != None) {
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
    ss << (side_to_move == White ? " w " : " b ");

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

    const auto count = builtin::popcount(all());

    if (count == 2) return Result::DRAWN;

    if (count == 3) {
        if (pieces(WhiteBishop) || pieces(BlackBishop)) return Result::DRAWN;
        if (pieces(WhiteKnight) || pieces(BlackKnight)) return Result::DRAWN;
    }

    if (count == 4) {
        if (pieces(WhiteBishop) && pieces(BlackBishop) &&
            sameColor(builtin::lsb(pieces(WhiteBishop)), builtin::lsb(pieces(BlackBishop))))
            return Result::DRAWN;
    }

    return Result::NONE;
}

bool Board::nonPawnMat(Color c) const {
    return pieces(KNIGHT, c) | pieces(BISHOP, c) | pieces(ROOK, c) | pieces(QUEEN, c);
}

Square Board::kingSQ(Color c) const { return builtin::lsb(pieces(KING, c)); }

U64 Board::enemy(Color c) const { return us(~c); }

U64 Board::us(Color c) const {
    return pieces_bb[PAWN + c * 6] | pieces_bb[KNIGHT + c * 6] | pieces_bb[BISHOP + c * 6] |
           pieces_bb[ROOK + c * 6] | pieces_bb[QUEEN + c * 6] | pieces_bb[KING + c * 6];
}

U64 Board::all() const { return us<White>() | us<Black>(); }

Color Board::colorOf(Square loc) const { return Color((pieceAtB(loc) / 6)); }

bool Board::isSquareAttacked(Color c, Square sq, U64 occ) const {
    if (pieces(PAWN, c) & attacks::Pawn(sq, ~c)) return true;
    if (pieces(KNIGHT, c) & attacks::Knight(sq)) return true;
    if ((pieces(BISHOP, c) | pieces(QUEEN, c)) & attacks::Bishop(sq, occ)) return true;
    if ((pieces(ROOK, c) | pieces(QUEEN, c)) & attacks::Rook(sq, occ)) return true;
    if (pieces(KING, c) & attacks::King(sq)) return true;
    return false;
}

U64 Board::allAttackers(Square sq, U64 occupied_bb) const {
    return attackersForSide(White, sq, occupied_bb) | attackersForSide(Black, sq, occupied_bb);
}

U64 Board::attackersForSide(Color attacker_color, Square sq, U64 occupied_bb) const {
    U64 attackingBishops = pieces(BISHOP, attacker_color);
    U64 attackingRooks = pieces(ROOK, attacker_color);
    U64 attackingQueens = pieces(QUEEN, attacker_color);
    U64 attackingKnights = pieces(KNIGHT, attacker_color);
    U64 attackingKing = pieces(KING, attacker_color);
    U64 attackingPawns = pieces(PAWN, attacker_color);

    U64 interCardinalRays = attacks::Bishop(sq, occupied_bb);
    U64 cardinalRaysRays = attacks::Rook(sq, occupied_bb);

    U64 attackers = interCardinalRays & (attackingBishops | attackingQueens);
    attackers |= cardinalRaysRays & (attackingRooks | attackingQueens);
    attackers |= attacks::Knight(sq) & attackingKnights;
    attackers |= attacks::King(sq) & attackingKing;
    attackers |= attacks::Pawn(sq, ~attacker_color) & attackingPawns;
    return attackers;
}

void Board::makeNullMove() {
    state_history.emplace_back(en_passant_square, castling_rights, half_move_clock, None);
    side_to_move = ~side_to_move;

    // Update the hash key
    hash_key ^= updateKeySideToMove();
    if (en_passant_square != NO_SQ) hash_key ^= updateKeyEnPassant(en_passant_square);

    TTable.prefetch(hash_key);

    // Set the en passant square to NO_SQ and increment the full move number
    en_passant_square = NO_SQ;
    full_move_number++;
}

void Board::unmakeNullMove() {
    const State restore = state_history.back();
    state_history.pop_back();

    en_passant_square = restore.en_passant;
    castling_rights = restore.castling;
    half_move_clock = restore.half_move;

    hash_key ^= updateKeySideToMove();
    if (en_passant_square != NO_SQ) hash_key ^= updateKeyEnPassant(en_passant_square);

    full_move_number--;
    side_to_move = ~side_to_move;
}

U64 Board::attacksByPiece(PieceType pt, Square sq, Color c, U64 occ) {
    switch (pt) {
        case PAWN:
            return attacks::Pawn(sq, c);
        case KNIGHT:
            return attacks::Knight(sq);
        case BISHOP:
            return attacks::Bishop(sq, occ);
        case ROOK:
            return attacks::Rook(sq, occ);
        case QUEEN:
            return attacks::Queen(sq, occ);
        case KING:
            return attacks::King(sq);
        case NONETYPE:
            return 0ULL;
        default:
            return 0ULL;
    }
}

void Board::clearStacks() {
    accumulators_->clear();
    state_history.clear();
}

std::ostream &operator<<(std::ostream &os, const Board &b) {
    for (int i = 63; i >= 0; i -= 8) {
        os << " " << pieceToChar[b.board[i - 7]] << " " << pieceToChar[b.board[i - 6]] << " "
           << pieceToChar[b.board[i - 5]] << " " << pieceToChar[b.board[i - 4]] << " "
           << pieceToChar[b.board[i - 3]] << " " << pieceToChar[b.board[i - 2]] << " "
           << pieceToChar[b.board[i - 1]] << " " << pieceToChar[b.board[i]] << " \n";
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

U64 Board::zobristHash() const {
    U64 hash = 0ULL;
    U64 wPieces = us<White>();
    U64 bPieces = us<Black>();
    // Piece hashes
    while (wPieces) {
        Square sq = builtin::poplsb(wPieces);
        hash ^= updateKeyPiece(pieceAtB(sq), sq);
    }
    while (bPieces) {
        Square sq = builtin::poplsb(bPieces);
        hash ^= updateKeyPiece(pieceAtB(sq), sq);
    }
    // Ep hash
    U64 ep_hash = 0ULL;
    if (en_passant_square != NO_SQ) {
        ep_hash = updateKeyEnPassant(en_passant_square);
    }
    // Turn hash
    U64 turn_hash = side_to_move == White ? RANDOM_ARRAY[780] : 0;
    // Castle hash
    U64 cast_hash = updateKeyCastling();

    return hash ^ cast_hash ^ turn_hash ^ ep_hash;
}

U64 Board::updateKeyPiece(Piece piece, Square sq) const {
    assert(piece < None);
    assert(sq < NO_SQ);
    return RANDOM_ARRAY[64 * hash_piece[piece] + sq];
}

U64 Board::updateKeyEnPassant(Square sq) const { return RANDOM_ARRAY[772 + square_file(sq)]; }

U64 Board::updateKeyCastling() const { return castlingKey[castling_rights.getHashIndex()]; }

U64 Board::updateKeySideToMove() const { return RANDOM_ARRAY[780]; }

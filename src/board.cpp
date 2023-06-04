#include "board.h"
#include "movegen.h"
#include "zobrist.h"

Board::Board() {
    state_history.reserve(MAX_PLY);
    hash_history.reserve(512);
    accumulatorStack.reserve(MAX_PLY);

    side_to_move = White;
    en_passant_square = NO_SQ;
    castling_rights.clearAllCastlingRights();
    half_move_clock = 0;
    full_move_number = 1;

    applyFen(DEFAULT_POS, true);

    std::fill(std::begin(board), std::end(board), None);
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
        accumulator[White][i] = HIDDEN_BIAS[i];
        accumulator[Black][i] = HIDDEN_BIAS[i];
    }

    const Square kSQ_White = builtin::lsb(pieces<KING, White>());
    const Square kSQ_Black = builtin::lsb(pieces<KING, Black>());

    for (Square i = SQ_A1; i < NO_SQ; i++) {
        Piece p = board[i];
        if (p == None) continue;
        NNUE::activate(accumulator, i, p, kSQ_White, kSQ_Black);
    }
}

Piece Board::pieceAtB(Square sq) const { return board[sq]; }

void Board::applyFen(const std::string &fen, bool updateAcc) {
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

    if (updateAcc) {
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
    accumulatorStack.clear();

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

Result Board::isDrawn(bool inCheck) {
    if (half_move_clock >= 100) {
        Movelist movelist;
        Movegen::legalmoves<Movetype::ALL>(*this, movelist);
        if (inCheck && movelist.size == 0) return Result::LOST;
        return Result::DRAWN;
    }

    const auto count = builtin::popcount(All());

    if (count == 2) return Result::DRAWN;

    if (count == 3) {
        if (pieces<WhiteBishop>() || pieces<BlackBishop>()) return Result::DRAWN;
        if (pieces<WhiteKnight>() || pieces<BlackKnight>()) return Result::DRAWN;
    }

    if (count == 4) {
        if (pieces<WhiteBishop>() && pieces<BlackBishop>() &&
            sameColor(builtin::lsb(pieces<WhiteBishop>()), builtin::lsb(pieces<BlackBishop>())))
            return Result::DRAWN;
    }

    return Result::NONE;
}

bool Board::nonPawnMat(Color c) const {
    return pieces(KNIGHT, c) | pieces(BISHOP, c) | pieces(ROOK, c) | pieces(QUEEN, c);
}

Square Board::KingSQ(Color c) const { return builtin::lsb(pieces(KING, c)); }

U64 Board::Enemy(Color c) const { return Us(~c); }

U64 Board::Us(Color c) const {
    return pieces_bb[PAWN + c * 6] | pieces_bb[KNIGHT + c * 6] | pieces_bb[BISHOP + c * 6] |
           pieces_bb[ROOK + c * 6] | pieces_bb[QUEEN + c * 6] | pieces_bb[KING + c * 6];
}

U64 Board::All() const { return Us<White>() | Us<Black>(); }

Color Board::colorOf(Square loc) const { return Color((pieceAtB(loc) / 6)); }

bool Board::isSquareAttacked(Color c, Square sq, U64 occ) const {
    if (pieces(PAWN, c) & Attacks::Pawn(sq, ~c)) return true;
    if (pieces(KNIGHT, c) & Attacks::Knight(sq)) return true;
    if ((pieces(BISHOP, c) | pieces(QUEEN, c)) & Attacks::Bishop(sq, occ)) return true;
    if ((pieces(ROOK, c) | pieces(QUEEN, c)) & Attacks::Rook(sq, occ)) return true;
    if (pieces(KING, c) & Attacks::King(sq)) return true;
    return false;
}

U64 Board::allAttackers(Square sq, U64 occupiedBB) {
    return attackersForSide(White, sq, occupiedBB) | attackersForSide(Black, sq, occupiedBB);
}

U64 Board::attackersForSide(Color attackerColor, Square sq, U64 occupiedBB) {
    U64 attackingBishops = pieces(BISHOP, attackerColor);
    U64 attackingRooks = pieces(ROOK, attackerColor);
    U64 attackingQueens = pieces(QUEEN, attackerColor);
    U64 attackingKnights = pieces(KNIGHT, attackerColor);
    U64 attackingKing = pieces(KING, attackerColor);
    U64 attackingPawns = pieces(PAWN, attackerColor);

    U64 interCardinalRays = Attacks::Bishop(sq, occupiedBB);
    U64 cardinalRaysRays = Attacks::Rook(sq, occupiedBB);

    U64 attackers = interCardinalRays & (attackingBishops | attackingQueens);
    attackers |= cardinalRaysRays & (attackingRooks | attackingQueens);
    attackers |= Attacks::Knight(sq) & attackingKnights;
    attackers |= Attacks::King(sq) & attackingKing;
    attackers |= Attacks::Pawn(sq, ~attackerColor) & attackingPawns;
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

const NNUE::accumulator &Board::getAccumulator() const { return accumulator; }

U64 Board::attacksByPiece(PieceType pt, Square sq, Color c, U64 occ) {
    switch (pt) {
        case PAWN:
            return Attacks::Pawn(sq, c);
        case KNIGHT:
            return Attacks::Knight(sq);
        case BISHOP:
            return Attacks::Bishop(sq, occ);
        case ROOK:
            return Attacks::Rook(sq, occ);
        case QUEEN:
            return Attacks::Queen(sq, occ);
        case KING:
            return Attacks::King(sq);
        case NONETYPE:
            return 0ULL;
        default:
            return 0ULL;
    }
}

/********************
 * Static Exchange Evaluation, logical based on Weiss (https://github.com/TerjeKir/weiss) licensed
 *under GPL-3.0
 *******************/
bool Board::see(Move move, int threshold) {
    Square from_sq = from(move);
    Square to_sq = to(move);
    PieceType attacker = type_of_piece(pieceAtB(from_sq));
    PieceType victim = type_of_piece(pieceAtB(to_sq));
    int swap = pieceValuesDefault[victim] - threshold;
    if (swap < 0) return false;
    swap -= pieceValuesDefault[attacker];
    if (swap >= 0) return true;
    U64 occ = (All() ^ (1ULL << from_sq)) | (1ULL << to_sq);
    U64 attackers = allAttackers(to_sq, occ) & occ;

    U64 queens = pieces_bb[WhiteQueen] | pieces_bb[BlackQueen];

    U64 bishops = pieces_bb[WhiteBishop] | pieces_bb[BlackBishop] | queens;
    U64 rooks = pieces_bb[WhiteRook] | pieces_bb[BlackRook] | queens;

    Color sT = ~colorOf(from_sq);

    while (true) {
        attackers &= occ;
        U64 myAttackers = attackers & Us(sT);
        if (!myAttackers) break;

        int pt;
        for (pt = 0; pt <= 5; pt++) {
            if (myAttackers & (pieces_bb[pt] | pieces_bb[pt + 6])) break;
        }
        sT = ~sT;
        if ((swap = -swap - 1 - piece_values[MG][pt]) >= 0) {
            if (pt == KING && (attackers & Us(sT))) sT = ~sT;
            break;
        }

        occ ^= (1ULL << (builtin::lsb(myAttackers & (pieces_bb[pt] | pieces_bb[pt + 6]))));

        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= Attacks::Bishop(to_sq, occ) & bishops;
        if (pt == ROOK || pt == QUEEN) attackers |= Attacks::Rook(to_sq, occ) & rooks;
    }
    return sT != colorOf(from_sq);
}

void Board::clearStacks() {
    accumulatorStack.clear();
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
    U64 wPieces = Us<White>();
    U64 bPieces = Us<Black>();
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

/**
 * PRIVATE FUNCTIONS
 *
 */

U64 Board::updateKeyPiece(Piece piece, Square sq) const {
    assert(piece < None);
    assert(sq < NO_SQ);
    return RANDOM_ARRAY[64 * hash_piece[piece] + sq];
}

U64 Board::updateKeyEnPassant(Square sq) const { return RANDOM_ARRAY[772 + square_file(sq)]; }

U64 Board::updateKeyCastling() const { return castlingKey[castling_rights.getHashIndex()]; }

U64 Board::updateKeySideToMove() const { return RANDOM_ARRAY[780]; }

std::string uciMove(Move move, bool chess960) {
    std::stringstream ss;

    // Get the from and to squares
    Square from_sq = from(move);
    Square to_sq = to(move);

    // If the move is not a chess960 castling move and is a king moving more than one square,
    // update the to square to be the correct square for a regular castling move
    if (!chess960 && typeOf(move) == CASTLING) {
        to_sq = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C, square_rank(from_sq));
    }

    // Add the from and to squares to the string stream
    ss << squareToString[from_sq];
    ss << squareToString[to_sq];

    // If the move is a promotion, add the promoted piece to the string stream
    if (typeOf(move) == PROMOTION) {
        ss << PieceTypeToPromPiece[promotionType(move)];
    }

    return ss.str();
}

Square extractSquare(std::string_view squareStr) {
    char letter = squareStr[0];
    int file = letter - 96;
    int rank = squareStr[1] - 48;
    int index = (rank - 1) * 8 + file - 1;
    return Square(index);
}

Move convertUciToMove(const Board &board, const std::string &input) {
    Square source = extractSquare(input.substr(0, 2));
    Square target = extractSquare(input.substr(2, 2));
    PieceType piece = type_of_piece(board.pieceAtB(source));

    // convert to king captures rook
    if (!board.chess960 && piece == KING && square_distance(target, source) == 2) {
        target = file_rank_square(target > source ? FILE_H : FILE_A, square_rank(source));
    }

    if (piece == KING && square_distance(target, source) >= 2) {
        return make<Move::CASTLING>(source, target);
    }

    if (piece == PAWN && target == board.en_passant_square) {
        return make<Move::ENPASSANT>(source, target);
    }

    switch (input.length()) {
        case 4:
            return make(source, target);
        case 5:
            return make<Move::PROMOTION>(source, target, pieceToInt[input.at(4)]);
        default:
            std::cout << "FALSE INPUT" << std::endl;
            return make(NO_SQ, NO_SQ);
    }
}
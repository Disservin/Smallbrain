#include "probe.h"

#include "syzygy/Fathom/src/tbprobe.h"

#include "movegen.h"

namespace syzygy {
Score probeWDL(const Board& board) {
    U64 white = board.Us<White>();
    U64 black = board.Us<Black>();

    if (builtin::popcount(white | black) > (signed)TB_LARGEST) return VALUE_NONE;

    Square ep = board.en_passant_square <= 63 ? board.en_passant_square : Square(0);

    unsigned TBresult =
        tb_probe_wdl(white, black, board.pieces<WhiteKing>() | board.pieces<BlackKing>(),
                     board.pieces<WhiteQueen>() | board.pieces<BlackQueen>(),
                     board.pieces<WhiteRook>() | board.pieces<BlackRook>(),
                     board.pieces<WhiteBishop>() | board.pieces<BlackBishop>(),
                     board.pieces<WhiteKnight>() | board.pieces<BlackKnight>(),
                     board.pieces<WhitePawn>() | board.pieces<BlackPawn>(), board.half_move_clock,
                     board.castling_rights.hasCastlingRight(board.side_to_move), ep,
                     board.side_to_move == White);

    if (TBresult == TB_LOSS) {
        return VALUE_TB_LOSS;
    } else if (TBresult == TB_WIN) {
        return VALUE_TB_WIN;
    } else if (TBresult == TB_DRAW || TBresult == TB_BLESSED_LOSS || TBresult == TB_CURSED_WIN) {
        return Score(0);
    }
    return VALUE_NONE;
}

Move probeDTZ(const Board& board) {
    U64 white = board.Us<White>();
    U64 black = board.Us<Black>();
    if (builtin::popcount(white | black) > (signed)TB_LARGEST) return NO_MOVE;

    Square ep = board.en_passant_square <= 63 ? board.en_passant_square : Square(0);

    unsigned TBresult = tb_probe_root(
        white, black, board.pieces<WhiteKing>() | board.pieces<BlackKing>(),
        board.pieces<WhiteQueen>() | board.pieces<BlackQueen>(),
        board.pieces<WhiteRook>() | board.pieces<BlackRook>(),
        board.pieces<WhiteBishop>() | board.pieces<BlackBishop>(),
        board.pieces<WhiteKnight>() | board.pieces<BlackKnight>(),
        board.pieces<WhitePawn>() | board.pieces<BlackPawn>(), board.half_move_clock,
        board.castling_rights.hasCastlingRight(board.side_to_move), ep, board.side_to_move == White,
        NULL);  //  * - turn: true=white, false=black

    if (TBresult == TB_RESULT_FAILED || TBresult == TB_RESULT_CHECKMATE ||
        TBresult == TB_RESULT_STALEMATE)
        return NO_MOVE;

    int dtz = TB_GET_DTZ(TBresult);
    int wdl = TB_GET_WDL(TBresult);

    Score s = 0;

    if (wdl == TB_LOSS) {
        s = VALUE_TB_LOSS_IN_MAX_PLY;
    }
    if (wdl == TB_WIN) {
        s = VALUE_TB_WIN_IN_MAX_PLY;
    }
    if (wdl == TB_BLESSED_LOSS || wdl == TB_DRAW || wdl == TB_CURSED_WIN) {
        s = 0;
    }

    // 1 - queen, 2 - rook, 3 - bishop, 4 - knight.
    int promo = TB_GET_PROMOTES(TBresult);
    PieceType promoTranslation[] = {NONETYPE, QUEEN, ROOK, BISHOP, KNIGHT, NONETYPE};

    // gets the square from and square to for the move which should be played
    Square sqFrom = Square(TB_GET_FROM(TBresult));
    Square sqTo = Square(TB_GET_TO(TBresult));

    Movelist legalmoves;
    Movegen::legalmoves<Movetype::ALL>(board, legalmoves);

    for (auto ext : legalmoves) {
        const Move move = ext.move;
        if (from(move) == sqFrom && to(move) == sqTo) {
            if ((promoTranslation[promo] == NONETYPE && typeOf(move) != PROMOTION) ||
                (promo < 5 && promoTranslation[promo] == promotionType(move) &&
                 typeOf(move) == PROMOTION)) {
                uciOutput(s, static_cast<int>(dtz), 1, 1, 1, 0, " " + uciMove(move, board.chess960),
                          0);
                return move;
            }
        }
    }

    std::cout << " something went wrong playing dtz :" << promoTranslation[promo] << " : " << promo
              << " : " << std::endl;
    exit(0);

    return NO_MOVE;
}
}  // namespace syzygy
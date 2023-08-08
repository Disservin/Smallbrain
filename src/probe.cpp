#include "probe.h"

#include "syzygy/Fathom/src/tbprobe.h"

#include "builtin.h"
#include "movegen.h"


namespace syzygy {

Score probeWDL(const Board &board) {
    const Bitboard white = board.us<WHITE>();
    const Bitboard black = board.us<BLACK>();

    if (builtin::popcount(white | black) > (signed)TB_LARGEST) return VALUE_NONE;

    const Square ep = board.enPassant() <= 63 ? board.enPassant() : Square(0);

    unsigned TBresult =
        tb_probe_wdl(white, black, board.pieces(WHITEKING) | board.pieces(BLACKKING),
                     board.pieces(WHITEQUEEN) | board.pieces(BLACKQUEEN),
                     board.pieces(WHITEROOK) | board.pieces(BLACKROOK),
                     board.pieces(WHITEBISHOP) | board.pieces(BLACKBISHOP),
                     board.pieces(WHITEKNIGHT) | board.pieces(BLACKKNIGHT),
                     board.pieces(WHITEPAWN) | board.pieces(BLACKPAWN), board.halfmoves(),
                     board.castlingRights().hasCastlingRight(board.sideToMove()), ep,
                     board.sideToMove() == WHITE);

    if (TBresult == TB_LOSS) {
        return VALUE_TB_LOSS;
    } else if (TBresult == TB_WIN) {
        return VALUE_TB_WIN;
    } else if (TBresult == TB_DRAW || TBresult == TB_BLESSED_LOSS || TBresult == TB_CURSED_WIN) {
        return Score(0);
    }
    return VALUE_NONE;
}

std::pair<int, Move> probeDTZ(const Board &board) {
    const Bitboard white = board.us<WHITE>();
    const Bitboard black = board.us<BLACK>();
    if (builtin::popcount(white | black) > (signed)TB_LARGEST) return {TB_RESULT_FAILED, NO_MOVE};

    const Square ep = board.enPassant() <= 63 ? board.enPassant() : Square(0);

    unsigned TBresult =
        tb_probe_root(white, black, board.pieces(WHITEKING) | board.pieces(BLACKKING),
                      board.pieces(WHITEQUEEN) | board.pieces(BLACKQUEEN),
                      board.pieces(WHITEROOK) | board.pieces(BLACKROOK),
                      board.pieces(WHITEBISHOP) | board.pieces(BLACKBISHOP),
                      board.pieces(WHITEKNIGHT) | board.pieces(BLACKKNIGHT),
                      board.pieces(WHITEPAWN) | board.pieces(BLACKPAWN), board.halfmoves(),
                      board.castlingRights().hasCastlingRight(board.sideToMove()), ep,
                      board.sideToMove() == WHITE,
                      nullptr);  //  * - turn: true=white, false=black

    if (TBresult == TB_RESULT_FAILED || TBresult == TB_RESULT_CHECKMATE ||
        TBresult == TB_RESULT_STALEMATE)
        return {TB_RESULT_FAILED, NO_MOVE};

    const int wdl = TB_GET_WDL(TBresult);

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
    const int promo = TB_GET_PROMOTES(TBresult);
    const PieceType promoTranslation[] = {NONETYPE, QUEEN, ROOK, BISHOP, KNIGHT, NONETYPE};

    // gets the square from and square to for the move which should be played
    const auto sqFrom = Square(TB_GET_FROM(TBresult));
    const auto sqTo = Square(TB_GET_TO(TBresult));

    Movelist legalmoves;
    movegen::legalmoves<Movetype::ALL>(board, legalmoves);

    for (auto ext : legalmoves) {
        const Move move = ext.move;
        if (from(move) == sqFrom && to(move) == sqTo) {
            if ((promoTranslation[promo] == NONETYPE && typeOf(move) != PROMOTION) ||
                (promo < 5 && promoTranslation[promo] == promotionType(move) &&
                 typeOf(move) == PROMOTION)) {
                return {s, move};
            }
        }
    }

    std::cout << " something went wrong playing dtz :" << promoTranslation[promo] << " : " << promo
              << " : " << std::endl;
    std::exit(0);
}
}  // namespace syzygy
#include "movegen.h"

namespace Movegen
{

template <Color c> U64 DoCheckmask(Board &board, Square sq)
{
    U64 Occ = board.occAll;
    U64 checks = 0ULL;
    U64 pawn_mask = board.Pawns<~c>() & PawnAttacks(sq, c);
    U64 knight_mask = board.Knights<~c>() & KnightAttacks(sq);
    U64 bishop_mask = (board.Bishops<~c>() | board.Queens<~c>()) & BishopAttacks(sq, Occ);
    U64 rook_mask = (board.Rooks<~c>() | board.Queens<~c>()) & RookAttacks(sq, Occ);
    board.doubleCheck = 0;
    if (pawn_mask)
    {
        checks |= pawn_mask;
        board.doubleCheck++;
    }
    if (knight_mask)
    {
        checks |= knight_mask;
        board.doubleCheck++;
    }
    if (bishop_mask)
    {
        if (popcount(bishop_mask) > 1)
            board.doubleCheck++;

        int8_t index = bsf(bishop_mask);
        checks |= board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        board.doubleCheck++;
    }
    if (rook_mask)
    {
        if (popcount(rook_mask) > 1)
            board.doubleCheck++;

        int8_t index = bsf(rook_mask);
        checks |= board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index);
        board.doubleCheck++;
    }
    return checks;
}

template <Color c> void DoPinMask(Board &board, Square sq)
{
    U64 them = board.occEnemy;
    U64 rook_mask = (board.Rooks<~c>() | board.Queens<~c>()) & RookAttacks(sq, them);
    U64 bishop_mask = (board.Bishops<~c>() | board.Queens<~c>()) & BishopAttacks(sq, them);

    board.pinD = 0ULL;
    board.pinHV = 0ULL;
    while (rook_mask)
    {
        Square index = poplsb(rook_mask);
        U64 possible_pin = (board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & board.occUs) == 1)
            board.pinHV |= possible_pin;
    }
    while (bishop_mask)
    {
        Square index = poplsb(bishop_mask);
        U64 possible_pin = (board.SQUARES_BETWEEN_BB[sq][index] | (1ULL << index));
        if (popcount(possible_pin & board.occUs) == 1)
            board.pinD |= possible_pin;
    }
}

template <Color c> void seenSquares(Board &board)
{
    U64 pawns = board.Pawns<c>();
    U64 knights = board.Knights<c>();
    U64 bishops = board.Bishops<c>();
    U64 rooks = board.Rooks<c>();
    U64 queens = board.Queens<c>();
    U64 kings = board.Kings<c>();

    board.seen = 0ULL;
    Square kSq = board.KingSQ(~c);
    board.occAll &= ~(1ULL << kSq);

    if (c == White)
    {
        board.seen |= pawns << 9 & ~MASK_FILE[0];
        board.seen |= pawns << 7 & ~MASK_FILE[7];
    }
    else
    {
        board.seen |= pawns >> 7 & ~MASK_FILE[0];
        board.seen |= pawns >> 9 & ~MASK_FILE[7];
    }
    while (knights)
    {
        Square index = poplsb(knights);
        board.seen |= KnightAttacks(index);
    }
    while (bishops)
    {
        Square index = poplsb(bishops);
        board.seen |= BishopAttacks(index, board.occAll);
    }
    while (rooks)
    {
        Square index = poplsb(rooks);
        board.seen |= RookAttacks(index, board.occAll);
    }
    while (queens)
    {
        Square index = poplsb(queens);
        board.seen |= QueenAttacks(index, board.occAll);
    }
    while (kings)
    {
        Square index = poplsb(kings);
        board.seen |= KingAttacks(index);
    }

    board.occAll |= (1ULL << kSq);
}

template <Color c> void init(Board &board, Square sq)
{
    board.occUs = board.Us<c>();
    board.occEnemy = board.Us<~c>();
    board.occAll = board.occUs | board.occEnemy;
    board.enemyEmptyBB = ~board.occUs;
    seenSquares<~c>(board);
    U64 newMask = DoCheckmask<c>(board, sq);
    board.checkMask = newMask ? newMask : DEFAULT_CHECKMASK;
    DoPinMask<c>(board, sq);
}

template <Color c> U64 PawnPushSingle(U64 occAll, Square sq)
{
    if (c == White)
    {
        return ((1ULL << sq) << 8) & ~occAll;
    }
    else
    {
        return ((1ULL << sq) >> 8) & ~occAll;
    }
}

template <Color c> U64 PawnPushBoth(U64 occAll, Square sq)
{
    U64 push = (1ULL << sq);
    if (c == White)
    {
        push = (push << 8) & ~occAll;
        return square_rank(sq) == 1 ? push | ((push << 8) & ~occAll) : push;
    }
    else
    {
        push = (push >> 8) & ~occAll;
        return square_rank(sq) == 6 ? push | ((push >> 8) & ~occAll) : push;
    }
}

template <Color c> U64 LegalPawnMoves(const Board &board, Square sq)
{
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the board.checkMask
    if (board.pinD & (1ULL << sq))
        return PawnAttacks(sq, c) & board.pinD & board.checkMask & board.occEnemy;

    // If we are pinned horizontally we can do no moves but if we are pinned vertically we can only do pawn pushs
    if (board.pinHV & (1ULL << sq))
        return PawnPushBoth<c>(board.occAll, sq) & board.pinHV & board.checkMask;
    return ((PawnAttacks(sq, c) & board.occEnemy) | PawnPushBoth<c>(board.occAll, sq)) & board.checkMask;
}

template <Color c> U64 LegalPawnMovesEP(Board &board, Square sq, Square ep)
{
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the board.checkMask
    if (board.pinD & (1ULL << sq))
        return PawnAttacks(sq, c) & board.pinD & board.checkMask & (board.occEnemy | (1ULL << ep));

    // Calculate pawn pushs

    // If we are pinned horizontally we can do no moves but if we are pinned vertically we can only do pawn pushs
    if (board.pinHV & (1ULL << sq))
        return PawnPushBoth<c>(board.occAll, sq) & board.pinHV & board.checkMask;
    U64 attacks = PawnAttacks(sq, c);
    if (board.checkMask != DEFAULT_CHECKMASK)
    {
        // If we are in check and the en passant square lies on our attackmask and the en passant piece gives check
        // return the ep mask as a move square
        if (attacks & (1ULL << ep) && board.checkMask & (1ULL << (ep - (c * -2 + 1) * 8)))
            return 1ULL << ep;
        // If we are in check we can do all moves that are on the board.checkMask
        return ((attacks & board.occEnemy) | PawnPushBoth<c>(board.occAll, sq)) & board.checkMask;
    }

    U64 moves = ((attacks & board.occEnemy) | PawnPushBoth<c>(board.occAll, sq)) & board.checkMask;
    // We need to make extra sure that ep moves dont leave the king in check
    // 7k/8/8/K1Pp3r/8/8/8/8 w - d6 0 1
    // Horizontal rook pins our pawn through another pawn, our pawn can push but not take enpassant
    // remove both the pawn that made the push and our pawn that could take in theory and check if that would give check
    if ((1ULL << ep) & attacks)
    {
        Square tP = c == White ? Square((int)ep - 8) : Square((int)ep + 8);
        Square kSQ = board.KingSQ(c);
        U64 enemyQueenRook = board.Rooks(~c) | board.Queens(~c);
        if ((enemyQueenRook)&MASK_RANK[square_rank(kSQ)])
        {
            Piece ourPawn = board.makePiece(PAWN, c);
            Piece theirPawn = board.makePiece(PAWN, ~c);
            board.removePiece<false>(ourPawn, sq);
            board.removePiece<false>(theirPawn, tP);
            board.placePiece<false>(ourPawn, ep);
            if (!((RookAttacks(kSQ, board.All()) & (enemyQueenRook))))
                moves |= (1ULL << ep);
            board.placePiece<false>(ourPawn, sq);
            board.placePiece<false>(theirPawn, tP);
            board.removePiece<false>(ourPawn, ep);
        }
        else
        {
            moves |= (1ULL << ep);
        }
    }
    return moves;
}

U64 LegalKnightMoves(const Board &board, Square sq)
{
    if (board.pinD & (1ULL << sq) || board.pinHV & (1ULL << sq))
        return 0ULL;
    return KnightAttacks(sq) & board.enemyEmptyBB & board.checkMask;
}

U64 LegalBishopMoves(const Board &board, Square sq)
{
    if (board.pinHV & (1ULL << sq))
        return 0ULL;
    if (board.pinD & (1ULL << sq))
        return BishopAttacks(sq, board.occAll) & board.enemyEmptyBB & board.pinD & board.checkMask;
    return BishopAttacks(sq, board.occAll) & board.enemyEmptyBB & board.checkMask;
}

U64 LegalRookMoves(const Board &board, Square sq)
{
    if (board.pinD & (1ULL << sq))
        return 0ULL;
    if (board.pinHV & (1ULL << sq))
        return RookAttacks(sq, board.occAll) & board.enemyEmptyBB & board.pinHV & board.checkMask;
    return RookAttacks(sq, board.occAll) & board.enemyEmptyBB & board.checkMask;
}

U64 LegalQueenMoves(const Board &board, Square sq)
{
    return LegalRookMoves(board, sq) | LegalBishopMoves(board, sq);
}

U64 LegalKingMoves(const Board &board, Square sq)
{
    return KingAttacks(sq) & board.enemyEmptyBB & ~board.seen;
}

template <Color c> U64 LegalKingMovesCastling(const Board &board, Square sq)
{
    U64 moves = KingAttacks(sq) & board.enemyEmptyBB & ~board.seen;

    if (c == White)
    {
        if (board.castlingRights & wk && !(WK_CASTLE_MASK & board.occAll) && moves & (1ULL << SQ_F1) &&
            !(board.seen & (1ULL << SQ_G1)))
        {
            moves |= (1ULL << SQ_G1);
        }
        if (board.castlingRights & wq && !(WQ_CASTLE_MASK & board.occAll) && moves & (1ULL << SQ_D1) &&
            !(board.seen & (1ULL << SQ_C1)))
        {
            moves |= (1ULL << SQ_C1);
        }
    }
    else
    {
        if (board.castlingRights & bk && !(BK_CASTLE_MASK & board.occAll) && moves & (1ULL << SQ_F8) &&
            !(board.seen & (1ULL << SQ_G8)))
        {
            moves |= (1ULL << SQ_G8);
        }
        if (board.castlingRights & bq && !(BQ_CASTLE_MASK & board.occAll) && moves & (1ULL << SQ_D8) &&
            !(board.seen & (1ULL << SQ_C8)))
        {
            moves |= (1ULL << SQ_C8);
        }
    }
    return moves;
}

template <Color c> U64 LegalPawnCaptures(Board &board, Square sq, Square ep)
{
    U64 enemy = board.occEnemy;
    // If we are pinned diagonally we can only do captures which are on the pin_dg and on the board.checkMask
    if (board.pinD & (1ULL << sq))
        return PawnAttacks(sq, c) & board.pinD & board.checkMask & (enemy | (1ULL << ep));
    // If we are pinned horizontally/vertically we can do no captures
    if (board.pinHV & (1ULL << sq))
        return 0ULL;
    U64 attacks = PawnAttacks(sq, c);
    U64 pawnPromote = 0ULL;
    if ((square_rank(sq) == 1 && c == Black) || (square_rank(sq) == 6 && c == White))
    {
        pawnPromote = PawnPushSingle<c>(board.occAll, sq) & ~board.occAll;
    }
    return ((attacks & enemy) | pawnPromote) & board.checkMask;
}

U64 LegalKnightCaptures(const Board &board, Square sq)
{
    if (board.pinD & (1ULL << sq) || board.pinHV & (1ULL << sq))
        return 0ULL;
    return KnightAttacks(sq) & board.occEnemy & board.checkMask;
}

U64 LegalBishopCaptures(const Board &board, Square sq)
{
    if (board.pinHV & (1ULL << sq))
        return 0ULL;
    if (board.pinD & (1ULL << sq))
        return BishopAttacks(sq, board.occAll) & board.occEnemy & board.pinD & board.checkMask;
    return BishopAttacks(sq, board.occAll) & board.occEnemy & board.checkMask;
}

U64 LegalRookCaptures(const Board &board, Square sq)
{
    if (board.pinD & (1ULL << sq))
        return 0ULL;
    if (board.pinHV & (1ULL << sq))
        return RookAttacks(sq, board.occAll) & board.occEnemy & board.pinHV & board.checkMask;
    return RookAttacks(sq, board.occAll) & board.occEnemy & board.checkMask;
}

U64 LegalQueenCaptures(const Board &board, Square sq)
{
    return LegalRookCaptures(board, sq) | LegalBishopCaptures(board, sq);
}

U64 LegalKingCaptures(const Board &board, Square sq)
{
    return KingAttacks(sq) & board.occEnemy & ~board.seen;
}

template <Color c> Movelist legalmoves(Board &board)
{
    Movelist movelist{};
    movelist.size = 0;

    init<c>(board, board.KingSQ(c));
    if (board.doubleCheck < 2)
    {
        U64 pawns_mask = board.Pawns<c>();
        U64 knights_mask = board.Knights<c>();
        U64 bishops_mask = board.Bishops<c>();
        U64 rooks_mask = board.Rooks<c>();
        U64 queens_mask = board.Queens<c>();

        const bool noEP = board.enPassantSquare == NO_SQ;

        while (pawns_mask)
        {
            Square from = poplsb(pawns_mask);
            U64 moves = noEP ? LegalPawnMoves<c>(board, from) : LegalPawnMovesEP<c>(board, from, board.enPassantSquare);
            while (moves)
            {
                Square to = poplsb(moves);
                if (square_rank(to) == 7 || square_rank(to) == 0)
                {
                    movelist.Add(make(QUEEN, from, to, true));
                    movelist.Add(make(ROOK, from, to, true));
                    movelist.Add(make(KNIGHT, from, to, true));
                    movelist.Add(make(BISHOP, from, to, true));
                }
                else
                {
                    movelist.Add(make(PAWN, from, to, false));
                }
            }
        }
        while (knights_mask)
        {
            Square from = poplsb(knights_mask);
            U64 moves = LegalKnightMoves(board, from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(KNIGHT, from, to, false));
            }
        }
        while (bishops_mask)
        {
            Square from = poplsb(bishops_mask);
            U64 moves = LegalBishopMoves(board, from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(BISHOP, from, to, false));
            }
        }
        while (rooks_mask)
        {
            Square from = poplsb(rooks_mask);
            U64 moves = LegalRookMoves(board, from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(ROOK, from, to, false));
            }
        }
        while (queens_mask)
        {
            Square from = poplsb(queens_mask);
            U64 moves = LegalQueenMoves(board, from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(QUEEN, from, to, false));
            }
        }
    }

    Square from = board.KingSQ(c);
    U64 moves = !board.castlingRights || board.checkMask != DEFAULT_CHECKMASK ? LegalKingMoves(board, from)
                                                                              : LegalKingMovesCastling<c>(board, from);
    while (moves)
    {
        Square to = poplsb(moves);
        movelist.Add(make(KING, from, to, false));
    }
    return movelist;
}

template <Color c> Movelist capturemoves(Board &board)
{
    Movelist movelist{};
    movelist.size = 0;

    init<c>(board, board.KingSQ(c));
    if (board.doubleCheck < 2)
    {
        U64 pawns_mask = board.Pawns<c>();
        U64 knights_mask = board.Knights<c>();
        U64 bishops_mask = board.Bishops<c>();
        U64 rooks_mask = board.Rooks<c>();
        U64 queens_mask = board.Queens<c>();
        while (pawns_mask)
        {
            Square from = poplsb(pawns_mask);
            U64 moves = LegalPawnCaptures<c>(board, from, board.enPassantSquare);
            while (moves)
            {
                Square to = poplsb(moves);
                if (square_rank(to) == 7 || square_rank(to) == 0)
                {
                    movelist.Add(make(QUEEN, from, to, true));
                    movelist.Add(make(ROOK, from, to, true));
                    movelist.Add(make(KNIGHT, from, to, true));
                    movelist.Add(make(BISHOP, from, to, true));
                }
                else
                {
                    movelist.Add(make(PAWN, from, to, false));
                }
            }
        }
        while (knights_mask)
        {
            Square from = poplsb(knights_mask);
            U64 moves = LegalKnightCaptures(board, from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(KNIGHT, from, to, false));
            }
        }
        while (bishops_mask)
        {
            Square from = poplsb(bishops_mask);
            U64 moves = LegalBishopCaptures(board, from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(BISHOP, from, to, false));
            }
        }
        while (rooks_mask)
        {
            Square from = poplsb(rooks_mask);
            U64 moves = LegalRookCaptures(board, from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(ROOK, from, to, false));
            }
        }
        while (queens_mask)
        {
            Square from = poplsb(queens_mask);
            U64 moves = LegalQueenCaptures(board, from);
            while (moves)
            {
                Square to = poplsb(moves);
                movelist.Add(make(QUEEN, from, to, false));
            }
        }
    }
    Square from = board.KingSQ(c);
    U64 moves = LegalKingCaptures(board, from);
    while (moves)
    {
        Square to = poplsb(moves);
        movelist.Add(make(KING, from, to, false));
    }
    return movelist;
}

template <Color c> int pseudoLegalMovesNumber(Board &board)
{
    int total = 0;
    U64 pawns_mask = board.Pawns<c>();
    U64 knights_mask = board.Knights<c>();
    U64 bishops_mask = board.Bishops<c>();
    U64 rooks_mask = board.Rooks<c>();
    U64 queens_mask = board.Queens<c>();
    while (pawns_mask)
    {
        Square from = poplsb(pawns_mask);
        total += popcount(PawnPushBoth<c>(board.occAll, from) | PawnAttacks(c, c));
    }
    while (knights_mask)
    {
        Square from = poplsb(knights_mask);
        total += popcount(KnightAttacks(from) & board.enemyEmptyBB);
    }
    while (bishops_mask)
    {
        Square from = poplsb(bishops_mask);
        total += popcount(BishopAttacks(from, board.occAll) & board.enemyEmptyBB);
    }
    while (rooks_mask)
    {
        Square from = poplsb(rooks_mask);
        total += popcount(RookAttacks(from, board.occAll) & board.enemyEmptyBB);
    }
    while (queens_mask)
    {
        Square from = poplsb(queens_mask);
        total += popcount(QueenAttacks(from, board.occAll) & board.enemyEmptyBB);
    }
    Square from = board.KingSQ(c);
    total += popcount(KingAttacks(from) & board.enemyEmptyBB);
    return total;
}

template <Color c> bool hasLegalMoves(Board &board)
{
    init<c>(board, board.KingSQ(c));
    if (board.doubleCheck < 2)
    {
        U64 pawns_mask = board.Pawns<c>();
        U64 knights_mask = board.Knights<c>();
        U64 bishops_mask = board.Bishops<c>();
        U64 rooks_mask = board.Rooks<c>();
        U64 queens_mask = board.Queens<c>();

        const bool noEP = board.enPassantSquare == NO_SQ;

        while (pawns_mask)
        {
            Square from = poplsb(pawns_mask);
            U64 moves = noEP ? LegalPawnMoves<c>(board, from) : LegalPawnMovesEP<c>(board, from, board.enPassantSquare);
            while (moves)
            {
                return true;
            }
        }
        while (knights_mask)
        {
            Square from = poplsb(knights_mask);
            U64 moves = LegalKnightMoves(board, from);
            while (moves)
            {
                return true;
            }
        }
        while (bishops_mask)
        {
            Square from = poplsb(bishops_mask);
            U64 moves = LegalBishopMoves(board, from);
            while (moves)
            {
                return true;
            }
        }
        while (rooks_mask)
        {
            Square from = poplsb(rooks_mask);
            U64 moves = LegalRookMoves(board, from);
            while (moves)
            {
                return true;
            }
        }
        while (queens_mask)
        {
            Square from = poplsb(queens_mask);
            U64 moves = LegalQueenMoves(board, from);
            while (moves)
            {
                return true;
            }
        }
    }

    Square from = board.KingSQ(c);
    U64 moves = !board.castlingRights || board.checkMask != DEFAULT_CHECKMASK ? LegalKingMoves(board, from)
                                                                              : LegalKingMovesCastling<c>(board, from);
    while (moves)
    {
        return true;
    }
    return false;
}

Movelist legalmoves(Board &board)
{
    if (board.sideToMove == White)
        return legalmoves<White>(board);
    else
        return legalmoves<Black>(board);
}

Movelist capturemoves(Board &board)
{
    if (board.sideToMove == White)
        return capturemoves<White>(board);
    else
        return capturemoves<Black>(board);
}

bool hasLegalMoves(Board &board)
{
    if (board.sideToMove == White)
        return hasLegalMoves<White>(board);
    else
        return hasLegalMoves<Black>(board);
}
} // namespace Movegen
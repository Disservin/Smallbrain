#pragma once
#include "board.h"
#include "search.h"
#include "types.h"

enum SearchType
{
    QSEARCH,
    ABSEARCH
};

template <SearchType st> class MovePick
{
  public:
    MovePick(ThreadData *t, Stack *s, Movelist &moves, const Move move);

    Move nextMove(const bool inCheck);

  private:
    ThreadData *td;
    Stack *ss;
    Movelist &movelist;
    Move ttMove;

    Staging stage;

    int played = 0;
    bool playedTT = false;

    Move orderNext();

    int mvvlva(const Move move);
    int scoreMove(const Move move);
};

template <SearchType st>
MovePick<st>::MovePick(ThreadData *t, Stack *s, Movelist &moves, const Move move)
    : td(t), ss(s), movelist(moves), ttMove(move)
{
    stage = TT_MOVE;
    movelist.size = 0;
    played = 0;
}

template <SearchType st> Move MovePick<st>::orderNext()
{
    int index = played;
    for (int i = 1 + played; i < movelist.size; i++)
    {
        if (movelist[i] > movelist[index])
            index = i;
    }
    std::swap(movelist[index], movelist[played]);

    return movelist[played++].move;
}

template <SearchType st> Move MovePick<st>::nextMove(const bool inCheck)
{
    switch (stage)
    {
    case TT_MOVE:
        stage++;

        if (td->board.isPseudoLegal(ttMove) && td->board.isLegal(ttMove))
        {
            playedTT = true;
            return ttMove;
        }

        [[fallthrough]];
    case GENERATE:
        if ((st == QSEARCH && inCheck) || st == ABSEARCH)
            Movegen::legalmoves<Movetype::ALL>(td->board, movelist);
        else
            Movegen::legalmoves<Movetype::CAPTURE>(td->board, movelist);

        for (int i = 0; i < movelist.size; i++)
        {
            movelist[i].value = scoreMove(movelist[i].move);

            if (movelist[i].move == ttMove)
            {
                assert(playedTT);
                assert(td->board.isPseudoLegal(ttMove) && td->board.isLegal(ttMove));

                std::swap(movelist[0], movelist[i]);
                played++;
            }
        }

        stage++;
        [[fallthrough]];
    case PICK_NEXT:
        if (played < movelist.size)
        {
            const Move move = orderNext();
            assert(td->board.isPseudoLegal(move) && td->board.isLegal(move));
            return move;
        }

        stage++;
        [[fallthrough]];
    case OTHER:
        return NO_MOVE;

    default:
        return NO_MOVE;
    }
}

template <SearchType st> int MovePick<st>::mvvlva(Move move)
{
    int attacker = type_of_piece(td->board.pieceAtB(from(move))) + 1;
    int victim = type_of_piece(td->board.pieceAtB(to(move))) + 1;
    return mvvlvaArray[victim][attacker];
}

template <SearchType st> int MovePick<st>::scoreMove(const Move move)
{
    if (move == ttMove)
    {
        return TT_MOVE_SCORE;
    }
    else if (td->board.pieceAtB(to(move)) != None)
    {
        if (st == QSEARCH)
        {
            return CAPTURE_SCORE + mvvlva(move);
        }
        return td->board.see(move, 0) ? CAPTURE_SCORE + mvvlva(move) : mvvlva(move);
    }
    else
    {
        if (td->killerMoves[0][ss->ply] == move)
        {
            return KILLER_ONE_SCORE;
        }
        else if (td->killerMoves[1][ss->ply] == move)
        {
            return KILLER_TWO_SCORE;
        }
        else
        {
            return getHistory<Movetype::QUIET>(move, td);
        }
    }
}
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
    MovePick(Search &sh, Stack *s, Movelist &moves, const Move move);
    MovePick(Search &sh, Stack *s, Movelist &moves, const Movelist &searchmoves, bool rootNode, const Move move,
             Staging entry);

    Move nextMove();

    Staging stage;

  private:
    Search &search;
    Stack *ss;
    Movelist &movelist;
    Move ttMove;

    bool playedTT = false;
    int played = 0;

    template <bool score> Move orderNext();

    int mvvlva(const Move move) const;
    int scoreMove(const Move move) const;
};

template <SearchType st>
MovePick<st>::MovePick(Search &sh, Stack *s, Movelist &moves, const Move move)
    : search(sh), ss(s), movelist(moves), ttMove(move)
{
    stage = TT_MOVE;
    movelist.size = 0;
    played = 0;
}

template <SearchType st>
MovePick<st>::MovePick(Search &sh, Stack *s, Movelist &moves, const Movelist &searchmoves, bool rootNode,
                       const Move move, Staging entry)
    : search(sh), ss(s), movelist(moves), ttMove(move)
{
    movelist.size = 0;
    played = 0;
    stage = entry;

    if (rootNode && searchmoves.size)
    {
        movelist = searchmoves;
        stage = PICK_NEXT;

        for (auto &ext : movelist)
        {
            if (ext.move == ttMove)
                ext.value = 10'000'000;
            else
                ext.value = scoreMove(ext.move);
        }
    }
}

template <SearchType st> template <bool score> Move MovePick<st>::orderNext()
{
    int index = played;
    if constexpr (score)
        movelist[index].value = scoreMove(movelist[index].move);

    for (int i = 1 + played; i < movelist.size; i++)
    {
        if constexpr (score)
            movelist[i].value = scoreMove(movelist[i].move);

        if (movelist[i] > movelist[index])
            index = i;
    }

    std::swap(movelist[index], movelist[played]);

    return movelist[played++].move;
}

template <SearchType st> Move MovePick<st>::nextMove()
{
    switch (stage)
    {
    case TT_MOVE:
        stage++;

        if ((st == ABSEARCH ? true : search.board.pieceAtB(to(ttMove)) != None) && search.board.isPseudoLegal(ttMove) &&
            search.board.isLegal(ttMove))
        {
            playedTT = true;
            return ttMove;
        }

        [[fallthrough]];
    case GENERATE:
        if constexpr (st == ABSEARCH)
            Movegen::legalmoves<Movetype::ALL>(search.board, movelist);
        else
            Movegen::legalmoves<Movetype::CAPTURE>(search.board, movelist);

        stage++;
        [[fallthrough]];
    case PICK_NEXT:
        while (played < movelist.size)
        {
            Move move = played == 0 ? orderNext<true>() : orderNext<false>();

            if (move == ttMove && playedTT)
                continue;

            assert(move == NO_MOVE || search.board.isPseudoLegal(move) && search.board.isLegal(move));
            return move;
        }

        return NO_MOVE;
    }

    return NO_MOVE;
}

template <SearchType st> int MovePick<st>::mvvlva(Move move) const
{
    int attacker = type_of_piece(search.board.pieceAtB(from(move))) + 1;
    int victim = type_of_piece(search.board.pieceAtB(to(move))) + 1;
    return mvvlvaArray[victim][attacker];
}

template <SearchType st> int MovePick<st>::scoreMove(const Move move) const
{
    if (search.board.pieceAtB(to(move)) != None)
    {
        if (st == QSEARCH)
        {
            return CAPTURE_SCORE + mvvlva(move);
        }

        return search.board.see(move, 0) ? CAPTURE_SCORE + mvvlva(move) : mvvlva(move);
    }
    else
    {
        if (search.killerMoves[0][ss->ply] == move)
        {
            return KILLER_ONE_SCORE;
        }
        else if (search.killerMoves[1][ss->ply] == move)
        {
            return KILLER_TWO_SCORE;
        }
        else if (getHistory<History::COUNTER>((ss - 1)->currentmove, NO_MOVE, search) == move)
        {
            return COUNTER_SCORE;
        }
        return getHistory<History::HH>(move, NO_MOVE, search) +
               2 * (getHistory<History::CONST>(move, (ss - 1)->currentmove, search) +
                    getHistory<History::CONST>(move, (ss - 2)->currentmove, search));
    }
}
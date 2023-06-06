#pragma once

#include "board.h"
#include "search.h"

enum SearchType { QSEARCH, ABSEARCH };

template <SearchType st>
class FastLookup {
   public:
    FastLookup(Search &sh, Stack *s, Movelist &moves, const Move move)
        : search(sh), ss(s), movelist(moves), ttMove(move) {
        movelist.size = 0;
        movegen::legalmoves<Movetype::CAPTURE>(search.board, movelist);
        score();
    }

    FastLookup(Search &sh, Stack *s, Movelist &moves, const Movelist &searchmoves, bool rootNode,
               const Move move)
        : search(sh), ss(s), movelist(moves), ttMove(move) {
        movelist.size = 0;
        movegen::legalmoves<Movetype::ALL>(search.board, movelist);
        score();
    }

    void score() {
        for (int i = 0; i < movelist.size; i++) {
            movelist[i].value = scoreMove(movelist[i].move);

            switch (movelist[i].value) {
                case TT_SCORE:
                    tt_move = movelist[i].move;
                    break;
                case KILLER_ONE_SCORE:
                    killer_move_1 = movelist[i].move;
                    break;
                case KILLER_TWO_SCORE:
                    killer_move_2 = movelist[i].move;
                    break;
                case COUNTER_SCORE:
                    counter_move = movelist[i].move;
                    break;

                default:
                    break;
            }
        }
    }

    Move nextMove() {
        switch (pick) {
            case Pick::TT:
                pick = Pick::CAPTURES;

                if (tt_move != NO_MOVE) {
                    return tt_move;
                }

                [[fallthrough]];
            case Pick::CAPTURES: {
                while (played < movelist.size) {
                    int index = played;
                    for (int i = 1 + index; i < movelist.size; i++) {
                        if (movelist[i] > movelist[index]) {
                            index = i;
                        }
                    }

                    if (movelist[index].value < CAPTURE_SCORE) {
                        break;
                    }

                    std::swap(movelist[index], movelist[played]);

                    if (movelist[played].move != tt_move) {
                        return movelist[played++].move;
                    }

                    played++;
                }

                if constexpr (st == QSEARCH) {
                    return NO_MOVE;
                }

                pick = Pick::KILLERS_1;
                [[fallthrough]];
            }
            case Pick::KILLERS_1:
                pick = Pick::KILLERS_2;

                if (killer_move_1 != NO_MOVE) {
                    return killer_move_1;
                }

                [[fallthrough]];
            case Pick::KILLERS_2:
                pick = Pick::COUNTER;

                if (killer_move_2 != NO_MOVE) {
                    return killer_move_2;
                }

                [[fallthrough]];
            case Pick::COUNTER:
                pick = Pick::QUIET;

                if (counter_move != NO_MOVE) {
                    return counter_move;
                }

                [[fallthrough]];
            case Pick::QUIET:
                while (played < movelist.size) {
                    int index = played;
                    for (int i = 1 + index; i < movelist.size; i++) {
                        if (movelist[i] > movelist[index]) {
                            index = i;
                        }
                    }

                    std::swap(movelist[index], movelist[played]);

                    if (movelist[played].move != tt_move &&
                        movelist[played].move != killer_move_1 &&
                        movelist[played].move != killer_move_2 &&
                        movelist[played].move != counter_move) {
                        assert(movelist[played].value < COUNTER_SCORE);

                        return movelist[played++].move;
                    }

                    played++;
                }

                return NO_MOVE;

            default:
                return NO_MOVE;
        }
        return NO_MOVE;
    }

    int mvvlva(Move move) const {
        int attacker = type_of_piece(search.board.pieceAtB(from(move))) + 1;
        int victim = type_of_piece(search.board.pieceAtB(to(move))) + 1;
        return mvvlvaArray[victim][attacker];
    }

    int scoreMove(const Move move) const {
        if (move == ttMove) return TT_SCORE;

        if constexpr (st == QSEARCH) {
            return CAPTURE_SCORE + mvvlva(move);
        } else if (search.board.pieceAtB(to(move)) != None) {
            return movegen::see(search.board, move, 0) ? CAPTURE_SCORE + mvvlva(move)
                                                       : mvvlva(move);
        } else {
            if (search.killer_moves[0][ss->ply] == move) {
                return KILLER_ONE_SCORE;
            } else if (search.killer_moves[1][ss->ply] == move) {
                return KILLER_TWO_SCORE;
            } else if (getHistory<History::COUNTER>((ss - 1)->currentmove, NO_MOVE, search) ==
                       move) {
                return COUNTER_SCORE;
            }
            return getHistory<History::HH>(move, NO_MOVE, search) +
                   2 * (getHistory<History::CONST>(move, (ss - 1)->currentmove, search) +
                        getHistory<History::CONST>(move, (ss - 2)->currentmove, search));
        }
    }

   private:
    enum class Pick { TT, CAPTURES, KILLERS_1, KILLERS_2, COUNTER, QUIET };

    Search &search;
    Stack *ss;
    Movelist &movelist;
    Move ttMove;

    int played = 0;

    Pick pick = Pick::TT;

    Move tt_move = NO_MOVE;
    Move killer_move_1 = NO_MOVE;
    Move killer_move_2 = NO_MOVE;
    Move counter_move = NO_MOVE;
};

template <SearchType st>
class MovePick {
   public:
    MovePick(Search &sh, Stack *s, Movelist &moves, const Move move);
    MovePick(Search &sh, Stack *s, Movelist &moves, const Movelist &searchmoves, bool rootNode,
             const Move move);

    Move nextMove();

    Staging stage = GENERATE;

   private:
    Search &search;
    Stack *ss;
    Movelist &movelist;
    Move ttMove;

    int played = 0;

    template <bool score>
    Move orderNext();

    int mvvlva(const Move move) const;
    int scoreMove(const Move move) const;
};

template <SearchType st>
MovePick<st>::MovePick(Search &sh, Stack *s, Movelist &moves, const Move move)
    : search(sh), ss(s), movelist(moves), ttMove(move) {
    stage = GENERATE;
    movelist.size = 0;
    played = 0;
}

template <SearchType st>
MovePick<st>::MovePick(Search &sh, Stack *s, Movelist &moves, const Movelist &searchmoves,
                       bool rootNode, const Move move)
    : search(sh), ss(s), movelist(moves), ttMove(move) {
    movelist.size = 0;
    played = 0;

    if (rootNode && searchmoves.size) {
        movelist = searchmoves;
        stage = PICK_NEXT;

        for (auto &ext : movelist) {
            if (ext.move == ttMove)
                ext.value = 10'000'000;
            else
                ext.value = scoreMove(ext.move);
        }
    }
}

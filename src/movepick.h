#pragma once

#include "board.h"
#include "search.h"
#include "types/constants.h"
#include "see.h"

enum SearchType { QSEARCH, ABSEARCH };

template <SearchType st>
class MovePicker {
   public:
    MovePicker(Search &sh, Stack *s, Movelist &moves, const Move move)
        : search_(sh), ss_(s), movelist_(moves), available_tt_move_(move) {
        movelist_.size = 0;
        movegen::legalmoves<Movetype::CAPTURE>(search_.board, movelist_);
    }

    MovePicker(Search &sh, Stack *s, Movelist &moves, const Movelist &searchmoves, bool rootNode,
               const Move move)
        : search_(sh), ss_(s), movelist_(moves), available_tt_move_(move) {
        if (rootNode && searchmoves.size > 0) {
            movelist_ = searchmoves;
            return;
        }
        movelist_.size = 0;
        movegen::legalmoves<Movetype::ALL>(search_.board, movelist_);
    }

    void score() {
        for (int i = 0; i < movelist_.size; i++) {
            movelist_[i].value = scoreMove(movelist_[i].move);
        }
    }

    Move nextMove() {
        switch (pick_) {
            case Pick::TT:
                pick_ = Pick::SCORE;

                if (available_tt_move_ != NO_MOVE && movelist_.find(available_tt_move_) != -1) {
                    tt_move_ = available_tt_move_;
                    return tt_move_;
                }

                [[fallthrough]];
            case Pick::SCORE:
                pick_ = Pick::CAPTURES;

                score();
                [[fallthrough]];
            case Pick::CAPTURES: {
                while (played_ < movelist_.size) {
                    int index = played_;
                    for (int i = 1 + index; i < movelist_.size; i++) {
                        if (movelist_[i] > movelist_[index]) {
                            index = i;
                        }
                    }

                    if (movelist_[index].value < CAPTURE_SCORE) {
                        break;
                    }

                    std::swap(movelist_[index], movelist_[played_]);

                    if (movelist_[played_].move != tt_move_) {
                        return movelist_[played_++].move;
                    }

                    played_++;
                }

                if constexpr (st == QSEARCH) {
                    return NO_MOVE;
                }

                pick_ = Pick::KILLERS_1;
                [[fallthrough]];
            }
            case Pick::KILLERS_1:
                pick_ = Pick::KILLERS_2;

                if (killer_move_1_ != NO_MOVE) {
                    return killer_move_1_;
                }

                [[fallthrough]];
            case Pick::KILLERS_2:
                pick_ = Pick::COUNTER;

                if (killer_move_2_ != NO_MOVE) {
                    return killer_move_2_;
                }

                [[fallthrough]];
            case Pick::COUNTER:
                pick_ = Pick::QUIET;

                if (counter_move_ != NO_MOVE) {
                    return counter_move_;
                }

                [[fallthrough]];
            case Pick::QUIET:
                while (played_ < movelist_.size) {
                    int index = played_;
                    for (int i = 1 + index; i < movelist_.size; i++) {
                        if (movelist_[i] > movelist_[index]) {
                            index = i;
                        }
                    }

                    std::swap(movelist_[index], movelist_[played_]);

                    if (movelist_[played_].move != tt_move_ &&
                        movelist_[played_].move != killer_move_1_ &&
                        movelist_[played_].move != killer_move_2_ &&
                        movelist_[played_].move != counter_move_) {
                        assert(movelist_[played_].value < COUNTER_SCORE);

                        return movelist_[played_++].move;
                    }

                    played_++;
                }

                return NO_MOVE;

            default:
                return NO_MOVE;
        }
        return NO_MOVE;
    }

    int mvvlva(Move move) const {
        int attacker = typeOfPiece(search_.board.at(from(move))) + 1;
        int victim = typeOfPiece(search_.board.at(to(move))) + 1;
        return mvvlvaArray[victim][attacker];
    }

    int scoreMove(const Move move) {
        if constexpr (st == QSEARCH) {
            return CAPTURE_SCORE + mvvlva(move);
        }

        if (search_.board.at(to(move)) != None) {
            return see::see(search_.board, move, 0) ? CAPTURE_SCORE + mvvlva(move) : mvvlva(move);
        }

        if (search_.killer_moves[0][ss_->ply] == move) {
            killer_move_1_ = move;
            return KILLER_ONE_SCORE;
        } else if (search_.killer_moves[1][ss_->ply] == move) {
            killer_move_2_ = move;
            return KILLER_TWO_SCORE;
        } else if (getHistory<History::COUNTER>((ss_ - 1)->currentmove, NO_MOVE, search_) == move) {
            counter_move_ = move;
            return COUNTER_SCORE;
        }

        return getHistory<History::HH>(move, NO_MOVE, search_) +
               2 * (getHistory<History::CONST>(move, (ss_ - 1)->currentmove, search_) +
                    getHistory<History::CONST>(move, (ss_ - 2)->currentmove, search_));
    }

   private:
    enum class Pick { TT, SCORE, CAPTURES, KILLERS_1, KILLERS_2, COUNTER, QUIET };

    const Search &search_;
    const Stack *ss_;

    Movelist &movelist_;

    int played_ = 0;

    Pick pick_ = Pick::TT;

    Move available_tt_move_ = NO_MOVE;

    Move tt_move_ = NO_MOVE;
    Move killer_move_1_ = NO_MOVE;
    Move killer_move_2_ = NO_MOVE;
    Move counter_move_ = NO_MOVE;
};

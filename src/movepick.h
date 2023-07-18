#pragma once

#include "board.h"
#include "history.h"
#include "search.h"
#include "see.h"
#include "types/constants.h"

enum SearchType { QSEARCH, ABSEARCH };

enum MoveScores : int {
    TT_SCORE = 10'000'000,
    CAPTURE_SCORE = 7'000'000,
    KILLER_ONE_SCORE = 6'000'000,
    KILLER_TWO_SCORE = 5'000'000,
    COUNTER_SCORE = 4'000'000,
    NEGATIVE_SCORE = -10'000'000
};

template <SearchType st>
class MovePicker {
   public:
    MovePicker(const Search &sh, const Stack *s, Movelist &moves, const Move move)
        :  movelist(moves), search_(sh), ss_(s), available_tt_move_(move) {
        movelist.size = 0;
        movegen::legalmoves<Movetype::CAPTURE>(search_.board, movelist);
    }

    MovePicker(const Search &sh, const Stack *s, Movelist &moves, const Movelist &searchmoves,
               const bool root_node, const Move move)
        : movelist(moves),search_(sh), ss_(s),  available_tt_move_(move) {
        if (root_node && searchmoves.size > 0) {
            movelist = searchmoves;
            return;
        }
        movelist.size = 0;
        movegen::legalmoves<Movetype::ALL>(search_.board, movelist);
    }

    void score() {
        for (int i = 0; i < movelist.size; i++) {
            movelist[i].value = scoreMove(movelist[i].move);
        }
    }

    [[nodiscard]] Move nextMove() {
        switch (pick_) {
            case Pick::TT:
                pick_ = Pick::SCORE;

                if (available_tt_move_ != NO_MOVE && movelist.find(available_tt_move_) != -1) {
                    tt_move_ = available_tt_move_;
                    return tt_move_;
                }

                [[fallthrough]];
            case Pick::SCORE:
                pick_ = Pick::CAPTURES;

                score();
                [[fallthrough]];
            case Pick::CAPTURES: {
                while (played_ < movelist.size) {
                    int index = played_;
                    for (int i = 1 + index; i < movelist.size; i++) {
                        if (movelist[i] > movelist[index]) {
                            index = i;
                        }
                    }

                    if (movelist[index].value < CAPTURE_SCORE) {
                        break;
                    }

                    std::swap(movelist[index], movelist[played_]);

                    if (movelist[played_].move != tt_move_) {
                        return movelist[played_++].move;
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
                while (played_ < movelist.size) {
                    int index = played_;
                    for (int i = 1 + index; i < movelist.size; i++) {
                        if (movelist[i] > movelist[index]) {
                            index = i;
                        }
                    }

                    std::swap(movelist[index], movelist[played_]);

                    if (movelist[played_].move != tt_move_ &&
                        movelist[played_].move != killer_move_1_ &&
                        movelist[played_].move != killer_move_2_ &&
                        movelist[played_].move != counter_move_) {
                        assert(movelist[played_].value < COUNTER_SCORE);

                        return movelist[played_++].move;
                    }

                    played_++;
                }

                return NO_MOVE;

            default:
                return NO_MOVE;
        }
    }

    [[nodiscard]] int mvvlva(Move move) const {
        int attacker = search_.board.at<PieceType>(from(move)) + 1;
        int victim = search_.board.at<PieceType>(to(move)) + 1;
        return mvvlvaArray[victim][attacker];
    }

    [[nodiscard]] int scoreMove(const Move move) {
        if constexpr (st == QSEARCH) {
            return CAPTURE_SCORE + mvvlva(move);
        }

        if (search_.board.at(to(move)) != NONE) {
            return see::see(search_.board, move, 0) ? CAPTURE_SCORE + mvvlva(move) : mvvlva(move);
        }

        if (search_.killers[0][ss_->ply] == move) {
            killer_move_1_ = move;
            return KILLER_ONE_SCORE;
        } else if (search_.killers[1][ss_->ply] == move) {
            killer_move_2_ = move;
            return KILLER_TWO_SCORE;
        } else if (history::get<HistoryType::COUNTER>((ss_ - 1)->currentmove, NO_MOVE, search_) ==
                   move) {
            counter_move_ = move;
            return COUNTER_SCORE;
        }

        return history::get<HistoryType::HH>(move, NO_MOVE, search_) +
               2 * (history::get<HistoryType::CONST>(move, (ss_ - 1)->currentmove, search_) +
                    history::get<HistoryType::CONST>(move, (ss_ - 2)->currentmove, search_));
    }

    Movelist &movelist;

   private:
    enum class Pick { TT, SCORE, CAPTURES, KILLERS_1, KILLERS_2, COUNTER, QUIET };

    const Search &search_;
    const Stack *ss_;

    int played_ = 0;

    Pick pick_ = Pick::TT;

    Move available_tt_move_ = NO_MOVE;

    Move tt_move_ = NO_MOVE;
    Move killer_move_1_ = NO_MOVE;
    Move killer_move_2_ = NO_MOVE;
    Move counter_move_ = NO_MOVE;
};

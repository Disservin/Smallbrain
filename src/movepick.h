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

int threatScore(const Board &board, const Move move) {
    const auto bb = attacks::attacksByPiece(board.at<PieceType>(from(move)), board.sideToMove(),
                                            to(move), board.all() & ~(1ull << from(move)));
    switch (board.at<PieceType>(from(move))) {
        case PAWN: {
            const int score =
                500 * builtin::popcount(bb & board.pieces(PieceType::QUEEN, ~board.sideToMove())) +
                350 * builtin::popcount(bb & board.pieces(PieceType::ROOK, ~board.sideToMove())) +
                150 * builtin::popcount(bb & board.pieces(PieceType::BISHOP, ~board.sideToMove())) +
                150 * builtin::popcount(bb & board.pieces(PieceType::KNIGHT, ~board.sideToMove()));
            return score;
        }
        case KNIGHT: {
            const int score =
                400 * builtin::popcount(bb & board.pieces(PieceType::QUEEN, ~board.sideToMove())) +
                300 * builtin::popcount(bb & board.pieces(PieceType::ROOK, ~board.sideToMove()));
            return score;
        }
        case BISHOP: {
            const int score =
                400 * builtin::popcount(bb & board.pieces(PieceType::QUEEN, ~board.sideToMove())) +
                300 * builtin::popcount(bb & board.pieces(PieceType::ROOK, ~board.sideToMove()));
            return score;
        }
        case ROOK: {
            const int score =
                200 * builtin::popcount(bb & board.pieces(PieceType::QUEEN, ~board.sideToMove()));
            return score;
        }
        case QUEEN:
            return 0;
        case KING:
            return 0;
        default:
            return 0;
    }
}

template <SearchType st>
class MovePicker {
   public:
    MovePicker(const Search &sh, const Stack *s, Movelist &moves, const Move move)
        : search_(sh), ss_(s), movelist_(moves), available_tt_move_(move) {
        movelist_.size = 0;
        movegen::legalmoves<Movetype::CAPTURE>(search_.board, movelist_);
    }

    MovePicker(const Search &sh, const Stack *s, Movelist &moves, const Movelist &searchmoves,
               const bool root_node, const Move move)
        : search_(sh), ss_(s), movelist_(moves), available_tt_move_(move) {
        if (root_node && searchmoves.size > 0) {
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

    [[nodiscard]] Move nextMove() {
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
        } else if (getHistory<History::COUNTER>((ss_ - 1)->currentmove, NO_MOVE, search_) == move) {
            counter_move_ = move;
            return COUNTER_SCORE;
        }

        return getHistory<History::HH>(move, NO_MOVE, search_) +
               2 * (getHistory<History::CONST>(move, (ss_ - 1)->currentmove, search_) +
                    getHistory<History::CONST>(move, (ss_ - 2)->currentmove, search_)) +
               threatScore(search_.board, move);
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

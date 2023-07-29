#include <algorithm>  // clamp
#include <cmath>

#include "probe.h"

#include "evaluation.h"
#include "movepick.h"
#include "search.h"
#include "see.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"

extern ThreadPool Threads;

// Initialize reduction table
int reductions[MAX_PLY][MAX_MOVES];

void init_reductions() {
    reductions[0][0] = 0;

    for (int depth = 1; depth < MAX_PLY; depth++) {
        for (int moves = 1; moves < MAX_MOVES; moves++)
            reductions[depth][moves] = 1 + log(depth) * log(moves) / 1.75;
    }
}

[[nodiscard]] Score mateIn(int ply) { return (VALUE_MATE - ply); }

[[nodiscard]] Score matedIn(int ply) { return (ply - VALUE_MATE); }

[[nodiscard]] Score scoreToTT(Score s, int plies) {
    return (s >= VALUE_TB_WIN_IN_MAX_PLY    ? s + plies
            : s <= VALUE_TB_LOSS_IN_MAX_PLY ? s - plies
                                            : s);
}

[[nodiscard]] Score scoreFromTT(Score s, int plies) {
    if (s == VALUE_NONE) return VALUE_NONE;

    return (s >= VALUE_TB_WIN_IN_MAX_PLY    ? s - plies
            : s <= VALUE_TB_LOSS_IN_MAX_PLY ? s + plies
                                            : s);
}

template <Node node>
Score Search::qsearch(Score alpha, Score beta, Stack *ss) {
    if (limitReached()) return 0;

    /********************
     * Initialize various variables
     *******************/
    constexpr bool pv_node = node == PV;

    assert(alpha >= -VALUE_INFINITE && alpha < beta && beta <= VALUE_INFINITE);
    assert(pv_node || (alpha == beta - 1));

    if (ss->ply >= MAX_PLY) return eval::evaluate(board);

    /********************
     * Check for repetition or 50 move rule draw
     *******************/
    if (board.isRepetition(1 + pv_node)) return -1 + (nodes & 0x2);

    const Color color = board.sideToMove();
    const bool in_check = board.isAttacked(~color, board.kingSQ(color), board.all());
    const Result state = board.isDrawn(in_check);

    if (state != Result::NONE) return state == Result::LOST ? matedIn(ss->ply) : 0;

    /********************
     * Look up in the TT
     * Adjust alpha and beta for non PV Nodes.
     *******************/

    Move ttmove = NO_MOVE;
    bool tt_hit = false;

    const TEntry *tte = TTable.probe(tt_hit, ttmove, board.hash());
    const Score tt_score = tt_hit ? scoreFromTT(tte->score, ss->ply) : Score(VALUE_NONE);

    // clang-format off
    if (tt_hit
        && !pv_node
        && tt_score != VALUE_NONE
        && tte->flag != NONEBOUND) {
        // clang-format on
        if (tte->flag == EXACTBOUND)
            return tt_score;
        else if (tte->flag == LOWERBOUND && tt_score >= beta)
            return tt_score;
        else if (tte->flag == UPPERBOUND && tt_score <= alpha)
            return tt_score;
    }

    Score best_value = eval::evaluate(board);

    if (best_value >= beta) return best_value;
    if (best_value > alpha) alpha = best_value;

    Movelist moves;
    MovePicker<QSEARCH> mp(*this, ss, moves, ttmove);

    /********************
     * Search the moves
     *******************/
    Move bestmove = NO_MOVE;
    Move move = NO_MOVE;

    while ((move = mp.nextMove()) != NO_MOVE) {
        auto captured = board.at<PieceType>(to(move));

        if (best_value > VALUE_TB_LOSS_IN_MAX_PLY) {
            // delta pruning, if the move + a large margin is still less then alpha we can safely
            // skip this
            // clang-format off
            if (captured != NONETYPE
                && !in_check
                && best_value + 400 + PIECE_VALUES_TUNED[1][captured] < alpha
                && typeOf(move) != PROMOTION
                && board.nonPawnMat(color))
                // clang-format on
                continue;

            // see based capture pruning
            if (!in_check && !see::see(board, move, 0)) continue;
        }

        nodes++;

        board.makeMove<true>(move);

        Score score = -qsearch<node>(-beta, -alpha, ss + 1);

        board.unmakeMove<false>(move);

        assert(score > -VALUE_INFINITE && score < VALUE_INFINITE);

        // update the best score
        if (score > best_value) {
            best_value = score;

            if (score > alpha) {
                alpha = score;
                bestmove = move;

                if (score >= beta) break;
            }
        }
    }

    /********************
     * store in the transposition table
     *******************/

    const Flag bound = best_value >= beta ? LOWERBOUND : UPPERBOUND;

    if (!Threads.stop.load(std::memory_order_relaxed))
        TTable.store(0, scoreToTT(best_value, ss->ply), bound, board.hash(), bestmove);

    assert(best_value > -VALUE_INFINITE && best_value < VALUE_INFINITE);
    return best_value;
}

template <Node node>
Score Search::absearch(int depth, Score alpha, Score beta, Stack *ss) {
    if (limitReached()) return 0;

    /********************
     * Initialize various variables
     *******************/

    const Color color = board.sideToMove();
    const bool in_check = board.isAttacked(~color, board.kingSQ(color), board.all());

    if (ss->ply >= MAX_PLY) return (ss->ply >= MAX_PLY && !in_check) ? eval::evaluate(board) : 0;

    pv_length_[ss->ply] = ss->ply;

    constexpr bool root_node = node == ROOT;
    constexpr bool pv_node = node != NONPV;

    /********************
     * Draw detection and mate pruning
     *******************/
    if (!root_node) {
        if (board.isRepetition(1 + pv_node)) return -1 + (nodes & 0x2);

        const Result state = board.isDrawn(in_check);
        if (state != Result::NONE) return state == Result::LOST ? matedIn(ss->ply) : 0;

        alpha = std::max(alpha, matedIn(ss->ply));
        beta = std::min(beta, mateIn(ss->ply + 1));
        if (alpha >= beta) return alpha;
    }

    /********************
     * Check extension
     *******************/

    if (in_check) depth++;

    /********************
     * Enter qsearch
     *******************/
    if (depth <= 0) return qsearch<node>(alpha, beta, ss);

    assert(-VALUE_INFINITE <= alpha && alpha < beta && beta <= VALUE_INFINITE);
    assert(pv_node || (alpha == beta - 1));
    assert(0 < depth && depth < MAX_PLY);
    assert(ss->ply < MAX_PLY);

    (ss + 1)->excluded_move = NO_MOVE;

    /********************
     * Selective depth
     * Heighest depth a pv_node has reached
     *******************/
    if (pv_node && ss->ply > seldepth_) seldepth_ = ss->ply;

    // Look up in the TT
    Move ttmove = NO_MOVE;
    bool tt_hit = false;

    const TEntry *tte = TTable.probe(tt_hit, ttmove, board.hash());
    const Score tt_score = tt_hit ? scoreFromTT(tte->score, ss->ply) : Score(VALUE_NONE);

    const Move excluded_move = ss->excluded_move;

    /********************
     * Look up in the TT
     * Adjust alpha and beta for non PV nodes
     *******************/
    // clang-format off
    if (!root_node
        && !excluded_move
        && !pv_node
        && tt_hit
        && tt_score != VALUE_NONE
        && tte->depth >= depth
        && (ss - 1)->currentmove != NULL_MOVE) {
        // clang-format on
        if (tte->flag == EXACTBOUND)
            return tt_score;
        else if (tte->flag == LOWERBOUND)
            alpha = std::max(alpha, tt_score);
        else if (tte->flag == UPPERBOUND)
            beta = std::min(beta, tt_score);
        if (alpha >= beta) return tt_score;
    }

    Score max_value = VALUE_INFINITE;
    Score best = -VALUE_INFINITE;

    /********************
     *  Tablebase probing
     *******************/
    Score tb_res = VALUE_NONE;

    if (!root_node && !silent && use_tb) tb_res = syzygy::probeWDL(board);

    if (tb_res != VALUE_NONE) {
        Flag flag = NONEBOUND;
        tbhits++;

        switch (tb_res) {
            case VALUE_TB_WIN:
                tb_res = VALUE_MATE_IN_PLY - ss->ply - 1;
                flag = LOWERBOUND;
                break;
            case VALUE_TB_LOSS:
                tb_res = VALUE_MATED_IN_PLY + ss->ply + 1;
                flag = UPPERBOUND;
                break;
            default:
                tb_res = 0;
                flag = EXACTBOUND;
                break;
        }

        if (flag == EXACTBOUND || (flag == LOWERBOUND && tb_res >= beta) ||
            (flag == UPPERBOUND && tb_res <= alpha)) {
            TTable.store(depth + 6, scoreToTT(tb_res, ss->ply), flag, board.hash(), NO_MOVE);
            return tb_res;
        }

        if (pv_node) {
            if (flag == LOWERBOUND) {
                best = tb_res;
                alpha = std::max(alpha, best);
            } else {
                max_value = tb_res;
            }
        }
    }

    bool improving = false;

    if (in_check) {
        ss->eval = VALUE_NONE;
        goto moves;
    }

    // Use the tt_score as a better evaluation of the position, other engines
    // typically have eval and staticEval. In Smallbrain its just eval.
    ss->eval = tt_hit ? tt_score : eval::evaluate(board);

    // improving boolean
    improving = (ss - 2)->eval != VALUE_NONE && ss->eval > (ss - 2)->eval;

    if (root_node) goto moves;

    /********************
     * Internal Iterative Reductions (IIR)
     *******************/
    if (depth >= 3 && !tt_hit) depth--;

    if (pv_node && !tt_hit) depth--;

    if (depth <= 0) return qsearch<PV>(alpha, beta, ss);

    // Skip early pruning in Pv Nodes

    if (pv_node) goto moves;

    /********************
     * Razoring
     *******************/
    if (depth < 3 && ss->eval + 129 < alpha) return qsearch<NONPV>(alpha, beta, ss);

    /********************
     * Reverse futility pruning
     *******************/
    if (std::abs(beta) < VALUE_TB_WIN_IN_MAX_PLY)
        if (depth < 7 && ss->eval - 64 * depth + 71 * improving >= beta) return beta;

    /********************
     * Null move pruning
     *******************/
    // clang-format off
    if (board.nonPawnMat(color)
        && !excluded_move
        && (ss - 1)->currentmove != NULL_MOVE
        && depth >= 3
        && ss->eval >= beta) {
        // clang-format on
        int R = 5 + std::min(4, depth / 5) + std::min(3, (ss->eval - beta) / 214);

        (ss)->currentmove = NULL_MOVE;

        board.makeNullMove();
        Score score = -absearch<NONPV>(depth - R, -beta, -beta + 1, ss + 1);
        board.unmakeNullMove();

        if (score >= beta) {
            // dont return mate scores
            if (score >= VALUE_TB_WIN_IN_MAX_PLY) score = beta;

            return score;
        }
    }

moves:
    Movelist moves;
    Move quiets[64];

    Score score = VALUE_NONE;
    Move bestmove = NO_MOVE;
    Move move = NO_MOVE;
    uint8_t quiet_count = 0;
    uint8_t made_moves = 0;
    bool do_full_search = false;

    MovePicker<ABSEARCH> mp(*this, ss, moves, searchmoves, root_node, tt_hit ? ttmove : NO_MOVE);
    ss->move_count = mp.movelist.size;

    /********************
     * Movepicker fetches the next move that we should search.
     * It is very important to return the likely best move first,
     * since then we get many cut offs.
     *******************/
    while ((move = mp.nextMove()) != NO_MOVE) {
        if (move == excluded_move) continue;

        made_moves++;

        int extension = 0;

        const bool capture = board.at(to(move)) != NONE;

        /********************
         * Various pruning techniques.
         *******************/
        if (!root_node && best > VALUE_TB_LOSS_IN_MAX_PLY) {
            // clang-format off
            if (capture) {
                // SEE pruning
                if (depth < 6
                    && !see::see(board, move, -(depth * 92)))
                    continue;
            } else {
                // late move pruning/movecount pruning
                if (!in_check
                    && !pv_node
                    && typeOf(move) != PROMOTION
                    && depth <= 5
                    && quiet_count > (4 + depth * depth))

                    continue;
                // SEE pruning
                if (depth < 7
                    && !see::see(board, move, -(depth * 93)))
                    continue;
            }
            // clang-format on
        }

        // clang-format off
        // Singular extensions
        if (!root_node
            && depth >= 8
            && tt_hit  // tt_score cannot be VALUE_NONE!
            && ttmove == move
            && !excluded_move
            && std::abs(tt_score) < 10000
            && tte->flag & LOWERBOUND
            && tte->depth >= depth - 3) {
            // clang-format on
            const Score singular_beta = tt_score - 3 * depth;
            const int singular_depth = (depth - 1) / 2;

            ss->excluded_move = move;
            const auto value =
                absearch<NONPV>(singular_depth, singular_beta - 1, singular_beta, ss);
            ss->excluded_move = NO_MOVE;

            if (value < singular_beta)
                extension = 1;
            else if (singular_beta >= beta)
                return singular_beta;
        }

        // One reply extensions
        if (in_check && (ss - 1)->move_count == 1 && ss->move_count == 1) extension += 1;

        const int newDepth = depth - 1 + extension;

        /********************
         * Print currmove information.
         *******************/
        // clang-format off
        if (id == 0
            && root_node
            && !silent
            && !Threads.stop.load(std::memory_order_relaxed)
            && getTime() > 10000)
            std::cout << "info depth " << depth - in_check
                      << " currmove " << uci::moveToUci(move, board.chess960)
                      << " currmovenumber " << signed(made_moves) << std::endl;
        // clang-format on

        /********************
         * Play the move on the internal board.
         *******************/
        nodes++;
        board.makeMove<true>(move);

        const U64 node_count = nodes;
        ss->currentmove = move;

        /********************
         * Late move reduction, later moves will be searched
         * with a reduced depth, if they beat alpha we search again at
         * full depth.
         *******************/
        if (depth >= 3 && !in_check && made_moves > 3 + 2 * pv_node) {
            int rdepth = reductions[depth][made_moves];

            rdepth -= id % 2;

            rdepth += improving;

            rdepth -= pv_node;

            rdepth -= capture;

            rdepth = std::clamp(newDepth - rdepth, 1, newDepth + 1);

            score = -absearch<NONPV>(rdepth, -alpha - 1, -alpha, ss + 1);

            /**********************
             * If the reduced search beats alpha we do a full search but only if
             * rdepth is smaller than newDepth, because otherwise we would do the same search twice.
             *******************/
            do_full_search = score > alpha && rdepth < newDepth;
        } else
            do_full_search = !pv_node || made_moves > 1;

        /********************
         * Do a full research if lmr failed or lmr was skipped
         *******************/
        if (do_full_search) {
            score = -absearch<NONPV>(newDepth, -alpha - 1, -alpha, ss + 1);
        }

        /********************
         * PVS Search
         * We search the first move or PV Nodes that are inside the bounds
         * with a full window at (more or less) full depth.
         *******************/
        if (pv_node && ((score > alpha && score < beta) || made_moves == 1)) {
            score = -absearch<PV>(newDepth, -beta, -alpha, ss + 1);
        }

        board.unmakeMove<false>(move);

        assert(score > -VALUE_INFINITE && score < VALUE_INFINITE);

        /********************
         * Node count logic used for time control.
         *******************/
        if (id == 0) node_effort[from(move)][to(move)] += nodes - node_count;

        /********************
         * Score beat best -> update PV and Bestmove.
         *******************/
        if (score > best) {
            best = score;

            if (score > alpha) {
                alpha = score;
                bestmove = move;

                // update the PV
                pv_table_[ss->ply][ss->ply] = move;

                for (int next_ply = ss->ply + 1; next_ply < pv_length_[ss->ply + 1]; next_ply++) {
                    assert(ss->ply < MAX_PLY + 1);
                    assert(next_ply < MAX_PLY + 1);
                    assert(ss->ply + 1 < MAX_PLY + 1);
                    pv_table_[ss->ply][next_ply] = pv_table_[ss->ply + 1][next_ply];
                }

                pv_length_[ss->ply] = pv_length_[ss->ply + 1];

                /********************
                 * Score beat beta -> update histories and break.
                 *******************/
                if (score >= beta) {
                    TTable.prefetch<1>(board.hash());
                    // update history heuristic
                    history::update(*this, bestmove, depth, quiets, quiet_count, ss);
                    break;
                }
            }
        }
        if (!capture) quiets[quiet_count++] = move;
    }

    ss->move_count = made_moves;

    /********************
     * If the move list is empty, we are in checkmate or stalemate.
     *******************/
    if (made_moves == 0) best = excluded_move ? alpha : in_check ? matedIn(ss->ply) : 0;

    if (pv_node) best = std::min(best, max_value);

    /********************
     * Store an TEntry in the Transposition Table.
     *******************/

    // Transposition table flag
    const Flag b =
        best >= beta ? LOWERBOUND : (pv_node && bestmove != NO_MOVE ? EXACTBOUND : UPPERBOUND);

    if (!excluded_move && !Threads.stop.load(std::memory_order_relaxed))
        TTable.store(depth, scoreToTT(best, ss->ply), b, board.hash(), bestmove);

    assert(best > -VALUE_INFINITE && best < VALUE_INFINITE);
    return best;
}

Score Search::aspirationSearch(int depth, Score prev_eval, Stack *ss) {
    Score alpha = -VALUE_INFINITE;
    Score beta = VALUE_INFINITE;
    int delta = 30;

    /********************
     * We search moves after depth 9 with an aspiration Window.
     * A small window around the previous evaluation enables us
     * to have a smaller search tree. We dont do this for the first
     * few depths because these have quite unstable evaluation which
     * would lead to many researches.
     *******************/
    if (depth >= 9) {
        alpha = prev_eval - delta;
        beta = prev_eval + delta;
    }

    Score result = 0;
    while (true) {
        if (alpha < -3500) alpha = -VALUE_INFINITE;
        if (beta > 3500) beta = VALUE_INFINITE;

        result = absearch<ROOT>(depth, alpha, beta, ss);

        if (Threads.stop.load(std::memory_order_relaxed)) return 0;

        if (id == 0 && limit.nodes != 0 && nodes >= limit.nodes) return 0;

        /********************
         * Increase the bounds because the score was outside of them or
         * break in case it was a EXACTBOUND result.
         *******************/
        if (result <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = std::max(alpha - delta, -(static_cast<int>(VALUE_INFINITE)));
            delta += delta / 2;
        } else if (result >= beta) {
            beta = std::min(beta + delta, static_cast<int>(VALUE_INFINITE));
            delta += delta / 2;
        } else {
            break;
        }
    }

    if (id == 0 && !silent) {
        uci::output(result, board.ply(), depth, seldepth_, Threads.getNodes(), Threads.getTbHits(),
                    getTime(), getPV(), TTable.hashfull());
    }

    return result;
}

SearchResult Search::iterativeDeepening() {
    SearchResult search_result;

    Stack stack[MAX_PLY + 4], *ss = stack + 2;

    for (int i = 2; i > 0; --i) {
        (ss - i)->ply = i;
        (ss - i)->move_count = 0;
        (ss - i)->currentmove = NO_MOVE;
        (ss - i)->eval = 0;
        (ss - i)->excluded_move = NO_MOVE;
    }

    for (int i = 0; i <= MAX_PLY + 1; ++i) {
        (ss + i)->ply = i;
        (ss + i)->move_count = 0;
        (ss + i)->currentmove = NO_MOVE;
        (ss + i)->eval = 0;
        (ss + i)->excluded_move = NO_MOVE;
    }

    pv_table_.reset();
    pv_length_.reset();
    node_effort.reset();

    auto lastPv = getPV();

    /********************
     * Iterative Deepening Loop.
     *******************/
    int bestmove_changes = 0;
    int eval_average = 0;

    int depth = 1;
    for (; depth <= limit.depth; depth++) {
        seldepth_ = 0;

        const auto previousResult = search_result.score;
        const auto value = aspirationSearch(depth, search_result.score, ss);

        if (limitReached()) break;

        // only mainthread manages time control
        if (id != 0) continue;

        if (search_result.bestmove != pv_table_[0][0]) bestmove_changes++;

        search_result.bestmove = pv_table_[0][0];
        search_result.score = value;

        lastPv = getPV();

        eval_average += search_result.score;

        // limit type time
        if (limit.time.optimum != 0) {
            auto now = getTime();

            // node count time management (https://github.com/Luecx/Koivisto 's idea)
            int effort =
                (node_effort[from(search_result.bestmove)][to(search_result.bestmove)] * 100) /
                nodes;
            if (depth > 10 && limit.time.optimum * (110 - std::min(effort, 90)) / 100 < now) break;

            // increase optimum time if score is increasing
            if (search_result.score + 30 < eval_average / depth) limit.time.optimum *= 1.10;

            // increase optimum time if score is dropping
            if (search_result.score > -200 && search_result.score - previousResult < -20)
                limit.time.optimum *= 1.10;

            // increase optimum time if bestmove fluctates
            if (bestmove_changes > 4) limit.time.optimum = limit.time.maximum * 0.75;

            // stop if we have searched for more than 75% of our max time.
            if (depth > 10 && now * 10 > limit.time.maximum * 6) break;
        }
    }

    /********************
     * Dont stop analysis in infinite mode when max depth is reached
     * wait for uci stop or quit
     *******************/
    while (limit.infinite && !Threads.stop.load(std::memory_order_relaxed)) {
    }

    /********************
     * In case the depth was 1 make sure we have at least a bestmove.
     *******************/
    if (depth == 1) search_result.bestmove = pv_table_[0][0];

    /********************
     * Mainthread prints bestmove.
     * Allowprint is disabled in data generation
     *******************/
    if (id == 0 && !silent) {
        uci::output(
            search_result.score, board.ply(), depth, seldepth_, Threads.getNodes(),
            Threads.getTbHits(), getTime(),
            lastPv.empty() ? uci::moveToUci(search_result.bestmove, board.chess960) : lastPv,
            TTable.hashfull());
        std::cout << "bestmove " << uci::moveToUci(search_result.bestmove, board.chess960)
                  << std::endl;
        Threads.stop = true;
    }

    printMean();

    return search_result;
}

void Search::reset() {
    nodes = 0;
    tbhits = 0;

    node_effort.reset();

    history.reset();
    counters.reset();
    consthist.reset();

    killers.reset();
}

void Search::startThinking() {
    /********************
     * Various Limits that only the MainThread needs to know
     * and initialise.
     *******************/
    t0_ = TimePoint::now();
    check_time_ = 0;

    /********************
     * Play dtz move when time is limited
     *******************/
    if (id == 0 && limit.time.optimum != 0 && use_tb) {
        const auto dtz = syzygy::probeDTZ(board);
        if (dtz.second != NO_MOVE) {
            uci::output(dtz.first, 1, 1, 1, 1, 1, 0,
                        " " + uci::moveToUci(dtz.second, board.chess960), 0);
            std::cout << "bestmove " << uci::moveToUci(dtz.second, board.chess960) << std::endl;
            Threads.stop = true;
            return;
        }
    }

    iterativeDeepening();
}

bool Search::limitReached() {
    if (!silent && Threads.stop.load(std::memory_order_relaxed)) return true;

    if (id != 0) return false;

    if (limit.nodes != 0 && nodes >= limit.nodes) return true;

    if (--check_time_ > 0) return false;

    check_time_ = 2047;

    if (limit.time.maximum != 0) {
        auto ms = getTime();

        if (ms >= limit.time.maximum) {
            Threads.stop = true;

            return true;
        }
    }
    return false;
}

std::string Search::getPV() const {
    std::stringstream ss;

    for (int i = 0; i < pv_length_[0]; i++) {
        ss << " " << uci::moveToUci(pv_table_[0][i], board.chess960);
    }

    return ss.str();
}

int64_t Search::getTime() const {
    auto t1 = TimePoint::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0_).count();
}

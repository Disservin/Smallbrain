#include <algorithm>  // clamp
#include <cmath>

#include "syzygy/Fathom/src/tbprobe.h"

#include "evaluation.h"
#include "movepick.h"
#include "search.h"
#include "thread.h"
#include "tt.h"

extern ThreadPool Threads;
extern TranspositionTable TTable;

// Initialize reduction table
int reductions[MAX_PLY][MAX_MOVES];

void init_reductions() {
    reductions[0][0] = 0;

    for (int depth = 1; depth < MAX_PLY; depth++) {
        for (int moves = 1; moves < MAX_MOVES; moves++)
            reductions[depth][moves] = 1 + log(depth) * log(moves) / 1.75;
    }
}

int bonus(int depth) { return std::min(2000, depth * 155); }

template <History type>
void Search::updateHistoryBonus(Move move, Move secondMove, int bonus) {
    int hhBonus = bonus - getHistory<type>(move, secondMove, *this) * std::abs(bonus) / 16384;

    if constexpr (type == History::HH)
        history[board.sideToMove][from(move)][to(move)] += hhBonus;
    else if constexpr (type == History::CONST)
        consthist[board.pieceAtB(from(secondMove))][to(secondMove)][board.pieceAtB(from(move))]
                 [to(move)] += hhBonus;
}

template <History type>
void Search::updateHistory(Move bestmove, int bonus, int depth, Move *moves, int moveCount,
                           Stack *ss) {
    if constexpr (type == History::HH) {
        if (depth > 1) updateHistoryBonus<type>(bestmove, NO_MOVE, bonus);
    }

    if constexpr (type == History::CONST) {
        if (ss->ply > 0) {
            updateHistoryBonus<type>(bestmove, (ss - 1)->currentmove, bonus);
            if (ss->ply > 1) updateHistoryBonus<type>(bestmove, (ss - 2)->currentmove, bonus);
        }
    }

    for (int i = 0; i < moveCount; i++) {
        const Move move = moves[i];

        if constexpr (type == History::CONST) {
            if (ss->ply > 0) {
                updateHistoryBonus<type>(move, (ss - 1)->currentmove, -bonus);
                if (ss->ply > 1) updateHistoryBonus<type>(move, (ss - 2)->currentmove, -bonus);
            }
        } else
            updateHistoryBonus<type>(move, NO_MOVE, -bonus);
    }
}

void Search::updateAllHistories(Move bestMove, int depth, Move *quiets, int quietCount, Stack *ss) {
    int depthBonus = bonus(depth);

    counters[from((ss - 1)->currentmove)][to((ss - 1)->currentmove)] = bestMove;

    /********************
     * Update Quiet Moves
     *******************/
    if (board.pieceAtB(to(bestMove)) == None) {
        // update Killer Moves
        killerMoves[1][ss->ply] = killerMoves[0][ss->ply];
        killerMoves[0][ss->ply] = bestMove;

        updateHistory<History::HH>(bestMove, depthBonus, depth, quiets, quietCount, ss);

        int constbonus = std::min(4 * depth * depth * depth, 1500);
        updateHistory<History::CONST>(bestMove, constbonus, depth, quiets, quietCount, ss);
    }
}

template <Node node>
Score Search::qsearch(Score alpha, Score beta, Stack *ss) {
    if (limitReached()) return 0;

    /********************
     * Initialize various variables
     *******************/
    constexpr bool PvNode = node == PV;
    const Color color = board.sideToMove;
    const bool inCheck = board.isSquareAttacked(~color, board.KingSQ(color), board.All());

    Move bestMove = NO_MOVE;

    assert(alpha >= -VALUE_INFINITE && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));

    if (ss->ply >= MAX_PLY) return Eval::evaluation(board);

    /********************
     * Check for repetition or 50 move rule draw
     *******************/
    if (board.isRepetition(1 + PvNode)) return -1 + (nodes & 0x2);

    const Result state = board.isDrawn(inCheck);
    if (state != Result::NONE) return state == Result::LOST ? mated_in(ss->ply) : 0;

    /********************
     * Look up in the TT
     * Adjust alpha and beta for non PV Nodes.
     *******************/

    Move ttMove = NO_MOVE;
    bool ttHit = false;

    TEntry *tte = TTable.probeTT(ttHit, ttMove, board.hashKey);
    Score ttScore =
        ttHit && tte->score != VALUE_NONE ? scoreFromTT(tte->score, ss->ply) : Score(VALUE_NONE);
    // clang-format off
    if (    ttHit 
        &&  !PvNode 
        &&  ttScore != VALUE_NONE
        &&  tte->flag != NONEBOUND)
    {
        // clang-format on
        if (tte->flag == EXACTBOUND)
            return ttScore;
        else if (tte->flag == LOWERBOUND && ttScore >= beta)
            return ttScore;
        else if (tte->flag == UPPERBOUND && ttScore <= alpha)
            return ttScore;
    }

    Score bestValue = Eval::evaluation(board);

    if (bestValue >= beta) return bestValue;
    if (bestValue > alpha) alpha = bestValue;

    Movelist moves;
    MovePick<QSEARCH> mp(*this, ss, moves, ttMove);
    mp.stage = ttHit ? TT_MOVE : GENERATE;

    /********************
     * Search the moves
     *******************/
    Move move = NO_MOVE;
    while ((move = mp.nextMove()) != NO_MOVE) {
        PieceType captured = type_of_piece(board.pieceAtB(to(move)));

        if (bestValue > VALUE_TB_LOSS_IN_MAX_PLY) {
            // delta pruning, if the move + a large margin is still less then alpha we can safely
            // skip this
            // clang-format off
            if (    captured != NONETYPE 
                &&  !inCheck 
                &&  bestValue + 400 + piece_values[EG][captured] < alpha 
                &&  typeOf(move) != PROMOTION
                &&  board.nonPawnMat(color))
                // clang-format on
                continue;

            // see based capture pruning
            if (!inCheck && !board.see(move, 0)) continue;
        }

        nodes++;

        board.makeMove<true>(move);

        Score score = -qsearch<node>(-beta, -alpha, ss + 1);

        board.unmakeMove<false>(move);

        assert(score > -VALUE_INFINITE && score < VALUE_INFINITE);

        // update the best score
        if (score > bestValue) {
            bestValue = score;

            if (score > alpha) {
                alpha = score;
                bestMove = move;

                if (score >= beta) break;
            }
        }
    }

    /********************
     * store in the transposition table
     *******************/

    Flag b = bestValue >= beta ? LOWERBOUND : UPPERBOUND;

    if (!Threads.stop.load(std::memory_order_relaxed))
        TTable.storeEntry(0, scoreToTT(bestValue, ss->ply), b, board.hashKey, bestMove);

    assert(bestValue > -VALUE_INFINITE && bestValue < VALUE_INFINITE);
    return bestValue;
}

template <Node node>
Score Search::absearch(int depth, Score alpha, Score beta, Stack *ss) {
    if (limitReached()) return 0;

    /********************
     * Initialize various variables
     *******************/
    constexpr bool RootNode = node == Root;
    constexpr bool PvNode = node != NonPV;

    Color color = board.sideToMove;

    Score best = -VALUE_INFINITE;
    Score maxValue = VALUE_INFINITE;
    Move excludedMove = ss->excludedMove;

    const bool inCheck = board.isSquareAttacked(~color, board.KingSQ(color), board.All());
    bool improving;

    if (ss->ply >= MAX_PLY) return (ss->ply >= MAX_PLY && !inCheck) ? Eval::evaluation(board) : 0;

    pvLength[ss->ply] = ss->ply;

    /********************
     * Draw detection and mate pruning
     *******************/
    if (!RootNode) {
        if (board.isRepetition(1 + PvNode)) return -1 + (nodes & 0x2);

        const Result state = board.isDrawn(inCheck);
        if (state != Result::NONE) return state == Result::LOST ? mated_in(ss->ply) : 0;

        alpha = std::max(alpha, mated_in(ss->ply));
        beta = std::min(beta, mate_in(ss->ply + 1));
        if (alpha >= beta) return alpha;
    }

    /********************
     * Check extension
     *******************/

    if (inCheck) depth++;

    /********************
     * Enter qsearch
     *******************/
    if (depth <= 0) return qsearch<node>(alpha, beta, ss);

    assert(-VALUE_INFINITE <= alpha && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));
    assert(0 < depth && depth < MAX_PLY);

    (ss + 1)->excludedMove = NO_MOVE;

    /********************
     * Selective depth
     * Heighest depth a pvnode has reached
     *******************/
    if (PvNode && ss->ply > seldepth) seldepth = ss->ply;

    // Look up in the TT
    Move ttMove = NO_MOVE;
    bool ttHit = false;

    TEntry *tte = TTable.probeTT(ttHit, ttMove, board.hashKey);
    Score ttScore = ttHit ? scoreFromTT(tte->score, ss->ply) : Score(VALUE_NONE);

    /********************
     * Look up in the TT
     * Adjust alpha and beta for non PV nodes
     *******************/
    // clang-format off
    if (    !RootNode 
        &&  !excludedMove
        &&  !PvNode 
        &&  ttHit 
        &&  ttScore != VALUE_NONE
        &&  tte->depth >= depth 
        &&  (ss - 1)->currentmove != NULL_MOVE )
    {
        // clang-format on
        if (tte->flag == EXACTBOUND)
            return ttScore;
        else if (tte->flag == LOWERBOUND)
            alpha = std::max(alpha, ttScore);
        else if (tte->flag == UPPERBOUND)
            beta = std::min(beta, ttScore);
        if (alpha >= beta) return ttScore;
    }

    /********************
     *  Tablebase probing
     *******************/
    Score tbRes = RootNode ? Score(VALUE_NONE) : probeWDL();

    if (tbRes != VALUE_NONE) {
        Flag flag = NONEBOUND;
        tbhits++;

        switch (tbRes) {
            case VALUE_TB_WIN:
                tbRes = VALUE_MATE_IN_PLY - ss->ply - 1;
                flag = LOWERBOUND;
                break;
            case VALUE_TB_LOSS:
                tbRes = VALUE_MATED_IN_PLY + ss->ply + 1;
                flag = UPPERBOUND;
                break;
            default:
                tbRes = 0;
                flag = EXACTBOUND;
                break;
        }

        if (flag == EXACTBOUND || (flag == LOWERBOUND && tbRes >= beta) ||
            (flag == UPPERBOUND && tbRes <= alpha)) {
            TTable.storeEntry(depth + 6, scoreToTT(tbRes, ss->ply), flag, board.hashKey, NO_MOVE);
            return tbRes;
        }

        if (PvNode) {
            if (flag == LOWERBOUND) {
                best = tbRes;
                alpha = std::max(alpha, best);
            } else {
                maxValue = tbRes;
            }
        }
    }

    if (inCheck) {
        improving = false;
        ss->eval = VALUE_NONE;
        goto moves;
    }

    // Use the ttscore as a better evaluation of the position, other engines
    // typically have eval and staticEval. In Smallbrain its just eval.
    ss->eval = ttHit ? ttScore : Eval::evaluation(board);

    // improving boolean
    improving = (ss - 2)->eval != VALUE_NONE ? ss->eval > (ss - 2)->eval : false;

    if (RootNode) goto moves;

    /********************
     * Internal Iterative Reductions (IIR)
     *******************/
    if (depth >= 3 && !ttHit) depth--;

    if (PvNode && !ttHit) depth--;

    if (depth <= 0) return qsearch<PV>(alpha, beta, ss);

    // Skip early pruning in Pv Nodes

    if (PvNode) goto moves;

    /********************
     * Razoring
     *******************/
    if (depth < 3 && ss->eval + 129 < alpha) return qsearch<NonPV>(alpha, beta, ss);

    /********************
     * Reverse futility pruning
     *******************/
    if (std::abs(beta) < VALUE_TB_WIN_IN_MAX_PLY)
        if (depth < 7 && ss->eval - 64 * depth + 71 * improving >= beta) return beta;

    /********************
     * Null move pruning
     *******************/
    // clang-format off
    if (    board.nonPawnMat(color) 
        &&  !excludedMove
        &&  (ss - 1)->currentmove != NULL_MOVE 
        &&  depth >= 3 
        &&  ss->eval >= beta)
    {
        // clang-format on
        int R = 5 + std::min(4, depth / 5) + std::min(3, (ss->eval - beta) / 214);

        board.makeNullMove();
        (ss)->currentmove = NULL_MOVE;
        Score score = -absearch<NonPV>(depth - R, -beta, -beta + 1, ss + 1);
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
    Move bestMove = NO_MOVE;
    Move move = NO_MOVE;
    uint8_t quietCount = 0;
    uint8_t madeMoves = 0;
    bool doFullSearch = false;

    MovePick<ABSEARCH> mp(*this, ss, moves, searchmoves, RootNode, ttMove,
                          ttHit ? TT_MOVE : GENERATE);

    /********************
     * Movepicker fetches the next move that we should search.
     * It is very important to return the likely best move first,
     * since then we get many cut offs.
     *******************/
    while ((move = mp.nextMove()) != NO_MOVE) {
        if (move == excludedMove) continue;

        madeMoves++;

        int extension = 0;

        const bool capture = board.pieceAtB(to(move)) != None;

        /********************
         * Various pruning techniques.
         *******************/
        if (!RootNode && best > VALUE_TB_LOSS_IN_MAX_PLY) {
            // clang-format off
            if (capture)
            {
                // SEE pruning
                if (    depth < 6 
                    &&  !board.see(move, -(depth * 92)))
                    continue;
            }
            else
            {
                // late move pruning/movecount pruning
                if (    !inCheck 
                    &&  !PvNode 
                    &&  typeOf(move) != PROMOTION
                    &&  depth <= 5
                    &&  quietCount > (4 + depth * depth))

                    continue;
                // SEE pruning
                if (    depth < 7 
                    &&  !board.see(move, -(depth * 93)))
                    continue;
            }
            // clang-format on
        }

        // clang-format off
        // Singular extensions
        if (    !RootNode 
            &&  depth >= 8 
            &&  ttHit  // ttScore cannot be VALUE_NONE!
            &&  ttMove == move 
            &&  !excludedMove
            &&  std::abs(ttScore) < 10000
            &&  tte->flag & LOWERBOUND
            &&  tte->depth >= depth - 3)
        {
            // clang-format on
            int singularBeta = ttScore - 3 * depth;
            int singularDepth = (depth - 1) / 2;

            ss->excludedMove = move;
            int value = absearch<NonPV>(singularDepth, singularBeta - 1, singularBeta, ss);
            ss->excludedMove = NO_MOVE;

            if (value < singularBeta)
                extension = 1;
            else if (singularBeta >= beta)
                return singularBeta;
        }

        int newDepth = depth - 1 + extension;

        /********************
         * Print currmove information.
         *******************/
        // clang-format off
        if (    id == 0 
            &&  RootNode 
            &&  normalSearch 
            &&  !Threads.stop.load(std::memory_order_relaxed) 
            &&  getTime() > 10000)
            std::cout << "info depth " << depth - inCheck 
                      << " currmove " << uciMove(move, board.chess960)
                      << " currmovenumber " << signed(madeMoves) << std::endl;
        // clang-format on

        /********************
         * Play the move on the internal board.
         *******************/
        nodes++;
        board.makeMove<true>(move);

        U64 nodeCount = nodes;
        ss->currentmove = move;

        /********************
         * Late move reduction, later moves will be searched
         * with a reduced depth, if they beat alpha we search again at
         * full depth.
         *******************/
        if (depth >= 3 && !inCheck && madeMoves > 3 + 2 * PvNode) {
            int rdepth = reductions[depth][madeMoves];

            rdepth -= id % 2;

            rdepth += improving;

            rdepth -= PvNode;

            rdepth -= capture;

            rdepth = std::clamp(newDepth - rdepth, 1, newDepth + 1);

            score = -absearch<NonPV>(rdepth, -alpha - 1, -alpha, ss + 1);
            doFullSearch = score > alpha && rdepth < newDepth;
        } else
            doFullSearch = !PvNode || madeMoves > 1;

        /********************
         * Do a full research if lmr failed or lmr was skipped
         *******************/
        if (doFullSearch) {
            score = -absearch<NonPV>(newDepth, -alpha - 1, -alpha, ss + 1);
        }

        /********************
         * PVS Search
         * We search the first move or PV Nodes that are inside the bounds
         * with a full window at (more or less) full depth.
         *******************/
        if (PvNode && ((score > alpha && score < beta) || madeMoves == 1)) {
            score = -absearch<PV>(newDepth, -beta, -alpha, ss + 1);
        }

        board.unmakeMove<false>(move);

        assert(score > -VALUE_INFINITE && score < VALUE_INFINITE);

        /********************
         * Node count logic used for time control.
         *******************/
        if (id == 0) spentEffort[from(move)][to(move)] += nodes - nodeCount;

        /********************
         * Score beat best -> update PV and Bestmove.
         *******************/
        if (score > best) {
            best = score;

            if (score > alpha) {
                alpha = score;
                bestMove = move;

                // update the PV
                pvTable[ss->ply][ss->ply] = move;

                for (int next_ply = ss->ply + 1; next_ply < pvLength[ss->ply + 1]; next_ply++) {
                    pvTable[ss->ply][next_ply] = pvTable[ss->ply + 1][next_ply];
                }

                pvLength[ss->ply] = pvLength[ss->ply + 1];

                /********************
                 * Score beat beta -> update histories and break.
                 *******************/
                if (score >= beta) {
                    // update history heuristic
                    updateAllHistories(bestMove, depth, quiets, quietCount, ss);
                    break;
                }
            }
        }
        if (!capture) quiets[quietCount++] = move;
    }

    /********************
     * If the move list is empty, we are in checkmate or stalemate.
     *******************/
    if (madeMoves == 0) best = excludedMove ? alpha : inCheck ? mated_in(ss->ply) : 0;

    if (PvNode) best = std::min(best, maxValue);

    /********************
     * Store an TEntry in the Transposition Table.
     *******************/

    // Transposition table flag
    Flag b = best >= beta ? LOWERBOUND : (PvNode && bestMove != NO_MOVE ? EXACTBOUND : UPPERBOUND);

    if (!excludedMove && !Threads.stop.load(std::memory_order_relaxed))
        TTable.storeEntry(depth, scoreToTT(best, ss->ply), b, board.hashKey, bestMove);

    assert(best > -VALUE_INFINITE && best < VALUE_INFINITE);
    return best;
}

Score Search::aspirationSearch(int depth, Score prevEval, Stack *ss) {
    Score alpha = -VALUE_INFINITE;
    Score beta = VALUE_INFINITE;
    int delta = 30;

    Score result = 0;

    /********************
     * We search moves after depth 9 with an aspiration Window.
     * A small window around the previous evaluation enables us
     * to have a smaller search tree. We dont do this for the first
     * few depths because these have quite unstable evaluation which
     * would lead to many researches.
     *******************/
    if (depth >= 9) {
        alpha = prevEval - delta;
        beta = prevEval + delta;
    }

    while (true) {
        if (alpha < -3500) alpha = -VALUE_INFINITE;
        if (beta > 3500) beta = VALUE_INFINITE;

        result = absearch<Root>(depth, alpha, beta, ss);

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

    if (id == 0 && normalSearch) {
        uciOutput(result, depth, seldepth, Threads.getNodes(), Threads.getTbHits(), getTime(),
                  getPV(), TTable.hashfull());
    }

    return result;
}

SearchResult Search::iterativeDeepening() {
    Move bestmove = NO_MOVE;
    SearchResult sr;

    Score result = -VALUE_INFINITE;
    int depth = 1;

    Stack stack[MAX_PLY + 4], *ss = stack + 2;

    spentEffort.reset();

    for (int i = -2; i <= MAX_PLY + 1; ++i) {
        (ss + i)->ply = i;
        (ss + i)->currentmove = NO_MOVE;
        (ss + i)->eval = 0;
        (ss + i)->excludedMove = NO_MOVE;
    }

    int bestmoveChanges = 0;
    int evalAverage = 0;

    /********************
     * Iterative Deepening Loop.
     *******************/
    for (depth = 1; depth <= limit.depth; depth++) {
        seldepth = 0;

        auto previousResult = result;
        result = aspirationSearch(depth, result, ss);
        evalAverage += result;

        if (limitReached()) break;

        // only mainthread manages time control
        if (id != 0) continue;

        sr.score = result;

        if (bestmove != pvTable[0][0]) bestmoveChanges++;

        bestmove = pvTable[0][0];

        // limit type time
        if (limit.time.optimum != 0) {
            auto now = getTime();

            // node count time management (https://github.com/Luecx/Koivisto 's idea)
            int effort = (spentEffort[from(bestmove)][to(bestmove)] * 100) / nodes;
            if (depth > 10 && limit.time.optimum * (110 - std::min(effort, 90)) / 100 < now) break;

            // increase optimum time if score is increasing
            if (result + 30 < evalAverage / depth) limit.time.optimum *= 1.10;

            // increase optimum time if score is dropping
            if (result > -200 && result - previousResult < -20) limit.time.optimum *= 1.10;

            // increase optimum time if bestmove fluctates
            if (bestmoveChanges > 4) limit.time.optimum = limit.time.maximum * 0.75;

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
    if (depth == 1) bestmove = pvTable[0][0];

    /********************
     * Mainthread prints bestmove.
     * Allowprint is disabled in data generation
     *******************/
    if (id == 0 && normalSearch) {
        std::cout << "bestmove " << uciMove(bestmove, board.chess960) << std::endl;
        Threads.stop = true;
    }

    print_mean();

    sr.move = bestmove;
    return sr;
}

void Search::reset() {
    consthist.reset();
    counters.reset();
    history.reset();
    killerMoves.reset();
    spentEffort.reset();
    pvLength.reset();
    pvTable.reset();

    nodes = 0;
    tbhits = 0;
}

void Search::startThinking() {
    /********************
     * Various Limits that only the MainThread needs to know
     * and initialise.
     *******************/
    t0 = TimePoint::now();
    checkTime = 0;

    /********************
     * Play dtz move when time is limited
     *******************/
    if (id == 0 && limit.time.optimum != 0) {
        Move dtzMove = probeDTZ();
        if (dtzMove != NO_MOVE) {
            std::cout << "bestmove " << uciMove(dtzMove, board.chess960) << std::endl;
            Threads.stop = true;
            return;
        }
    }

    iterativeDeepening();
}

bool Search::limitReached() {
    if (normalSearch && Threads.stop.load(std::memory_order_relaxed)) return true;

    if (id != 0) return false;

    if (limit.nodes != 0 && nodes >= limit.nodes) return true;

    if (--checkTime > 0) return false;

    checkTime = 2047;

    if (limit.time.maximum != 0) {
        auto ms = getTime();

        if (ms >= limit.time.maximum) {
            Threads.stop = true;

            return true;
        }
    }
    return false;
}

std::string Search::getPV() {
    std::stringstream ss;

    for (int i = 0; i < pvLength[0]; i++) {
        ss << " " << uciMove(pvTable[0][i], board.chess960);
    }

    return ss.str();
}

int64_t Search::getTime() {
    auto t1 = TimePoint::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

Score Search::probeWDL() {
    if (!normalSearch || !useTB) return VALUE_NONE;

    U64 white = board.Us<White>();
    U64 black = board.Us<Black>();

    if (popcount(white | black) > (signed)TB_LARGEST) return VALUE_NONE;

    Square ep = board.enPassantSquare <= 63 ? board.enPassantSquare : Square(0);

    unsigned TBresult = tb_probe_wdl(
        white, black, board.pieces<WhiteKing>() | board.pieces<BlackKing>(),
        board.pieces<WhiteQueen>() | board.pieces<BlackQueen>(),
        board.pieces<WhiteRook>() | board.pieces<BlackRook>(),
        board.pieces<WhiteBishop>() | board.pieces<BlackBishop>(),
        board.pieces<WhiteKnight>() | board.pieces<BlackKnight>(),
        board.pieces<WhitePawn>() | board.pieces<BlackPawn>(), board.halfMoveClock,
        board.castlingRights.hasCastlingRight(board.sideToMove), ep, board.sideToMove == White);

    if (TBresult == TB_LOSS) {
        return VALUE_TB_LOSS;
    } else if (TBresult == TB_WIN) {
        return VALUE_TB_WIN;
    } else if (TBresult == TB_DRAW || TBresult == TB_BLESSED_LOSS || TBresult == TB_CURSED_WIN) {
        return Score(0);
    }
    return VALUE_NONE;
}

Move Search::probeDTZ() {
    U64 white = board.Us<White>();
    U64 black = board.Us<Black>();
    if (popcount(white | black) > (signed)TB_LARGEST) return NO_MOVE;

    Square ep = board.enPassantSquare <= 63 ? board.enPassantSquare : Square(0);

    unsigned TBresult = tb_probe_root(
        white, black, board.pieces<WhiteKing>() | board.pieces<BlackKing>(),
        board.pieces<WhiteQueen>() | board.pieces<BlackQueen>(),
        board.pieces<WhiteRook>() | board.pieces<BlackRook>(),
        board.pieces<WhiteBishop>() | board.pieces<BlackBishop>(),
        board.pieces<WhiteKnight>() | board.pieces<BlackKnight>(),
        board.pieces<WhitePawn>() | board.pieces<BlackPawn>(), board.halfMoveClock,
        board.castlingRights.hasCastlingRight(board.sideToMove), ep, board.sideToMove == White,
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
                uciOutput(s, static_cast<int>(dtz), 1, Threads.getNodes(), Threads.getTbHits(),
                          getTime(), " " + uciMove(move, board.chess960), TTable.hashfull());
                return move;
            }
        }
    }

    std::cout << " something went wrong playing dtz :" << promoTranslation[promo] << " : " << promo
              << " : " << std::endl;
    exit(0);

    return NO_MOVE;
}
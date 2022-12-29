#include "search.h"
#include "movepick.h"

#include <algorithm> // clamp
#include <cmath>

// Initialize reduction table
int reductions[MAX_PLY][MAX_MOVES];
void init_reductions()
{
    for (int depth = 0; depth < MAX_PLY; depth++)
    {
        for (int moves = 0; moves < MAX_MOVES; moves++)
            reductions[depth][moves] = 1 + log(depth) * log(moves) / 1.75;
    }
}

int bonus(int depth)
{
    return std::min(2000, depth * 150);
}

template <Movetype type> void Search::updateHistoryBonus(Move move, int bonus, ThreadData *td)
{
    int hhBonus = bonus - getHistory<type>(move, td) * std::abs(bonus) / 16384;
    if constexpr (type == Movetype::QUIET)
        td->history[td->board.sideToMove][from(move)][to(move)] += hhBonus;
}

template <Movetype type>
void Search::updateHistory(Move bestmove, int bonus, int depth, Movelist &movelist, ThreadData *td)
{
    if (depth > 1)
        updateHistoryBonus<type>(bestmove, bonus, td);

    for (int i = 0; i < movelist.size; i++)
    {
        const Move move = movelist[i].move;
        if (move == bestmove)
            continue;

        updateHistoryBonus<type>(move, -bonus, td);
    }
}

void Search::updateAllHistories(Move bestMove, Score best, Score beta, int depth, Movelist &quietMoves, ThreadData *td,
                                Stack *ss)
{
    if (best < beta)
        return;

    int depthBonus = bonus(depth);

    /********************
     * Update Quiet Moves
     *******************/
    if (td->board.pieceAtB(to(bestMove)) == None)
    {
        // update Killer Moves
        td->killerMoves[1][ss->ply] = td->killerMoves[0][ss->ply];
        td->killerMoves[0][ss->ply] = bestMove;

        updateHistory<Movetype::QUIET>(bestMove, depthBonus, depth, quietMoves, td);
    }
}

template <Node node> Score Search::qsearch(Score alpha, Score beta, Stack *ss, ThreadData *td)
{
    if (exitEarly(td->nodes, td->id))
        return 0;

    /********************
     * Initialize various variables
     *******************/
    constexpr bool PvNode = node == PV;
    const Color color = td->board.sideToMove;
    const bool inCheck = td->board.isSquareAttacked(~color, td->board.KingSQ(color));

    Move bestMove = NO_MOVE;

    assert(alpha >= -VALUE_INFINITE && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));

    /********************
     * Check for repetition or 50 move rule draw
     *******************/

    if (td->board.isRepetition() || ss->ply >= MAX_PLY)
        return (ss->ply >= MAX_PLY && !inCheck) ? Eval::evaluation(td->board) : 0;

    Score bestValue = Eval::evaluation(td->board);
    if (bestValue >= beta)
        return bestValue;
    if (bestValue > alpha)
        alpha = bestValue;

    /********************
     * Look up in the TT
     * Adjust alpha and beta for non PV Nodes.
     *******************/

    Move ttMove = NO_MOVE;
    bool ttHit = false;

    TEntry *tte = TTable.probeTT(ttHit, ttMove, td->board.hashKey);
    Score ttScore = ttHit ? scoreFromTT(tte->score, ss->ply) : Score(VALUE_NONE);

    if (ttHit && !PvNode && ttScore != VALUE_NONE && tte->flag != NONEBOUND)
    {
        if (tte->flag == EXACTBOUND)
            return ttScore;
        else if (tte->flag == LOWERBOUND && ttScore >= beta)
            return ttScore;
        else if (tte->flag == UPPERBOUND && ttScore <= alpha)
            return ttScore;
    }

    MovePick<QSEARCH> mp(td, ss, ss->moves, ttMove);
    mp.stage = ttHit ? TT_MOVE : GENERATE;

    /********************
     * Search the moves
     *******************/
    Move move = NO_MOVE;
    while ((move = mp.nextMove()) != NO_MOVE)
    {
        PieceType captured = type_of_piece(td->board.pieceAtB(to(move)));

        if (bestValue > VALUE_TB_LOSS_IN_MAX_PLY)
        {
            // delta pruning, if the move + a large margin is still less then alpha we can safely skip this
            if (captured != NONETYPE && !inCheck && bestValue + 400 + piece_values[EG][captured] < alpha &&
                !promoted(move) && td->board.nonPawnMat(color))
                continue;

            // see based capture pruning
            if (!inCheck && !td->board.see(move, 0))
                continue;
        }

        td->nodes++;

        td->board.makeMove<true>(move);

        Score score = -qsearch<node>(-beta, -alpha, ss + 1, td);

        td->board.unmakeMove<false>(move);

        assert(score > -VALUE_INFINITE && score < VALUE_INFINITE);

        // update the best score
        if (score > bestValue)
        {
            bestValue = score;

            if (score > alpha)
            {
                alpha = score;
                bestMove = move;

                if (score >= beta)
                    break;
            }
        }
    }

    /********************
     * store in the transposition table
     *******************/

    // Transposition table flag
    Flag b = bestValue >= beta ? LOWERBOUND : UPPERBOUND;

    if (!stopped.load(std::memory_order_relaxed))
        TTable.storeEntry(0, scoreToTT(bestValue, ss->ply), b, td->board.hashKey, bestMove);

    assert(bestValue > -VALUE_INFINITE && bestValue < VALUE_INFINITE);
    return bestValue;
}

template <Node node> Score Search::absearch(int depth, Score alpha, Score beta, Stack *ss, ThreadData *td)
{
    if (exitEarly(td->nodes, td->id))
        return 0;

    /********************
     * Initialize various variables
     *******************/
    constexpr bool RootNode = node == Root;
    constexpr bool PvNode = node != NonPV;

    Color color = td->board.sideToMove;

    Score best = -VALUE_INFINITE;
    Score maxValue = VALUE_MATE;
    Score staticEval;

    bool improving;
    bool inCheck = td->board.isSquareAttacked(~color, td->board.KingSQ(color));

    ss->eval = VALUE_NONE;

    if (ss->ply >= MAX_PLY)
        return (ss->ply >= MAX_PLY && !inCheck) ? Eval::evaluation(td->board) : 0;

    td->pvLength[ss->ply] = ss->ply;

    /********************
     * Draw detection and mate pruning
     *******************/
    if (!RootNode)
    {
        if (td->board.halfMoveClock >= 100)
        {
            if (inCheck)
                return !Movegen::hasLegalMoves(td->board) ? mated_in(ss->ply) : 0;
            return 0;
        }

        if (td->board.isRepetition(1 + 2 * (node == PV)) && (ss - 1)->currentmove != NULL_MOVE)
            return -3 + (td->nodes & 7);

        alpha = std::max(alpha, mated_in(ss->ply));
        beta = std::min(beta, mate_in(ss->ply + 1));
        if (alpha >= beta)
            return alpha;
    }

    /********************
     * Check extension
     *******************/

    if (inCheck)
        depth++;

    /********************
     * Enter qsearch
     *******************/
    if (depth <= 0)
        return qsearch<node>(alpha, beta, ss, td);

    assert(-VALUE_INFINITE <= alpha && alpha < beta && beta <= VALUE_INFINITE);
    assert(PvNode || (alpha == beta - 1));
    assert(0 < depth && depth < MAX_PLY);

    /********************
     * Selective depth (heighest depth we have ever reached)
     *******************/

    if (ss->ply > td->seldepth)
        td->seldepth = ss->ply;

    // Look up in the TT
    Move ttMove = NO_MOVE;
    bool ttHit = false;

    TEntry *tte = TTable.probeTT(ttHit, ttMove, td->board.hashKey);
    Score ttScore = ttHit ? scoreFromTT(tte->score, ss->ply) : Score(VALUE_NONE);
    /********************
     * Look up in the TT
     * Adjust alpha and beta for non PV nodes
     *******************/

    if (!RootNode && !PvNode && ttHit && tte->depth >= depth && (ss - 1)->currentmove != NULL_MOVE &&
        ttScore != VALUE_NONE)
    {
        if (tte->flag == EXACTBOUND)
            return ttScore;
        else if (tte->flag == LOWERBOUND)
            alpha = std::max(alpha, ttScore);
        else if (tte->flag == UPPERBOUND)
            beta = std::min(beta, ttScore);
        if (alpha >= beta)
            return ttScore;
    }

    /********************
     *  Tablebase probing
     *******************/

    if (!RootNode && td->allowPrint && td->useTB)
    {
        Score tbRes = probeTB(td);

        if (tbRes != VALUE_NONE)
        {
            Flag flag = NONEBOUND;
            td->tbhits++;

            switch (tbRes)
            {
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

            if (flag == EXACTBOUND || (flag == LOWERBOUND && tbRes >= beta) || (flag == UPPERBOUND && tbRes <= alpha))
            {
                TTable.storeEntry(depth + 6, scoreToTT(tbRes, ss->ply), flag, td->board.hashKey, NO_MOVE);
                return tbRes;
            }

            if (PvNode)
            {
                if (flag == LOWERBOUND)
                {
                    best = tbRes;
                    alpha = std::max(alpha, best);
                }
                else
                {
                    maxValue = tbRes;
                }
            }
        }
    }

    if (inCheck)
    {
        improving = false;
        staticEval = VALUE_NONE;
        goto moves;
    }

    // use tt eval for a better staticEval
    ss->eval = staticEval = ttHit ? tte->score : Eval::evaluation(td->board);

    // improving boolean
    improving = (ss - 2)->eval != VALUE_NONE ? staticEval > (ss - 2)->eval : false;

    if (RootNode)
        goto moves;

    /********************
     * Internal Iterative Deepening
     *******************/
    if (depth >= 4 && !ttHit)
        depth--;

    if (PvNode && !ttHit)
        depth--;

    if (depth <= 0)
        return qsearch<PV>(alpha, beta, ss, td);

    if (PvNode)
        goto moves;

    /********************
     * Razoring
     *******************/
    if (depth < 3 && staticEval + 120 < alpha)
        return qsearch<NonPV>(alpha, beta, ss, td);

    /********************
     * Reverse futility pruning
     *******************/
    if (std::abs(beta) < VALUE_TB_WIN_IN_MAX_PLY)
        if (depth < 6 && staticEval - 61 * depth + 73 * improving >= beta)
            return beta;

    /********************
     * Null move pruning
     *******************/
    if (td->board.nonPawnMat(color) && (ss - 1)->currentmove != NULL_MOVE && depth >= 3 && staticEval >= beta)
    {
        int R = 5 + std::min(4, depth / 5) + std::min(3, (staticEval - beta) / 214);

        td->board.makeNullMove();
        (ss)->currentmove = NULL_MOVE;
        Score score = -absearch<NonPV>(depth - R, -beta, -beta + 1, ss + 1, td);
        td->board.unmakeNullMove();
        if (score >= beta)
        {
            // dont return mate scores
            if (score >= VALUE_TB_WIN_IN_MAX_PLY)
                score = beta;

            return score;
        }
    }

moves:
    // reset movelists
    ss->quietMoves.size = 0;

    Score score = VALUE_NONE;
    Move bestMove = NO_MOVE;
    Move move = NO_MOVE;
    uint8_t madeMoves = 0;
    bool doFullSearch = false;

    MovePick<ABSEARCH> mp(td, ss, ss->moves, ttMove);
    mp.stage = ttHit ? TT_MOVE : GENERATE;

    /********************
     * Movepicker fetches the next move that we should search.
     * It is very important to return the likely best move first,
     * since then we get many cut offs.
     *******************/
    while ((move = mp.nextMove()) != NO_MOVE)
    {
        madeMoves++;

        int extension = 0;

        const bool capture = td->board.pieceAtB(to(move)) != None;

        int newDepth = depth - 1 + extension;

        /********************
         * Various pruning techniques.
         *******************/
        if (!RootNode && best > VALUE_TB_LOSS_IN_MAX_PLY)
        {
            // late move pruning/movecount pruning
            if (!capture && !inCheck && !PvNode && !promoted(move) && depth <= 4 &&
                ss->quietMoves.size > (4 + depth * depth))
                continue;

            // SEE pruning
            if (depth < 6 && !td->board.see(move, -(depth * 97)))
                continue;
        }

        td->nodes++;

        /********************
         * Print currmove information.
         *******************/
        if (td->id == 0 && RootNode && !stopped.load(std::memory_order_relaxed) && getTime() > 10000 && td->allowPrint)
            std::cout << "info depth " << depth - inCheck << " currmove " << uciRep(td->board, move)
                      << " currmovenumber " << signed(madeMoves) << std::endl;

        /********************
         * Play the move on the internal board.
         *******************/
        td->board.makeMove<true>(move);

        U64 nodeCount = td->nodes;
        ss->currentmove = move;

        /********************
         * Late move reduction, later moves will be searched
         * with a reduced depth, if they beat alpha we search again at
         * full depth.
         *******************/
        if (depth >= 3 && !inCheck && madeMoves > 3 + 2 * PvNode)
        {
            int rdepth = reductions[depth][madeMoves];

            rdepth -= td->id % 2;

            rdepth += improving;

            rdepth -= PvNode;

            rdepth = std::clamp(newDepth - rdepth, 1, newDepth + 1);

            score = -absearch<NonPV>(rdepth, -alpha - 1, -alpha, ss + 1, td);
            doFullSearch = score > alpha && rdepth < newDepth;
        }
        else
            doFullSearch = !PvNode || madeMoves > 1;

        /********************
         * Do a full research if lmr failed or lmr was skipped
         *******************/
        if (doFullSearch)
        {
            score = -absearch<NonPV>(newDepth, -alpha - 1, -alpha, ss + 1, td);
        }

        /********************
         * PVS Search
         * We search the first move or PV Nodes that are inside the bounds
         * with a full window at (more or less) full depth.
         *******************/
        if (PvNode && ((score > alpha && score < beta) || madeMoves == 1))
        {
            score = -absearch<PV>(newDepth, -beta, -alpha, ss + 1, td);
        }

        td->board.unmakeMove<false>(move);

        assert(score > -VALUE_INFINITE && score < VALUE_INFINITE);

        /********************
         * Node count logic used for time control.
         *******************/
        if (td->id == 0)
            spentEffort[from(move)][to(move)] += td->nodes - nodeCount;

        /********************
         * Score beat best -> update PV and Bestmove.
         *******************/
        if (score > best)
        {
            best = score;

            // update the PV
            td->pvTable[ss->ply][ss->ply] = move;
            for (int next_ply = ss->ply + 1; next_ply < td->pvLength[ss->ply + 1]; next_ply++)
            {
                td->pvTable[ss->ply][next_ply] = td->pvTable[ss->ply + 1][next_ply];
            }
            td->pvLength[ss->ply] = td->pvLength[ss->ply + 1];

            if (score > alpha)
            {
                alpha = score;
                bestMove = move;

                /********************
                 * Score beat beta -> update histories and break.
                 *******************/
                if (score >= beta)
                {
                    // update history heuristic
                    updateAllHistories(bestMove, best, beta, depth, ss->quietMoves, td, ss);
                    break;
                }
            }
        }
        if (!capture)
            ss->quietMoves.Add(move);
    }

    /********************
     * If the move list is empty, we are in checkmate or stalemate.
     *******************/
    if (madeMoves == 0)
        return inCheck ? mated_in(ss->ply) : 0;

    best = std::min(best, maxValue);

    /********************
     * Store an TEntry in the Transposition Table.
     *******************/

    // Transposition table flag
    Flag b = best >= beta ? LOWERBOUND : (PvNode && bestMove != NO_MOVE ? EXACTBOUND : UPPERBOUND);
    if (!stopped.load(std::memory_order_relaxed))
        TTable.storeEntry(depth, scoreToTT(best, ss->ply), b, td->board.hashKey, bestMove);

    assert(best > -VALUE_INFINITE && best < VALUE_INFINITE);
    return best;
}

Score Search::aspirationSearch(int depth, Score prev_eval, Stack *ss, ThreadData *td)
{
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
    if (depth >= 9)
    {
        alpha = prev_eval - delta;
        beta = prev_eval + delta;
    }

    while (true)
    {
        if (alpha < -3500)
            alpha = -VALUE_INFINITE;
        if (beta > 3500)
            beta = VALUE_INFINITE;
        result = absearch<Root>(depth, alpha, beta, ss, td);

        if (stopped.load(std::memory_order_relaxed))
            return 0;

        if (td->id == 0 && limit.nodes != 0 && td->nodes >= limit.nodes)
            return 0;

        /********************
         * Increase the bounds because the score was outside of them or
         * break in case it was a EXACTBOUND result.
         *******************/
        if (result <= alpha)
        {
            beta = (alpha + beta) / 2;
            alpha = std::max(alpha - delta, -(static_cast<int>(VALUE_INFINITE)));
            delta += delta / 2;
        }
        else if (result >= beta)
        {
            beta = std::min(beta + delta, static_cast<int>(VALUE_INFINITE));
            delta += delta / 2;
        }
        else
        {
            break;
        }
    }

    if (td->id == 0 && td->allowPrint)
        uciOutput(result, depth, td->seldepth, getNodes(), getTbHits(), getTime(), getPV(), TTable.hashfull());

    return result;
}

SearchResult Search::iterativeDeepening(Limits lim, int threadId)
{
    /********************
     * Various Limits that only the main Thread needs to know
     * and initialise.
     *******************/
    if (threadId == 0)
    {
        t0 = TimePoint::now();

        limit = lim;
        checkTime = 0;
    }

    Move bestmove = NO_MOVE;
    SearchResult sr;

    Score result = -VALUE_INFINITE;
    int depth = 1;

    Stack stack[MAX_PLY + 4], *ss = stack + 2;
    spentEffort.fill({});

    for (int i = -2; i <= MAX_PLY + 1; ++i)
    {
        (ss + i)->ply = i;
        (ss + i)->moves.size = 0;
        (ss + i)->currentmove = Move(0);
        (ss + i)->eval = 0;
    }

    ThreadData *td = &tds[threadId];
    td->nodes = 0;
    td->tbhits = 0;
    td->seldepth = 0;

    int bestmoveChanges = 0;
    int evalAverage = 0;

    /********************
     * Iterative Deepening Loop.
     *******************/
    for (depth = 1; depth <= lim.depth; depth++)
    {
        td->seldepth = 0;
        result = aspirationSearch(depth, result, ss, td);
        evalAverage += result;

        if (exitEarly(td->nodes, td->id))
            break;

        // only mainthread manages time control
        if (threadId != 0)
            continue;

        sr.score = result;

        if (bestmove != td->pvTable[0][0])
            bestmoveChanges++;

        bestmove = td->pvTable[0][0];

        // limit type time
        if (lim.time.optimum != 0)
        {
            auto now = getTime();

            // node count time management (https://github.com/Luecx/Koivisto 's idea)
            int effort = (spentEffort[from(bestmove)][to(bestmove)] * 100) / td->nodes;
            if (depth > 10 && lim.time.optimum * (110 - std::min(effort, 90)) / 100 < now)
                break;

            if (result + 30 < evalAverage / depth)
                lim.time.optimum *= 1.10;

            // stop if we have searched for more than 75% of our max time.
            if (bestmoveChanges > 4)
                lim.time.optimum = lim.time.maximum * 0.75;
            else if (depth > 10 && now * 10 > lim.time.optimum * 6)
                break;
        }
    }

    /********************
     * Dont stop analysis in infinite mode when max depth is reached
     * wait for uci stop or quit
     *******************/
    while (td->allowPrint && depth == MAX_PLY + 1 && lim.nodes == 0 && lim.time.optimum == 0 &&
           !stopped.load(std::memory_order_relaxed))
    {
    }

    /********************
     * In case the depth was 1 make sure we have at least a bestmove.
     *******************/
    if (depth == 1)
        bestmove = td->pvTable[0][0];

    /********************
     * Mainthread prints bestmove.
     * Allowprint is disabled in data generation
     *******************/
    if (threadId == 0 && td->allowPrint)
    {
        std::cout << "bestmove " << uciRep(td->board, bestmove) << std::endl;
        stopped = true;
    }
    print_mean();

    sr.move = bestmove;
    return sr;
}

void Search::startThinking(Board board, int workers, Limits limit, bool useTB)
{
    /********************
     * If we dont have previous data create default data
     *******************/
    ThreadData td;
    if (!tds.size())
    {
        tds.emplace_back(td);
    }
    // Save old data
    else
    {
        td = tds[0];
    }

    td.nodes = 0;
    td.board = board;
    td.useTB = useTB;

    tds.clear();

    for (int i = 0; i < workers; i++)
    {
        td.id = i;
        tds.emplace_back(td);
    }

    /********************
     * Play dtz move when time is limited
     *******************/
    if (limit.time.optimum != 0)
    {
        Move dtzMove = probeDTZ(board);
        if (dtzMove != NO_MOVE)
        {
            std::cout << "bestmove " << uciRep(board, dtzMove) << std::endl;
            stopped = true;
            return;
        }
    }

    /********************
     * Start main thread right away
     *******************/
    threads.emplace_back(&Search::iterativeDeepening, this, limit, 0);

    /********************
     * Launch helper threads
     *******************/
    for (int i = 1; i < workers; i++)
    {
        threads.emplace_back(&Search::iterativeDeepening, this, limit, i);
    }
}

bool Search::exitEarly(uint64_t nodes, int ThreadId)
{
    if (stopped.load(std::memory_order_relaxed))
        return true;
    if (ThreadId != 0)
        return false;
    if (limit.nodes != 0 && nodes >= limit.nodes)
        return true;

    if (--checkTime > 0)
        return false;
    checkTime = 2047;

    if (limit.time.maximum != 0)
    {
        auto ms = getTime();
        if (ms >= limit.time.maximum)
        {
            stopped = true;
            return true;
        }
    }
    return false;
}

std::string Search::getPV()
{
    std::stringstream ss;

    for (int i = 0; i < tds[0].pvLength[0]; i++)
    {
        ss << " " << uciRep(tds[0].board, tds[0].pvTable[0][i]);
    }
    return ss.str();
}

long long Search::getTime()
{
    auto t1 = TimePoint::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

uint64_t Search::getNodes()
{
    uint64_t nodes = 0;
    for (const auto &td : tds)
    {
        nodes += td.nodes;
    }
    return nodes;
}

uint64_t Search::getTbHits()
{
    uint64_t nodes = 0;
    for (const auto &td : tds)
    {
        nodes += td.tbhits;
    }
    return nodes;
}

Score Search::probeTB(ThreadData *td)
{
    U64 white = td->board.Us<White>();
    U64 black = td->board.Us<Black>();

    if (popcount(white | black) > (signed)TB_LARGEST)
        return VALUE_NONE;

    Square ep = td->board.enPassantSquare <= 63 ? td->board.enPassantSquare : Square(0);

    unsigned TBresult;
    TBresult = tb_probe_wdl(
        white, black, td->board.Kings<White>() | td->board.Kings<Black>(),
        td->board.Queens<White>() | td->board.Queens<Black>(), td->board.Rooks<White>() | td->board.Rooks<Black>(),
        td->board.Bishops<White>() | td->board.Bishops<Black>(),
        td->board.Knights<White>() | td->board.Knights<Black>(), td->board.Pawns<White>() | td->board.Pawns<Black>(),
        td->board.halfMoveClock, td->board.castlingRights, ep,
        td->board.sideToMove == White); //  * - turn: true=white, false=black

    if (TBresult == TB_LOSS)
    {
        return VALUE_TB_LOSS;
    }
    else if (TBresult == TB_WIN)
    {
        return VALUE_TB_WIN;
    }
    else if (TBresult == TB_DRAW || TBresult == TB_BLESSED_LOSS || TBresult == TB_CURSED_WIN)
    {
        return Score(0);
    }
    return VALUE_NONE;
}

Move Search::probeDTZ(Board &board)
{
    U64 white = board.Us<White>();
    U64 black = board.Us<Black>();
    if (popcount(white | black) > (signed)TB_LARGEST)
        return NO_MOVE;

    Square ep = board.enPassantSquare <= 63 ? board.enPassantSquare : Square(0);

    unsigned TBresult;
    TBresult = tb_probe_root(
        white, black, board.Kings<White>() | board.Kings<Black>(), board.Queens<White>() | board.Queens<Black>(),
        board.Rooks<White>() | board.Rooks<Black>(), board.Bishops<White>() | board.Bishops<Black>(),
        board.Knights<White>() | board.Knights<Black>(), board.Pawns<White>() | board.Pawns<Black>(),
        board.halfMoveClock, board.castlingRights, ep, board.sideToMove == White,
        NULL); //  * - turn: true=white, false=black

    if (TBresult == TB_RESULT_FAILED || TBresult == TB_RESULT_CHECKMATE || TBresult == TB_RESULT_STALEMATE)
        return NO_MOVE;

    int dtz = TB_GET_DTZ(TBresult);
    int wdl = TB_GET_WDL(TBresult);

    Score s = 0;

    if (wdl == TB_LOSS)
    {
        s = VALUE_TB_LOSS_IN_MAX_PLY;
    }
    if (wdl == TB_WIN)
    {
        s = VALUE_TB_WIN_IN_MAX_PLY;
    }
    if (wdl == TB_BLESSED_LOSS || wdl == TB_DRAW || wdl == TB_CURSED_WIN)
    {
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

    for (int i = 0; i < legalmoves.size; i++)
    {
        Move move = legalmoves[i].move;
        if (from(move) == sqFrom && to(move) == sqTo)
        {
            if ((promoTranslation[promo] == NONETYPE && !promoted(move)) ||
                (promo < 5 && promoTranslation[promo] == piece(move) && promoted(move)))
            {
                uciOutput(s, static_cast<int>(dtz), 1, getNodes(), getTbHits(), getTime(), " " + uciRep(board, move),
                          TTable.hashfull());
                return move;
            }
        }
    }
    std::cout << " something went wrong playing dtz :" << promoTranslation[promo] << " : " << promo << " : "
              << std::endl;
    exit(0);

    return NO_MOVE;
}

#include "search.h"

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

template <Movetype type> int Search::getHistory(Move move, ThreadData *td)
{
    if constexpr (type == Movetype::QUIET)
        return td->historyTable[td->board.sideToMove][from(move)][to(move)];
}

template <Movetype type> void Search::updateHistoryBonus(Move move, int bonus, ThreadData *td)
{
    int hhBonus = bonus - getHistory<type>(move, td) * std::abs(bonus) / 16384;
    if constexpr (type == Movetype::QUIET)
        td->historyTable[td->board.sideToMove][from(move)][to(move)] += hhBonus;
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

template <Node node> Score Search::qsearch(Score alpha, Score beta, int depth, Stack *ss, ThreadData *td)
{
    if (exitEarly(td->nodes, td->id))
        return VALUE_NONE;

    /********************
     * Initialize various variables
     *******************/
    constexpr bool PvNode = node == PV;
    const Color color = td->board.sideToMove;
    const bool inCheck = td->board.isSquareAttacked(~color, td->board.KingSQ(color));

    Move bestMove = NO_MOVE;
    Score bestValue;
    Score oldAlpha = alpha;
    TEntry tte;

    /********************
     * Check for repetition or 50 move rule draw
     *******************/

    if (td->board.isRepetition() || ss->ply >= MAX_PLY)
        return (ss->ply >= MAX_PLY && !inCheck) ? Eval::evaluation(td->board) : 0;

    /********************
     * Skip evaluating positions that are in check
     *******************/

    if (inCheck)
    {
        bestValue = -VALUE_INFINITE;
    }
    else
    {
        bestValue = Eval::evaluation(td->board);
        if (bestValue >= beta)
            return bestValue;
        if (bestValue > alpha)
            alpha = bestValue;
    }

    /********************
     * Look up in the TT
     * Adjust alpha and beta for non PV Nodes.
     *******************/

    Move ttMove;
    Score ttScore = VALUE_NONE;
    bool ttHit = false;

    probeTT(tte, ttHit, ttMove, td->board.hashKey);
    if (ttHit && tte.depth >= 0 && !PvNode)
    {
        ttScore = scoreFromTT(tte.score, ss->ply);
        if (tte.flag == EXACT)
            return ttScore;
        else if (tte.flag == LOWERBOUND && ttScore >= beta)
            return ttScore;
        else if (tte.flag == UPPERBOUND && ttScore <= alpha)
            return ttScore;
    }

    /********************
     * Generate all legalmoves in case we are in check,
     * otherwise generate all capture moves or generate capture + check
     * giving moves for depth 0.
     *******************/

    ss->moves.size = 0;
    if (inCheck)
        Movegen::legalmoves<Movetype::ALL>(td->board, ss->moves);
    else if (depth == 0)
        Movegen::legalmoves<Movetype::CHECK>(td->board, ss->moves);
    else
        Movegen::legalmoves<Movetype::CAPTURE>(td->board, ss->moves);

    // assign a value to each move
    for (int i = 0; i < ss->moves.size; i++)
        ss->moves[i].value = scoreqMove(ss->moves[i].move, ss->ply, ttMove, td);

    /********************
     * Search the moves
     *******************/

    for (int i = 0; i < static_cast<int>(ss->moves.size); i++)
    {
        // sort the best move to the front
        // we dont need to sort the whole list, since we might have a cutoff
        // and return before we checked all moves
        sortMoves(ss->moves, i);

        Move move = ss->moves[i].move;

        PieceType captured = type_of_piece(td->board.pieceAtB(to(move)));

        // delta pruning, if the move + a large margin is still less then alpha we can safely skip this
        if (captured != NONETYPE && !inCheck && bestValue + 400 + piece_values[EG][captured] < alpha &&
            !promoted(move) && td->board.nonPawnMat(color))
            continue;

        // see based capture pruning
        if (bestValue > VALUE_MATED_IN_PLY && !inCheck && ss->moves[i].value == 50'000)
            continue;

        td->nodes++;
        td->board.makeMove<true>(move);

        Score score = -qsearch<node>(-beta, -alpha, depth + 1, ss + 1, td);
        td->board.unmakeMove<false>(move);

        // update the best score
        if (score > bestValue)
        {
            bestValue = score;
            if (score > alpha)
            {
                bestMove = move;
                alpha = score;
                if (score >= beta)
                    break;
            }
        }
    }

    /********************
     * Checkmate Check
     *******************/

    if (ss->moves.size == 0)
    {
        if (inCheck)
            return mated_in(ss->ply);
        else if (!Movegen::hasLegalMoves(td->board))
            return 0;
    }

    /********************
     * store in the transposition table
     *******************/

    // Transposition table flag
    Flag b = bestValue >= beta ? LOWERBOUND : (alpha != oldAlpha ? EXACT : UPPERBOUND);

    if (!stopped.load(std::memory_order_relaxed))
        storeEntry(0, scoreToTT(bestValue, ss->ply), b, td->board.hashKey, bestMove);
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

    TEntry tte;

    Score best = -VALUE_INFINITE;
    Score maxValue = VALUE_MATE;
    Score staticEval;
    Score oldAlpha = alpha;

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
        return qsearch<node>(alpha, beta, 0, ss, td);

    /********************
     * Selective depth (heighest depth we have ever reached)
     *******************/

    if (ss->ply > td->seldepth)
        td->seldepth = ss->ply;

    bool ttHit = false;
    Move ttMove = NO_MOVE;
    Score ttScore = VALUE_NONE;

    probeTT(tte, ttHit, ttMove, td->board.hashKey);

    /********************
     * Look up in the TT
     * Adjust alpha and beta for non PV nodes
     *******************/

    if (!RootNode && !PvNode && ttHit && tte.depth >= depth && (ss - 1)->currentmove != NULL_MOVE)
    {
        ttScore = scoreFromTT(tte.score, ss->ply);

        if (tte.flag == EXACT)
            return ttScore;
        else if (tte.flag == LOWERBOUND)
            alpha = std::max(alpha, ttScore);
        else if (tte.flag == UPPERBOUND)
            beta = std::min(beta, ttScore);
        if (alpha >= beta)
            return ttScore;
    }

    /********************
     *  Tablebase probing
     *******************/

    if (!RootNode && td->allowPrint && useTB)
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
                flag = EXACT;
                break;
            }

            if (flag == EXACT || (flag == LOWERBOUND && tbRes >= beta) || (flag == UPPERBOUND && tbRes <= alpha))
            {
                storeEntry(depth + 6, scoreToTT(tbRes, ss->ply), flag, td->board.hashKey, NO_MOVE);
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
    ss->eval = staticEval = ttHit ? tte.score : Eval::evaluation(td->board);

    // improving boolean
    improving = (ss - 2)->eval != VALUE_NONE ? staticEval > (ss - 2)->eval : false;

    if (RootNode)
        goto moves;

    /********************
     * Razoring
     *******************/
    if (!PvNode && depth < 3 && staticEval + 120 < alpha)
        return qsearch<NonPV>(alpha, beta, 0, ss, td);

    /********************
     * Reverse futility pruning
     *******************/
    if (std::abs(beta) < VALUE_MATE_IN_PLY)
        if (depth < 6 && staticEval - 61 * depth + 73 * improving >= beta)
            return beta;

    /********************
     * Null move pruning
     *******************/
    if (!PvNode && td->board.nonPawnMat(color) && (ss - 1)->currentmove != NULL_MOVE && depth >= 3 &&
        staticEval >= beta)
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

    /********************
     * Internal Iterative Deepening
     *******************/
    if (depth >= 4 && !ttHit)
        depth--;

    if (PvNode && !ttHit)
        depth--;

    if (depth <= 0)
        return qsearch<PV>(alpha, beta, 0, ss, td);

moves:
    // reset movelists
    ss->moves.size = 0;
    ss->quietMoves.size = 0;

    Movegen::legalmoves<Movetype::ALL>(td->board, ss->moves);

    Score score = VALUE_NONE;
    Move bestMove = NO_MOVE;
    Move move;
    uint8_t madeMoves = 0;

    bool doFullSearch = false;

    Movepicker mp;
    mp.stage = TT_MOVE;

    /********************
     * Movepicker fetches the next move that we should search.
     * It is very important to return the likely best move first,
     * since then we get many cut offs.
     *******************/
    while ((move = nextMove(ss->moves, mp, ttMove, td, ss)) != NO_MOVE)
    {
        madeMoves++;

        bool capture = td->board.pieceAtB(to(move)) != None;

        int newDepth = depth - 1;

        /********************
         * Various pruning techniques.
         *******************/
        if (!RootNode && best > -VALUE_INFINITE)
        {
            // late move pruning/movecount pruning
            if (!capture && !inCheck && !PvNode && !promoted(move) && depth <= 4 &&
                ss->quietMoves.size > (4 + depth * depth))
                continue;

            // SEE pruning
            if (depth < 6 && !see(move, -(depth * 97), td->board))
                continue;
        }

        td->nodes++;

        /********************
         * Print currmove information.
         *******************/
        if (td->id == 0 && RootNode && !stopped.load(std::memory_order_relaxed) && getTime() > 10000 && td->allowPrint)
            std::cout << "info depth " << depth - inCheck << " currmove " << uciRep(move) << " currmovenumber "
                      << signed(madeMoves) << "\n";

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
            doFullSearch = score > alpha;
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
            bestMove = move;

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
    Flag b = best >= beta ? LOWERBOUND : (alpha != oldAlpha ? EXACT : UPPERBOUND);

    if (!stopped.load(std::memory_order_relaxed))
        storeEntry(depth, scoreToTT(best, ss->ply), b, td->board.hashKey, bestMove);

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

        if (td->id == 0 && maxNodes != 0 && td->nodes >= maxNodes)
            return 0;

        /********************
         * Increase the bounds because the score was outside of them or
         * break in case it was a EXACT result.
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
        uciOutput(result, depth, td->seldepth, getNodes(), getTbHits(), getTime(), getPV());

    return result;
}

SearchResult Search::iterativeDeepening(int searchDepth, uint64_t maxN, Time time, int threadId)
{
    /********************
     * Various Limits that only the main Thread needs to know
     * and initialise.
     *******************/
    if (threadId == 0)
    {
        t0 = TimePoint::now();
        optimumTime = time.optimum;
        maxTime = time.maximum;
        maxNodes = maxN;
        checkTime = 0;
    }

    Move bestmove = NO_MOVE;
    SearchResult sr;

    Score result = -VALUE_INFINITE;
    int depth = 1;

    Stack stack[MAX_PLY + 4], *ss = stack + 2;
    std::memset(spentEffort, 0, 64 * 64 * sizeof(U64));

    for (int i = -2; i <= MAX_PLY + 1; ++i)
    {
        (ss + i)->ply = i;
        (ss + i)->moves.size = 0;
        (ss + i)->currentmove = Move(0);
        (ss + i)->eval = 0;
    }

    ThreadData *td = &this->tds[threadId];
    td->nodes = 0;
    td->tbhits = 0;
    td->seldepth = 0;

    int bestmoveChanges = 0;
    int evalAverage = 0;

    /********************
     * Iterative Deepening Loop.
     *******************/
    for (depth = 1; depth <= searchDepth; depth++)
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
        if (optimumTime != 0)
        {
            auto now = getTime();

            // node count time management (https://github.com/Luecx/Koivisto 's idea)
            int effort = (spentEffort[from(bestmove)][to(bestmove)] * 100) / td->nodes;
            if (optimumTime * (110 - std::min(effort, 90)) / 100 < now)
                break;

            if (result + 30 < evalAverage / depth)
                optimumTime *= 1.10;

            // stop if we have searched for more than 75% of our max time.
            if (bestmoveChanges > 4)
                optimumTime = maxTime * 0.75;
            else if (now * 10 > optimumTime * 6)
                break;
        }
    }

    /********************
     * Dont stop analysis in infinite mode when max depth is reached
     * wait for uci stop or quit
     *******************/
    while (td->allowPrint && depth == MAX_PLY + 1 && optimumTime == 0 && !stopped.load(std::memory_order_relaxed))
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
        std::cout << "bestmove " << uciRep(bestmove) << std::endl;
        stopped = true;
    }
    sr.move = bestmove;
    return sr;
}

void Search::startThinking(Board board, int workers, int searchDepth, uint64_t maxN, Time time)
{
    /********************
     * If we dont have previous data create default data
     *******************/
    for (int i = tds.size(); i < workers; i++)
    {
        ThreadData td;
        td.board = board;
        td.id = i;
        this->tds.push_back(td);
    }

    /********************
     * Set the right board for the main thread
     * important that this is done before probing the DTZ
     *******************/
    this->tds[0].board = board;
    this->tds[0].id = 0;

    /********************
     * Play dtz move when time is limited
     *******************/
    if (time.optimum != 0)
    {
        Move dtzMove = probeDTZ(&this->tds[0]);
        if (dtzMove != NO_MOVE)
        {
            std::cout << "bestmove " << uciRep(dtzMove) << std::endl;
            stopped = true;
            return;
        }
    }

    /********************
     * Start main thread right away
     *******************/
    this->threads.emplace_back(&Search::iterativeDeepening, this, searchDepth, maxN, time, 0);

    /********************
     * Launch helper threads
     *******************/
    for (int i = 1; i < workers; i++)
    {
        this->tds[i].board = board;
        this->threads.emplace_back(&Search::iterativeDeepening, this, searchDepth, maxN, time, i);
    }
}

/********************
 * Static Exchange Evaluation, logical based on Weiss (https://github.com/TerjeKir/weiss) licensed under GPL-3.0
 *******************/
bool Search::see(Move move, int threshold, Board &board)
{
    Square from_sq = from(move);
    Square to_sq = to(move);
    PieceType attacker = type_of_piece(board.pieceAtB(from_sq));
    PieceType victim = type_of_piece(board.pieceAtB(to_sq));
    int swap = pieceValuesDefault[victim] - threshold;
    if (swap < 0)
        return false;
    swap -= pieceValuesDefault[attacker];
    if (swap >= 0)
        return true;
    U64 occ = (board.All() ^ (1ULL << from_sq)) | (1ULL << to_sq);
    U64 attackers = board.allAttackers(to_sq, occ) & occ;

    U64 bishops = board.Bishops(board.sideToMove) | board.Queens(board.sideToMove) | board.Bishops(~board.sideToMove) |
                  board.Queens(~board.sideToMove);
    U64 rooks = board.Rooks(board.sideToMove) | board.Queens(board.sideToMove) | board.Rooks(~board.sideToMove) |
                board.Queens(~board.sideToMove);
    Color sT = ~Color((board.pieceAtB(from_sq) / 6));

    while (true)
    {
        attackers &= occ;
        U64 myAttackers = attackers & board.Us(sT);
        if (!myAttackers)
            break;

        int pt;
        for (pt = 0; pt <= 5; pt++)
        {
            if (myAttackers & (board.Bitboards[pt] | board.Bitboards[pt + 6]))
                break;
        }
        sT = ~sT;
        if ((swap = -swap - 1 - piece_values[MG][pt]) >= 0)
        {
            if (pt == KING && (attackers & board.Us(sT)))
                sT = ~sT;
            break;
        }

        occ ^= (1ULL << (bsf(myAttackers & (board.Bitboards[pt] | board.Bitboards[pt + 6]))));

        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= BishopAttacks(to_sq, occ) & bishops;
        if (pt == ROOK || pt == QUEEN)
            attackers |= RookAttacks(to_sq, occ) & rooks;
    }
    return sT != Color((board.pieceAtB(from_sq) / 6));
}

int Search::mvvlva(Move move, Board &board)
{
    int attacker = type_of_piece(board.pieceAtB(from(move))) + 1;
    int victim = type_of_piece(board.pieceAtB(to(move))) + 1;
    return mvvlvaArray[victim][attacker];
}

int Search::scoreMove(Move move, int ply, Move ttMove, ThreadData *td)
{
    if (move == ttMove)
    {
        return TT_MOVE_SCORE;
    }
    else if (promoted(move))
    {
        return PROMOTION_SCORE + piece(move);
    }
    else if (td->board.pieceAtB(to(move)) != None)
    {
        return see(move, 0, td->board) ? CAPTURE_SCORE + mvvlva(move, td->board) : mvvlva(move, td->board);
    }
    else if (td->killerMoves[0][ply] == move)
    {
        return KILLER_ONE_SCORE;
    }
    else if (td->killerMoves[1][ply] == move)
    {
        return KILLER_TWO_SCORE;
    }
    else
    {
        return getHistory<Movetype::QUIET>(move, td);
    }
}

int Search::scoreqMove(Move move, int ply, Move ttMove, ThreadData *td)
{
    if (move == ttMove)
    {
        return TT_MOVE_SCORE;
    }
    else if (promoted(move))
    {
        return PROMOTION_SCORE + piece(move);
    }
    else if (td->board.pieceAtB(to(move)) != None)
    {
        return see(move, 0, td->board) ? CAPTURE_SCORE + mvvlva(move, td->board) : 50'000;
    }
    else if (td->killerMoves[0][ply] == move)
    {
        return 40'000;
    }
    else if (td->killerMoves[1][ply] == move)
    {
        return 30'000;
    }
    else
    {
        return getHistory<Movetype::QUIET>(move, td);
    }
}

Move Search::nextMove(Movelist &moves, Movepicker &mp, Move ttMove, ThreadData *td, Stack *ss)
{
    switch (mp.stage)
    {
    case TT_MOVE:
        mp.stage++;
        mp.ttMoveIndex = -1;
        for (mp.i = 0; mp.i < moves.size; mp.i++)
        {
            const Move move = moves[mp.i].move;
            moves[mp.i].value = scoreMove(move, ss->ply, ttMove, td);
            if (moves[mp.i].value == TT_MOVE_SCORE)
            {
                mp.ttMoveIndex = mp.i;
                mp.i++;
                return move;
            }
        }
    case EVAL_OTHER:
        for (; mp.i < moves.size; mp.i++)
        {
            moves[mp.i].value = scoreMove(moves[mp.i].move, ss->ply, NO_MOVE, td);
        }
        if (mp.ttMoveIndex != -1)
        {
            std::swap(moves[0], moves[mp.ttMoveIndex]);
            mp.i = 1;
        }
        else
            mp.i = 0;

        mp.stage++;
    case OTHER:
        if (mp.i < moves.size)
        {
            sortMoves(moves, mp.i);
            const Move move = moves[mp.i].move;
            mp.i++;
            return move;
        }
        else
            return NO_MOVE;

    default:
        return NO_MOVE;
    }
}

void Search::sortMoves(Movelist &moves, int sorted)
{
    int index = 0 + sorted;
    for (int i = 1 + sorted; i < moves.size; i++)
    {
        if (moves[i] > moves[index])
            index = i;
    }
    std::swap(moves[index], moves[0 + sorted]);
}

bool Search::exitEarly(uint64_t nodes, int ThreadId)
{
    if (stopped.load(std::memory_order_relaxed))
        return true;
    if (ThreadId != 0)
        return false;
    if (maxNodes != 0 && nodes >= maxNodes)
        return true;

    if (--checkTime > 0)
        return false;
    checkTime = 2047;

    if (maxTime != 0)
    {
        auto ms = getTime();
        if (ms >= maxTime)
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
        ss << " " << uciRep(tds[0].pvTable[0][i]);
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
    for (size_t i = 0; i < tds.size(); i++)
        nodes += tds[i].nodes;
    return nodes;
}

uint64_t Search::getTbHits()
{
    uint64_t nodes = 0;
    for (size_t i = 0; i < tds.size(); i++)
    {
        nodes += tds[i].tbhits;
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

Move Search::probeDTZ(ThreadData *td)
{
    U64 white = td->board.Us<White>();
    U64 black = td->board.Us<Black>();
    if (popcount(white | black) > (signed)TB_LARGEST)
        return NO_MOVE;

    Square ep = td->board.enPassantSquare <= 63 ? td->board.enPassantSquare : Square(0);

    unsigned TBresult;
    TBresult = tb_probe_root(
        white, black, td->board.Kings<White>() | td->board.Kings<Black>(),
        td->board.Queens<White>() | td->board.Queens<Black>(), td->board.Rooks<White>() | td->board.Rooks<Black>(),
        td->board.Bishops<White>() | td->board.Bishops<Black>(),
        td->board.Knights<White>() | td->board.Knights<Black>(), td->board.Pawns<White>() | td->board.Pawns<Black>(),
        td->board.halfMoveClock, td->board.castlingRights, ep, td->board.sideToMove == White,
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
    Movegen::legalmoves<Movetype::ALL>(td->board, legalmoves);

    for (int i = 0; i < legalmoves.size; i++)
    {
        Move move = legalmoves[i].move;
        if (from(move) == sqFrom && to(move) == sqTo)
        {
            if ((promoTranslation[promo] == NONETYPE && !promoted(move)) ||
                (promo < 5 && promoTranslation[promo] == piece(move) && promoted(move)))
            {
                uciOutput(s, static_cast<int>(dtz), 1, getNodes(), getTbHits(), getTime(), " " + uciRep(move));
                return move;
            }
        }
    }
    std::cout << " something went wrong playing dtz :" << promoTranslation[promo] << " : " << promo << " : "
              << std::endl;
    exit(0);

    return NO_MOVE;
}
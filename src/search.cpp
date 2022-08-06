#include "search.h"

#define hhEntry(bestmove, td) &td->history_table[td->board.sideToMove][from(bestmove)][to(bestmove)]

void Search::UpdateHH(Move bestMove, Score best, Score beta, int depth, Movelist &quietMoves, ThreadData *td)
{
    if (td->board.pieceAtB(to(bestMove)) == None)
    {
        if (best < beta)
            return;
        int bonus = depth * depth;
        bonus += bonus - *hhEntry(bestMove, td) * std::abs(bonus) / 16384;
        if (depth > 1)
            *hhEntry(bestMove, td) += bonus;
        for (int i = 0; i < quietMoves.size; i++)
        {
            const Move move = quietMoves.list[i];
            if (move == bestMove)
                continue;
            *hhEntry(move, td) -= bonus;
        }
    }
}

template <Node node> Score Search::qsearch(Score alpha, Score beta, Stack *ss, ThreadData *td)
{
    if (exit_early(td->nodes, td->id))
        return VALUE_NONE;

    // Initialize various variables
    constexpr bool PvNode = node == PV;
    const Color color = td->board.sideToMove;
    const bool inCheck = td->board.isSquareAttacked(~color, td->board.KingSQ(color));

    bool ttHit = false;
    Move bestMove = NO_MOVE;
    Score bestValue;
    Score oldAlpha = alpha;
    TEntry tte;

    // check for repetition or 50 move rule draw
    if (td->board.isRepetition() || ss->ply >= MAX_PLY)
        return (ss->ply >= MAX_PLY && !inCheck) ? evaluation(td->board) : 0;

    // skip evaluating positions that are in check
    if (inCheck)
    {
        bestValue = -VALUE_INFINITE;
    }
    else
    {
        bestValue = evaluation(td->board);
        if (bestValue >= beta)
            return bestValue;
        if (bestValue > alpha)
            alpha = bestValue;
    }

    // probe the transposition table
    // probe_tt(tte, ttHit, td->board.hashKey);
    Score ttScore = VALUE_NONE;
    if (ttHit && tte.depth >= 0 && !PvNode)
    {
        ttScore = score_from_tt(tte.score, ss->ply);
        if (tte.flag == EXACT)
            return ttScore;
        else if (tte.flag == LOWERBOUND && ttScore >= beta)
            return ttScore;
        else if (tte.flag == UPPERBOUND && ttScore <= alpha)
            return ttScore;
    }

    // generate all legalmoves in case we are in check
    Movelist ml = inCheck ? td->board.legalmoves() : td->board.capturemoves();

    // assign a value to each move
    for (int i = 0; i < ml.size; i++)
        ml.values[i] = score_move(ml.list[i], ss->ply, ttHit, td);

    // search the moves
    for (int i = 0; i < (int)ml.size; i++)
    {
        // sort the best move to the front
        // we dont need to sort the whole list, since we might have a cutoff
        // and return before we checked all moves
        sortMoves(ml, i);

        Move move = ml.list[i];

        Piece captured = td->board.pieceAtB(to(move));

        // delta pruning, if the move + a large margin is still less then alpha we can safely skip this
        if (captured != None && !inCheck && bestValue + 400 + piece_values[EG][captured % 6] < alpha &&
            !promoted(move) && td->board.nonPawnMat(color))
            continue;

        // see based capture pruning
        if (bestValue > VALUE_MATED_IN_PLY && captured != None && !see(move, -100, td->board))
            continue;

        td->nodes++;
        td->board.makeMove<true>(move);
        Score score = -qsearch<node>(-beta, -alpha, ss + 1, td);
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

    if (inCheck && ml.size == 0)
        return mated_in(ss->ply);

    // Transposition table flag
    Flag b = bestValue >= beta ? LOWERBOUND : (alpha != oldAlpha ? EXACT : UPPERBOUND);

    // store in the transposition table
    // if (!stopped)
    //     store_entry(0, score_to_tt(bestValue, ss->ply), b, td->board.hashKey, bestMove);
    return bestValue;
}

template <Node node> Score Search::absearch(int depth, Score alpha, Score beta, Stack *ss, ThreadData *td)
{
    if (exit_early(td->nodes, td->id))
        return 0;

    // Initialize various variables
    constexpr bool RootNode = node == Root;
    constexpr bool PvNode = node != NonPV;

    Color color = td->board.sideToMove;

    TEntry tte;

    Score best = -VALUE_INFINITE;
    Score staticEval;
    Score oldAlpha = alpha;

    bool improving, ttHit;
    bool inCheck = td->board.isSquareAttacked(~color, td->board.KingSQ(color));

    if (ss->ply >= MAX_PLY)
        return (ss->ply >= MAX_PLY && !inCheck) ? evaluation(td->board) : 0;

    td->pv_length[ss->ply] = ss->ply;

    // Draw detection and mate pruning
    if (!RootNode)
    {
        if (td->board.halfMoveClock >= 100)
            return 0;
        if (td->board.isRepetition() && (ss - 1)->currentmove != NULL_MOVE)
            return -3 + (td->nodes & 7);
        int all = popcount(td->board.All());
        if (all == 2)
            return 0;
        if (all == 3 && (td->board.Bitboards[WhiteKnight] || td->board.Bitboards[BlackKnight] ||
                         td->board.Bitboards[WhiteBishop] || td->board.Bitboards[BlackBishop]))
            return 0;

        alpha = std::max(alpha, mated_in(ss->ply));
        beta = std::min(beta, mate_in(ss->ply + 1));
        if (alpha >= beta)
            return alpha;
    }

    // Check extension
    if (inCheck)
        depth++;

    // Enter qsearch
    if (depth <= 0)
        return qsearch<node>(alpha, beta, ss, td);

    // Selective depth (heighest depth we have ever reached)
    if (ss->ply > td->seldepth)
        td->seldepth = ss->ply;

    // Look up in the TT
    ttHit = false;
    // probe_tt(tte, ttHit, td->board.hashKey);
    Score ttScore = VALUE_NONE;
    // Adjust alpha and beta for non PV nodes
    if (!RootNode && !PvNode && ttHit && tte.depth >= depth && (ss - 1)->currentmove != NULL_MOVE)
    {
        ttScore = score_from_tt(tte.score, ss->ply);
        if (tte.flag == EXACT)
            return ttScore;
        else if (tte.flag == LOWERBOUND)
            alpha = std::max(alpha, ttScore);
        else if (tte.flag == UPPERBOUND)
            beta = std::min(beta, ttScore);
        if (alpha >= beta)
            return ttScore;
    }

    if (inCheck)
    {
        improving = false;
        staticEval = VALUE_NONE;
        goto moves;
    }

    // use tt eval for a better staticEval
    ss->eval = staticEval = ttHit ? tte.score : evaluation(td->board);

    // improving boolean, similar to stockfish
    improving = ss->ply >= 2 && staticEval > (ss - 2)->eval;

    // Razoring
    if (!PvNode && depth < 3 && staticEval + 120 < alpha)
        return qsearch<NonPV>(alpha, beta, ss, td);

    // Reverse futility pruning
    if (std::abs(beta) < VALUE_MATE_IN_PLY)
        if (depth < 6 && staticEval - 61 * depth + 73 * improving >= beta)
            return beta;

    // Null move pruning
    if (!PvNode && td->board.nonPawnMat(color) && (ss - 1)->currentmove != NULL_MOVE && depth >= 3 &&
        staticEval >= beta)
    {
        int R = 5 + depth / 5 + std::min(3, (staticEval - beta) / 214);
        td->board.makeNullMove();
        (ss)->currentmove = NULL_MOVE;
        Score score = -absearch<NonPV>(depth - R, -beta, -beta + 1, ss + 1, td);
        td->board.unmakeNullMove();
        if (score >= beta)
        {
            // dont return mate scores
            if (score >= VALUE_MATE_IN_PLY)
                score = beta;
            return score;
        }
    }

    // IID
    if (depth >= 4 && !ttHit)
        depth--;

    if (PvNode && !ttHit)
        depth--;

    if (depth <= 0)
        return qsearch<PV>(alpha, beta, ss, td);

moves:
    Movelist ml = td->board.legalmoves();

    // if the move list is empty, we are in checkmate or stalemate
    if (ml.size == 0)
        return inCheck ? mated_in(ss->ply) : 0;

    // assign a value to each move
    for (int i = 0; i < ml.size; i++)
        ml.values[i] = score_move(ml.list[i], ss->ply, ttHit, td);

    // set root node move list size
    if (RootNode && td->id == 0)
        rootSize = ml.size;

    Movelist quietMoves;
    Move bestMove = NO_MOVE;
    Score score = VALUE_NONE;
    uint8_t madeMoves = 0;

    bool doFullSearch = false;

    for (int i = 0; i < ml.size; i++)
    {
        // sort the moves
        sortMoves(ml, i);

        Move move = ml.list[i];
        bool capture = td->board.pieceAtB(to(move)) != None;

        if (!capture)
            quietMoves.Add(move);

        int newDepth = depth - 1;
        // Pruning
        if (!RootNode && best > -VALUE_INFINITE)
        {
            // late move pruning/movecount pruning
            if (!capture && !inCheck && !PvNode && !promoted(move) && depth <= 4 &&
                quietMoves.size > (4 + depth * depth))
                continue;

            // See pruning
            if (depth < 6 && !see(move, -(depth * 97), td->board))
                continue;
        }

        td->nodes++;
        madeMoves++;

        if (td->id == 0 && td->allowPrint && RootNode && !stopped && elapsed() > 10000)
            std::cout << "info depth " << depth - inCheck << " currmove " << printMove(move) << " currmovenumber "
                      << signed(madeMoves) << "\n";

        td->board.makeMove<true>(move);

        U64 nodeCount = td->nodes;
        ss->currentmove = move;
        // bool givesCheck = td->board.isSquareAttacked(color, td->board.KingSQ(~color));

        // late move reduction
        if (depth >= 3 && !inCheck && madeMoves > 3 + 2 * PvNode)
        {
            int rdepth = reductions[depth][madeMoves];
            rdepth -= td->id % 2;
            rdepth = std::clamp(newDepth - rdepth, 1, newDepth + 1);

            score = -absearch<NonPV>(rdepth, -alpha - 1, -alpha, ss + 1, td);
            doFullSearch = score > alpha;
        }
        else
            doFullSearch = !PvNode || madeMoves > 1;

        // do a full research if lmr failed or lmr was skipped
        if (doFullSearch)
        {
            score = -absearch<NonPV>(newDepth, -alpha - 1, -alpha, ss + 1, td);
        }

        // PVS search
        if (PvNode && ((score > alpha && score < beta) || madeMoves == 1))
        {
            score = -absearch<PV>(newDepth, -beta, -alpha, ss + 1, td);
        }

        td->board.unmakeMove<false>(move);
        spentEffort[from(move)][to(move)] += td->nodes - nodeCount;

        if (score > best)
        {
            best = score;
            bestMove = move;

            // update the PV
            td->pv_table[ss->ply][ss->ply] = move;
            for (int next_ply = ss->ply + 1; next_ply < td->pv_length[ss->ply + 1]; next_ply++)
            {
                td->pv_table[ss->ply][next_ply] = td->pv_table[ss->ply + 1][next_ply];
            }
            td->pv_length[ss->ply] = td->pv_length[ss->ply + 1];

            if (score > alpha)
            {
                alpha = score;

                if (score >= beta)
                {
                    // update Killer Moves
                    if (!capture)
                    {
                        td->killerMoves[1][ss->ply] = td->killerMoves[0][ss->ply];
                        td->killerMoves[0][ss->ply] = move;
                    }
                    break;
                }
            }
        }
    }

    // update history heuristic
    UpdateHH(bestMove, best, beta, depth, quietMoves, td);

    // Store position in TT
    Flag b = best >= beta ? LOWERBOUND : (alpha != oldAlpha ? EXACT : UPPERBOUND);
    // if (!stopped && !RootNode)
    //     store_entry(depth, score_to_tt(best, ss->ply), b, td->board.hashKey, bestMove);
    return best;
}

Score Search::aspiration_search(int depth, Score prev_eval, Stack *ss, ThreadData *td)
{
    Score alpha = -VALUE_INFINITE;
    Score beta = VALUE_INFINITE;
    int delta = 30;

    Score result = 0;

    if (depth >= 9)
    {
        alpha = prev_eval - delta;
        beta = prev_eval + delta;
    }

    while (true)
    {
        if (stopped)
            return 0;
        if (alpha < -3500)
            alpha = -VALUE_INFINITE;
        if (beta > 3500)
            beta = VALUE_INFINITE;
        result = absearch<Root>(depth, alpha, beta, ss, td);

        if (stopped)
            return 0;

        if (result <= alpha)
        {
            beta = (alpha + beta) / 2;
            alpha = std::max(alpha - delta, -((int)VALUE_INFINITE));
            delta += delta / 2;
        }
        else if (result >= beta)
        {
            beta = std::min(beta + delta, (int)VALUE_INFINITE);
            delta += delta / 2;
        }
        else
        {
            break;
        }
    }
    if (td->id == 0 && td->allowPrint)
        uci_output(result, alpha, beta, depth, td->seldepth, get_nodes(), elapsed(), get_pv());
    return result;
}

SearchResult Search::iterative_deepening(int search_depth, uint64_t maxN, Time time, int threadId)
{
    // Limits
    int64_t startTime = 0;
    if (threadId == 0)
    {
        t0 = TimePoint::now();
        searchTime = startTime = time.optimum;
        maxTime = time.maximum;
        maxNodes = maxN;
        checkTime = 0;
    }
    ThreadData *td = &this->tds[threadId];
    td->nodes = 0;
    td->seldepth = 0;

    SearchResult sResult;
    sResult.score = VALUE_NONE;
    sResult.move = NO_MOVE;

    Move reducedTimeMove = NO_MOVE;
    Move previousBestmove;
    bool adjustedTime;

    int result = -VALUE_INFINITE;
    int depth = 1;

    Stack stack[MAX_PLY + 4], *ss = stack + 2;
    std::memset(ss - 2, 0, 4 * sizeof(Stack));
    std::memset(spentEffort, 0, 64 * 64 * sizeof(U64));

    for (int i = -2; i <= MAX_PLY + 1; ++i)
        (ss + i)->ply = i;

    for (depth = 1; depth <= search_depth; depth++)
    {
        td->seldepth = 0;
        result = aspiration_search(depth, result, ss, td);
        if (stopped)
            break;

        sResult.score = result;

        if (threadId != 0)
            continue;

        if (searchTime != 0)
        {
            if (rootSize == 1)
                searchTime = std::min((int64_t)50, searchTime);

            int effort = (spentEffort[from(previousBestmove)][to(previousBestmove)] * 100) / td->nodes;

            if (depth >= 8 && effort >= 95 && searchTime != 0 && !adjustedTime)
            {
                adjustedTime = true;
                searchTime = searchTime / 3;
                reducedTimeMove = td->pv_table[0][0];
            }

            if (adjustedTime && td->pv_table[0][0] != reducedTimeMove)
            {
                searchTime = startTime * 1.05f;
            }
        }

        previousBestmove = td->pv_table[0][0];
    }

    Move bestmove = depth == 1 ? td->pv_table[0][0] : previousBestmove;
    if (threadId == 0)
    {
        if (td->allowPrint)
            std::cout << "bestmove " << printMove(bestmove) << std::endl;
        stopped = true;
    }

    sResult.move = bestmove;

    return sResult;
}

void Search::start_thinking(Board board, int workers, int search_depth, uint64_t maxN, Time time)
{
    stopped = true;
    for (std::thread &th : threads)
    {
        if (th.joinable())
            th.join();
    }
    threads.clear();
    stopped = false;
    // If we dont have previous data create default data
    for (int i = tds.size(); i < workers; i++)
    {
        ThreadData td;
        td.board = board;
        td.id = i;
        this->tds.push_back(td);
    }

    this->tds[0].board = board;
    this->tds[0].id = 0;
    this->threads.emplace_back(&Search::iterative_deepening, this, search_depth, maxN, time, 0);
    for (int i = 1; i < workers; i++)
    {
        this->tds[i].board = board;
        this->threads.emplace_back(&Search::iterative_deepening, this, search_depth, maxN, time, i);
    }
}

// Static Exchange Evaluation, logical based on Weiss (https://github.com/TerjeKir/weiss) licensed under GPL-3.0
bool Search::see(Move &move, int threshold, Board &board)
{
    Square from_sq = from(move);
    Square to_sq = to(move);
    PieceType attacker = type_of_piece(board.pieceAtB(from_sq));
    PieceType victim = type_of_piece(board.pieceAtB(to_sq));
    int swap = piece_values[MG][victim] - threshold;
    if (swap < 0)
        return false;
    swap -= piece_values[MG][attacker];
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
            attackers |= board.BishopAttacks(to_sq, occ) & bishops;
        if (pt == ROOK || pt == QUEEN)
            attackers |= board.RookAttacks(to_sq, occ) & rooks;
    }
    return sT != Color((board.pieceAtB(from_sq) / 6));
}

int Search::mmlva(Move &move, Board &board)
{
    int attacker = type_of_piece(board.pieceAtB(from(move))) + 1;
    int victim = type_of_piece(board.pieceAtB(to(move))) + 1;
    return mvvlva[victim][attacker];
}

int Search::score_move(Move &move, int ply, bool ttMove, ThreadData *td)
{
    if (ttMove && move == TTable[tt_index(td->board.hashKey)].move)
    {
        return 10'000'000;
    }
    else if (promoted(move))
    {
        return 9'000'000 + piece(move);
    }
    else if (td->board.pieceAtB(to(move)) != None)
    {
        return see(move, -100, td->board) ? 7'000'000 + mmlva(move, td->board) : mmlva(move, td->board);
    }
    else if (td->killerMoves[0][ply] == move)
    {
        return killerscore1;
    }
    else if (td->killerMoves[1][ply] == move)
    {
        return killerscore2;
    }
    else
    {
        return td->history_table[td->board.sideToMove][from(move)][to(move)];
    }
}

int Search::score_qmove(Move &move, Board &board)
{
    if (promoted(move))
    {
        return 2147483647 - 20 + piece(move);
    }
    else if (board.pieceAtB(to(move)) != None)
    {
        return see(move, -100, board) ? mmlva(move, board) * 10000 : mmlva(move, board);
    }
    else
    {
        return 0;
    }
}

std::string Search::get_pv()
{
    std::string line = "";
    for (int i = 0; i < tds[0].pv_length[0]; i++)
    {
        line += " ";
        line += printMove(tds[0].pv_table[0][i]);
    }
    return line;
}

long long Search::elapsed()
{
    auto t1 = TimePoint::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

bool Search::exit_early(uint64_t nodes, int ThreadId)
{
    if (stopped)
        return true;
    if (ThreadId != 0)
        return false;
    if (maxNodes != 0 && nodes >= maxNodes)
    {
        stopped = true;
        return true;
    }

    if (--checkTime > 0)
        return false;
    checkTime = 2047;

    if (searchTime != 0)
    {
        auto ms = elapsed();
        if (ms >= searchTime || ms >= maxTime)
        {
            stopped = true;
            return true;
        }
    }
    return false;
}

uint64_t Search::get_nodes()
{
    uint64_t nodes = 0;
    for (size_t i = 0; i < tds.size(); i++)
    {
        nodes += tds[i].nodes;
    }
    return nodes;
}

void Search::sortMoves(Movelist &moves, int sorted)
{
    int index = 0 + sorted;
    for (int i = 1 + sorted; i < moves.size; i++)
    {
        if (moves.values[i] > moves.values[index])
        {
            index = i;
        }
    }
    std::swap(moves.list[index], moves.list[0 + sorted]);
    std::swap(moves.values[index], moves.values[0 + sorted]);
}

std::string output_score(int score, Score alpha, Score beta)
{
    if (score >= beta)
        return "lowerbound " + std::to_string(score);
    else if (score <= alpha)
        return "upperbound " + std::to_string(score);
    else if (score >= VALUE_MATE_IN_PLY)
    {
        return "mate " + std::to_string(((VALUE_MATE - score) / 2) + ((VALUE_MATE - score) & 1));
    }
    else if (score <= VALUE_MATED_IN_PLY)
    {
        return "mate " + std::to_string(-((VALUE_MATE + score) / 2) + ((VALUE_MATE + score) & 1));
    }
    else
    {
        return "cp " + std::to_string(score);
    }
}

void uci_output(int score, Score alpha, Score beta, int depth, uint8_t seldepth, U64 nodes, int time, std::string pv)
{
    std::cout << "info depth " << signed(depth) << " seldepth " << signed(seldepth) << " score "
              << output_score(score, alpha, beta) << " nodes " << nodes << " nps "
              << signed((nodes / (time + 1)) * 1000) << " time " << time << " pv" << pv << std::endl;
}
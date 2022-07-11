#include "search.h"

void Search::UpdateHH(Move bestMove, int depth, Movelist quietMoves, ThreadData *td) {
    if (depth > 1) td->history_table[td->board.sideToMove][bestMove.from()][bestMove.to()] += depth * depth;
    for (int i = 0; i < quietMoves.size; i++) {
        Move move = quietMoves.list[i];
        if (move == bestMove) continue;
        td->history_table[td->board.sideToMove][move.from()][move.to()] -= depth * depth;
    }
}

Score Search::qsearch(int depth, Score alpha, Score beta, int ply, ThreadData *td) {
    if (exit_early(td->nodes, td->id)) return 0;
    Score stand_pat = evaluation(td->board);
    Score bestValue = stand_pat;
    Color color = td->board.sideToMove;
    
    if (ply > MAX_PLY - 1) return stand_pat;

    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    if (depth == 0) return alpha;

    Movelist ml = td->board.capturemoves();

    // assign a value to each move
    for (int i = 0; i < ml.size; i++) {
        ml.list[i].value = score_qmove(ml.list[i], td->board);
    }

    // sort the moves
    sortMoves(ml);

    // search the moves
    for (int i = 0; i < (int)ml.size; i++) {
        Move move = ml.list[i];

        Piece captured = td->board.pieceAtB(move.to());
        // delta pruning, if the move + a large margin is still less then alpha we can safely skip this
        if (stand_pat + 400 + piece_values[EG][captured%6] < alpha && !move.promoted() && td->board.nonPawnMat(color)) continue;
        if (bestValue > VALUE_MATED_IN_PLY && captured != None && !see(move, -100, td->board)) continue;

        td->nodes++;
        td->board.makeMove(move);
        Score score = -qsearch(depth - 1, -beta, -alpha, ply + 1, td);
        td->board.unmakeMove(move);
        if (score > bestValue) {
            bestValue = score;
            if (score >= beta) return beta;
            if (score > alpha) {
                alpha = score;
            }
        }

    }
    return bestValue;
}

Score Search::absearch(int depth, Score alpha, Score beta, Stack *ss, ThreadData *td) {
    if (exit_early(td->nodes, td->id)) return 0;
    if (ss->ply > MAX_PLY - 1) return evaluation(td->board);

    TEntry tte;
    Score best = -VALUE_INFINITE;
    Score staticEval;
    Score oldAlpha = alpha;
    bool RootNode = ss->ply == 0;
    bool improving, ttHit;
    Color color = td->board.sideToMove;
    
    td->pv_length[ss->ply] = ss->ply;

    // Draw detection and mate pruning
    if (!RootNode) {
        if (td->board.halfMoveClock >= 100) return 0;
        if (td->board.isRepetition() && (ss-1)->currentmove != NULLMOVE) return - 3 + (td->nodes & 7);
        int all = popcount(td->board.All());
        if (all == 2) return 0;
        if (all == 3 && (td->board.Bitboards[WhiteKnight] || td->board.Bitboards[BlackKnight])) return 0;
        if (all == 3 && (td->board.Bitboards[WhiteBishop] || td->board.Bitboards[BlackBishop])) return 0;

        alpha = std::max(alpha, (int16_t)(-VALUE_MATE + ss->ply));
        beta = std::min(beta, (int16_t)(VALUE_MATE - ss->ply - 1));
        if (alpha >= beta) return alpha;
    }

    bool inCheck = td->board.isSquareAttacked(~color, td->board.KingSQ(color));
    bool PvNode = beta - alpha > 1;

    // Check extension
    if (inCheck) depth++;

    // Enter qsearch
    if (depth <= 0) return qsearch(15, alpha, beta, ss->ply, td);

    // Selective depth (heighest depth we have ever reached)
    if (ss->ply > td->seldepth) td->seldepth = ss->ply; 

    // Look up in the TT
    ttHit = false;
    probe_tt(tte, ttHit, td->board.hashKey, depth);

    // Adjust alpha and beta for non PV nodes
    if (!RootNode && !PvNode 
        && ttHit && tte.depth >= depth
        && (ss-1)->currentmove != NULLMOVE)
    {
        if (tte.flag == EXACT) return tte.score;
        else if (tte.flag == LOWERBOUND) {
            alpha = std::max(alpha, tte.score);
        }
        else if (tte.flag == UPPERBOUND) {
            beta = std::min(beta, tte.score);
        }
        if (alpha >= beta) return tte.score;
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
    improving = !inCheck && ss->ply >= 2 && staticEval > (ss-2)->eval;

    // Razoring
    if (!PvNode
        && depth < 2
        && staticEval + 240 < alpha
        && !inCheck)
        return qsearch(15, alpha, beta, ss->ply, td);

    // Reverse futility pruning
    if (std::abs(beta) < VALUE_MATE_IN_PLY && !inCheck && !PvNode) {
        if (depth < 7 && staticEval - 150 * depth + 100 * improving >= beta) return beta;
    }

    // Null move pruning
    if (td->board.nonPawnMat(color) 
        && (ss-1)->currentmove != NULLMOVE
        && depth >= 3 && !inCheck
        && staticEval >= beta) 
    {
        int r = 5 + depth / 5 + std::min(3, (staticEval - beta) / 256);
        td->board.makeNullMove();
        (ss)->currentmove = NULLMOVE;
        Score score = -absearch(depth - r, -beta, -beta + 1, ss+1, td);
        td->board.unmakeNullMove();
        if (score >= beta) return beta;
    }

    // IID 
    if (depth >= 4 && !ttHit)
        depth--;

    moves:
    Movelist ml = td->board.legalmoves();

    // if the move list is empty, we are in checkmate or stalemate
    if (ml.size == 0) {
        return inCheck ? -VALUE_MATE + ss->ply : 0;
    }

    // assign a value to each move
    for (int i = 0; i < ml.size; i++) {
        ml.list[i].value = score_move(ml.list[i], ss->ply, ttHit, td);
    }

    if (RootNode && td->id == 0) rootSize = ml.size;

    Move bestMove = Move(NONETYPE, NO_SQ, NO_SQ, false);
    Movelist quietMoves;
    uint16_t madeMoves = 0;
    Score score = 0;
    bool doFullSearch = false;


    for (int i = 0; i < ml.size; i++) {
        // sort the moves
        sortMoves(ml, i);

        Move move = ml.list[i];
        bool capture = td->board.pieceAtB(move.to()) != None;

        if (!capture) quietMoves.Add(move);

        int newDepth = depth - 1;
        // Pruning
        if (!RootNode
            && best > -VALUE_INFINITE) 
        {
            // late move pruning/movecount pruning
            if (!capture 
                && !inCheck
                && !PvNode
                && !move.promoted()
                && depth <= 4
                && quietMoves.size > (4 + depth * depth))
                continue;

            // See pruning
            if (depth < 6 
                && !see(move, -(depth * 100), td->board))
                continue;
        }

        td->nodes++;
        madeMoves++;

        if (td->id == 0 && RootNode && !stopped && elapsed() > 10000 ) 
            std::cout << "info depth " << depth - inCheck << " currmove " << td->board.printMove(move) << " currmovenumber " << madeMoves << "\n";

        td->board.makeMove(move);

	    U64 nodeCount = td->nodes;
        ss->currentmove = move.get();
        // bool givesCheck = td->board.isSquareAttacked(color, td->board.KingSQ(~color));

        // late move reduction
        if (depth >= 3 && !inCheck && madeMoves > 3 + 2 * PvNode) {
            int rdepth = reductions[madeMoves][depth];
            rdepth -= td->id % 2;
            rdepth = std::clamp(newDepth - rdepth, 1, newDepth + 1);

            score = -absearch(rdepth, -alpha - 1, -alpha, ss+1, td);
            doFullSearch = score > alpha;
        }
        else
            doFullSearch = !PvNode || madeMoves > 1;

        // do a full research if lmr failed or lmr was skipped
        if (doFullSearch) {
            score = -absearch(newDepth, -alpha - 1, -alpha, ss+1, td);
        }

        // PVS search
        if (PvNode && ((score > alpha && score < beta) || madeMoves == 1)) {
            score = -absearch(newDepth, -beta, -alpha, ss+1, td);
        }

        td->board.unmakeMove(move);
	    spentEffort[move.from()][move.to()] += td->nodes - nodeCount;

        if (score > best) {
            best = score;
            bestMove = move;

            // update the PV
            td->pv_table[ss->ply][ss->ply] = move;
            for (int next_ply = ss->ply + 1; next_ply < td->pv_length[ss->ply + 1]; next_ply++) {
                td->pv_table[ss->ply][next_ply] = td->pv_table[ss->ply + 1][next_ply];
            }
            td->pv_length[ss->ply] = td->pv_length[ss->ply + 1];

            if (score > alpha) {
                alpha = score;

                if (score >= beta) {
                    // update Killer Moves
                    if (!capture) {
                        td->killerMoves[1][ss->ply] = td->killerMoves[0][ss->ply];
                        td->killerMoves[0][ss->ply] = move;
                    }
                    break;
                }
            }
        }
    }
    if (best >= beta && td->board.pieceAtB(bestMove.to()) == None)
        UpdateHH(bestMove, depth, quietMoves, td);

    // Store position in TT
    if (!exit_early(td->nodes, td->id)) store_entry(depth, best, oldAlpha, beta, td->board.hashKey, bestMove.get());
    return best;
}

Score Search::aspiration_search(int depth, Score prev_eval, Stack *ss, ThreadData *td) {
    Score alpha = -VALUE_INFINITE;
    Score beta = VALUE_INFINITE;
    int delta = 30;

    Score result = 0;
    int research = 0;

    if (depth >= 5) {
        alpha = prev_eval - 50;
        beta = prev_eval + 50;
    }

    while (true) {
        if (stopped) return 0;
        if (alpha < -3500) alpha = -VALUE_INFINITE;
        if (beta  >  3500) beta  =  VALUE_INFINITE;
        result = absearch(depth, alpha, beta, ss, td);
        if (result <= alpha) {
            research++;
            alpha = std::max(alpha - research * research * delta, -((int)VALUE_INFINITE));
        }
        else if (result >= beta) {
            research++;
            beta = std::min(beta + research * research * delta, (int)VALUE_INFINITE);
        }
        else {
            return result;
        }
    }
}

void Search::iterative_deepening(int search_depth, uint64_t maxN, Time time, int threadId) {
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
    ThreadData* td = &this->tds[threadId];
    td->nodes = 0;
    td->seldepth = 0;

    Move reducedTimeMove = Move(NONETYPE, NO_SQ, NO_SQ, false);
    Move previousBestmove;
    bool adjustedTime;

    int result = -VALUE_INFINITE;
    int depth = 1;

    Stack stack[MAX_PLY+4], *ss = stack+2;
    std::memset(ss-2, 0, 4 * sizeof(Stack));

    for (int i = -2; i <= MAX_PLY + 1; ++i)
        (ss+i)->ply = i;
    
    for (depth = 1; depth <= search_depth; depth++)
    {
        result = aspiration_search(depth, result, ss, td);
        if (exit_early(td->nodes, td->id)) break;
        if (threadId != 0) continue;

        if (searchTime != 0)
        {
            if (rootSize == 1)
                searchTime = std::min((int64_t)50, searchTime);
            
            int effort = (spentEffort[previousBestmove.from()][previousBestmove.to()] * 100) / td->nodes;

            if (depth >= 8 && effort >= 95 && searchTime != 0  && !adjustedTime) {
                adjustedTime = true;
                searchTime = searchTime / 3 ;
                reducedTimeMove = td->pv_table[0][0];
            }

            if (adjustedTime && td->pv_table[0][0] != reducedTimeMove) {
                searchTime = startTime * 1.05f;
            }
        }

        previousBestmove = td->pv_table[0][0];
        auto ms = elapsed();
        uci_output(result, depth, td->seldepth, get_nodes(), ms, get_pv());
    }

    if (threadId == 0)
    {
        Move bestmove = depth == 1 ? td->pv_table[0][0] : previousBestmove;
        std::cout << "bestmove " << td->board.printMove(bestmove) << std::endl;
        stopped = true;
    }
    
    return;
}

void Search::start_thinking(Board board, int workers, int search_depth, uint64_t maxN, Time time) {
    stopped = true;
    for (std::thread& th: threads) {
        if (th.joinable())
            th.join();
    }
    threads.clear();
    stopped = false;
    // If we dont have previous data create default data
    for (int i = tds.size(); i < workers; i++) {
        ThreadData td;
        td.board = board;
        td.id = i;
        this->tds.push_back(td);
    }

    this->tds[0].board = board;
    this->tds[0].id = 0;
    this->threads.emplace_back(&Search::iterative_deepening, this, search_depth, maxN, time, 0);
    for (int i = 1; i < workers; i++) {
        this->tds[i].board = board;
        this->threads.emplace_back(&Search::iterative_deepening, this, search_depth, maxN, time, i);
    }
}

// Static Exchange Evaluation, logical based on Weiss (https://github.com/TerjeKir/weiss) licensed under GPL-3.0
bool Search::see(Move& move, int threshold, Board& board) {
    Square from = move.from();
    Square to = move.to();
    PieceType attacker = board.piece_type(board.pieceAtB(from));
    PieceType victim = board.piece_type(board.pieceAtB(to));
    int swap = piece_values[MG][victim] - threshold;
    if (swap < 0)
        return false;
    swap -= piece_values[MG][attacker];
    if (swap >= 0)
        return true;
    U64 occ = (board.All() ^ (1ULL << from)) | (1ULL << to);
    U64 attackers = board.allAttackers(to, occ) & occ;

    U64 bishops = board.Bishops(board.sideToMove) | board.Queens(board.sideToMove) 
                  | board.Bishops(~board.sideToMove) | board.Queens(~board.sideToMove);
    U64 rooks = board.Rooks(board.sideToMove) | board.Queens(board.sideToMove) 
                | board.Rooks(~board.sideToMove) | board.Queens(~board.sideToMove);
    Color sT = ~Color((board.pieceAtB(from) / 6));
    
    while (true) {
        attackers &= occ;
        U64 myAttackers = attackers & board.Us(sT);
        if (!myAttackers) break;
        
        int pt;
        for (pt = 0; pt <= 5; pt++) {
            if (myAttackers & (board.Bitboards[pt] | board.Bitboards[pt+6])) break;
        }
        sT = ~sT;
        if ((swap = -swap - 1 - piece_values[MG][pt]) >= 0) {
            if (pt == KING && (attackers & board.Us(sT))) sT = ~sT;
            break;
        }

        occ ^= (1ULL << (bsf(myAttackers & (board.Bitboards[pt] | board.Bitboards[pt+6]))));

        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= board.BishopAttacks(to, occ) & bishops;
        if (pt == ROOK || pt == QUEEN)
            attackers |= board.RookAttacks(to, occ) & rooks;
    }
    return sT != Color((board.pieceAtB(from) / 6));
}

int Search::mmlva(Move& move, Board& board) {
    int attacker = board.piece_type(board.pieceAtB(move.from())) + 1;
    int victim = board.piece_type(board.pieceAtB(move.to())) + 1;
    return mvvlva[victim][attacker];
}

int Search::score_move(Move& move, int ply, bool ttMove, ThreadData *td) {
    if (ttMove && move.get() == TTable[td->board.hashKey % TT_SIZE].move) {
        return 10'000'000;
    }
    else if (move.promoted()) {
        return 9'000'000 + move.piece();
    }
    else if (td->board.pieceAtB(move.to()) != None) {
        return see(move, -100, td->board) ? 7'000'000 + mmlva(move, td->board) : mmlva(move, td->board);
    }
    else if (td->killerMoves[0][ply] == move) {
        return killerscore1;
    }
    else if (td->killerMoves[1][ply] == move) {
        return killerscore2;
    }
    else {
        return td->history_table[td->board.sideToMove][move.from()][move.to()];
    }
}

int Search::score_qmove(Move& move, Board& board) {
    if (move.promoted()) {
        return 2147483647 - 20 + move.piece();
    }
    else if (board.pieceAtB(move.to()) != None) {
        return see(move, -100, board) ? mmlva(move, board) * 10000 : mmlva(move, board);
    }
    else {
        return 0;
    }
}

std::string Search::get_pv() {
    std::string line = "";
    for (int i = 0; i < tds[0].pv_length[0]; i++) {
        line += " ";
        line += tds[0].board.printMove(tds[0].pv_table[0][i]);
    }
    return line;
}

long long Search::elapsed()
{
    auto t1 = TimePoint::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

bool Search::exit_early(uint64_t nodes, int ThreadId) {
    if (stopped) return true;
    if (ThreadId != 0) return false;
    if (maxNodes != 0 && nodes >= maxNodes) {
        stopped = true;
        return true;
    }

    if (--checkTime > 0) return false;
    checkTime = 2047; 
       
    if (searchTime != 0) {
        auto ms = elapsed();
        if (ms >= searchTime || ms >= maxTime) {
            stopped = true;
            return true;
        }
    }
    return false;
}

uint64_t Search::get_nodes() {
    uint64_t nodes = 0;
    for (size_t i = 0; i < tds.size(); i++) {
        nodes += tds[i].nodes;
    }
    return nodes;
}

void Search::sortMoves(Movelist& moves){
    int i = moves.size;
    for (int cmove = 1; cmove < moves.size; cmove++){
        Move temp = moves.list[cmove];
        for (i=cmove-1; i>=0 && (moves.list[i].value < temp.value ); i--){
            moves.list[i+1] = moves.list[i];
        }
        moves.list[i+1] = temp;
    }
}

void Search::sortMoves(Movelist& moves, int sorted){
    int index = 0 + sorted;
    for (int i = 1 + sorted; i < moves.size; i++) {
        if (moves.list[i].value > moves.list[index].value) {
            index = i;
        }
    }
    std::swap(moves.list[index], moves.list[0 + sorted]);
}

std::string output_score(int score) {
    if (score >= VALUE_MATE_IN_PLY) {
        return "mate " + std::to_string(((VALUE_MATE - score) / 2) + ((VALUE_MATE - score) & 1));
    }
    else if (score <= VALUE_MATED_IN_PLY) {
        return "mate " + std::to_string(-((VALUE_MATE + score) / 2) + ((VALUE_MATE + score) & 1));
    }
    else {
        return "cp " + std::to_string(score);
    }
}

void uci_output(int score, int depth, uint8_t seldepth, U64 nodes, int time, std::string pv) {
    std::cout       << "info depth " << signed(depth)
    << " seldepth " << signed(seldepth)
    << " score "    << output_score(score)
    << " nodes "    << nodes 
    << " nps "      << signed((nodes / (time + 1)) * 1000) 
    << " time "     << time
    << " pv"        << pv << std::endl;
}
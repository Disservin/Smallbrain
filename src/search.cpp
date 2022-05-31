#include "search.h"
#include "evaluation.h"
#include <cstring>

void Search::perf_Test(int depth, int max) {
    auto t1 = std::chrono::high_resolution_clock::now();
    perft(depth, max);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\ntime: " << ms << "ms" << std::endl;
    std::cout << "Nodes: " << nodes << " nps " << (nodes / (ms + 1)) * 1000 << std::endl;
}

void Search::testAllPos() {
    auto t1 = std::chrono::high_resolution_clock::now();
    U64 total = 0;
    int passed = 0;
    board.applyFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    nodes = 0;
    perf_Test(6, 6);
    total += nodes;
    if (nodes == 119060324) {
        passed++;
        std::cout << "Startpos: passed" << std::endl;
    }
    else std::cout << "Startpos: failed" << std::endl;
    board.applyFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    nodes = 0;
    perf_Test(5, 5);
    total += nodes;
    if (nodes == 193690690) {
        passed++;
        std::cout << "Kiwipete: passed" << std::endl;
    }
    else std::cout << "Kiwipete: failed" << std::endl;
    board.applyFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ");
    nodes = 0;
    perf_Test(7, 7);
    total += nodes;
    if (nodes == 178633661) {
        passed++;
        std::cout << "Pos 3: passed" << std::endl;
    }
    else std::cout << "Pos 3: failed" << std::endl;
    board.applyFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    nodes = 0;
    perf_Test(6, 6);
    total += nodes;
    if (nodes == 706045033) {
        passed++;
        std::cout << "Pos 4: passed" << std::endl;
    }
    else std::cout << "Pos 4: failed" << std::endl;
    board.applyFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ");
    nodes = 0;
    perf_Test(5, 5);
    total += nodes;
    if (nodes == 89941194) {
        passed++;
        std::cout << "Pos 5: passed" << std::endl;
    }
    else std::cout << "Pos 5: failed" << std::endl;
    board.applyFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ");
    nodes = 0;
    perf_Test(5, 5);
    total += nodes;
    if (nodes == 164075551) {
        passed++;
        std::cout << "Pos 6: passed" << std::endl;
    }
    else std::cout << "Pos 6: failed" << std::endl;
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\n";
    std::cout << "Total time (ms)    : " << ms << std::endl;
    std::cout << "Nodes searched     : " << total << std::endl;
    std::cout << "Nodes/second       : " << (total / (ms + 1)) * 1000 << std::endl;
    std::cout << "Correct Positions  : " << passed << "/" << "6" << std::endl;
}

U64 Search::perft(int depth, int max) {
    Movelist ml = board.legalmoves();
    if (depth == 1) {
        return ml.size;
    }
    U64 nodesIt = 0;
    for (int i = 0; i < ml.size; i++) {
        Move move = ml.list[i];
        board.makeMove(move);
        nodesIt += perft(depth - 1, depth);
        board.unmakeMove(move);
        if (depth == max) {
            nodes += nodesIt;
            std::cout << board.printMove(move) << " " << nodesIt << std::endl;
            nodesIt = 0;
        }
    }
    return nodesIt;
}

int Search::qsearch(int depth, int alpha, int beta, int ply) {
    if (exit_early()) return 0;
    int stand_pat = evaluation(board);
    Color color = board.sideToMove;

    if (ply > MAX_PLY - 1) return stand_pat;

    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    if (depth == 0) return alpha;

    Movelist ml = board.capturemoves();

    // assign a value to each move
    for (int i = 0; i < ml.size; i++) {
        ml.list[i].value = mmlva(ml.list[i]);
    }

    // sort the moves
    sortMoves(ml);

    // search the moves
    for (int i = 0; i < (int)ml.size; i++) {
        Move move = ml.list[i];

        Piece captured = board.pieceAtB(move.to);
        // delta pruning, if the move + a large margin is still less then alpha we can safely skip this
        if (stand_pat + 400 + piece_values[EG][captured%6] < alpha && !move.promoted && board.nonPawnMat(color)) continue;

        nodes++;
        board.makeMove(move);
        int score = -qsearch(depth - 1, -beta, -alpha, ply + 1);
        board.unmakeMove(move);
        if (score >= beta) return beta;
        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

int Search::absearch(int depth, int alpha, int beta, int ply, bool null) {
    if (exit_early()) return 0;
    if (ply > MAX_PLY - 1) return evaluation(board);

    int best = -VALUE_INFINITE;
    pv_length[ply] = ply;
    int oldAlpha = alpha;
    bool RootNode = ply == 0;
    Color color = board.sideToMove;

    if (ply >= 1 && board.isRepetition() && !(ss[ply-1].currentmove == nullmove)) return 0;
    if (!RootNode){
        if (board.halfMoveClock >= 100) return 0;
        int all = popcount(board.All());
        if (all == 2) return 0;
        if (all == 3 && (board.Bitboards[WhiteKnight] || board.Bitboards[BlackKnight])) return 0;
        if (all == 3 && (board.Bitboards[WhiteBishop] || board.Bitboards[BlackBishop])) return 0;

        alpha = std::max(alpha, -VALUE_MATE + ply);
        beta = std::min(beta, VALUE_MATE - ply - 1);
        if (alpha >= beta) return alpha;
    }

    bool inCheck = board.isSquareAttacked(~color, board.KingSQ(color));
    bool PvNode = beta - alpha > 1;

    // Check extension
    if (inCheck) depth++;

    // Enter qsearch
    if (depth <= 0) return qsearch(15, alpha, beta, ply);

    // Selective depth (heighest depth we have ever reached)
    if (ply > seldepth) seldepth = ply;

    U64 index = board.hashKey % TT_SIZE;
    bool ttMove = false;

    int staticEval;
    // Look up in the TT
    if (TTable[index].key == board.hashKey && TTable[index].depth >= depth && !RootNode && !PvNode) {
        if (TTable[index].flag == EXACT) return TTable[index].score;
        else if (TTable[index].flag == LOWERBOUND) {
            alpha = std::max(alpha, TTable[index].score);
        }
        else if (TTable[index].flag == UPPERBOUND) {
            beta = std::min(beta, TTable[index].score);
        }
        if (alpha >= beta) return TTable[index].score;
        staticEval = TTable[index].score;
        // use TT move
        ttMove = true;
    }
    else {
        staticEval = evaluation(board);
    }

    // Reverse futility pruning
    if (std::abs(beta) < VALUE_MATE_IN_PLY && !inCheck && !PvNode) {
        if (depth < 7 && staticEval - 150 * depth >= beta) return beta;
    }

    // Null move pruning
    if (board.nonPawnMat(color) && !(ss[ply-1].currentmove == nullmove) && depth >= 3 && !inCheck) {
        int r = 3 + depth / 5;
        board.makeNullMove();
        ss[ply].currentmove = nullmove;
        int score = -absearch(depth - r, -beta, -beta + 1, ply + 1, true);
        board.unmakeNullMove();
        if (score >= beta) return beta;
    }

    Movelist ml = board.legalmoves();

    // if the move list is empty, we are in checkmate or stalemate
    if (ml.size == 0) {
        return inCheck ?-VALUE_MATE + ply : 0;
    }

    // assign a value to each move
    for (int i = 0; i < ml.size; i++) {
        ml.list[i].value = score_move(ml.list[i], ply, ttMove);
    }

    if (RootNode) rootSize = ml.size;

    // sort the moves
    sortMoves(ml, 0);

    uint16_t madeMoves = 0;
    uint16_t quietMoves = 0;
    int score = 0;
    bool doFullSearch = false;

    for (int i = 0; i < ml.size; i++) {
        Move move = ml.list[i];
        bool capture = board.pieceAtB(move.to) != None;

        if (!capture) quietMoves++;

        nodes++;
        madeMoves++;

        if (RootNode && elapsed() > 10000 && !stopped) {
            std::cout << "info depth " << depth << " currmove " << board.printMove(move) << " currmovenumber " << madeMoves << "\n";
        }

        board.makeMove(move);

	    U64 nodeCount = nodes;
        ss[ply].currentmove = move;
        bool givesCheck = board.isSquareAttacked(color, board.KingSQ(~color));

        // late move pruning/movecount pruning
        if (depth <= 4 && !PvNode
            && !capture && !inCheck && !givesCheck
            && quietMoves > lmpM[depth]
            && !move.promoted) {
            board.unmakeMove(move);
            continue;
        }

        // late move reduction
        if (depth >= 3 && !inCheck && madeMoves > 3 + 2 * PvNode) {
            int rdepth = reductions[madeMoves];
            rdepth = std::clamp(depth - 1 - rdepth, 1, depth - 2);

            // Decrease reduction for pvnodes
            if (PvNode)
                rdepth++;

            // Increase reduction for quiet moves
            if (madeMoves > 15 && !capture)
                rdepth--;

            score = -absearch(rdepth, -alpha - 1, -alpha, ply + 1, false);
            doFullSearch = score > alpha;
        }
        else
            doFullSearch = !PvNode || madeMoves > 1;

        // do a full research if lmr failed or lmr was skipped
        if (doFullSearch) {
            score = -absearch(depth - 1, -alpha - 1, -alpha, ply + 1, false);
        }

        // PVS search
        if (PvNode && ((score > alpha && score < beta) || madeMoves == 1)) {
            score = -absearch(depth - 1, -beta, -alpha, ply + 1, false);
        }

        board.unmakeMove(move);
	    spentEffort[move.from][move.to] += nodes - nodeCount;

        int bonus = std::clamp(depth * depth, 0, 400);

        if (!capture && score < best)
            history_table[color][move.piece][move.from][move.to] = std::clamp(history_table[color][move.piece][move.from][move.to] - 32 * bonus, -100000, 16384);

        if (score > best) {
            best = score;

            // update the PV
            pv_table[ply][ply] = move;
            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++) {
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
            }
            pv_length[ply] = pv_length[ply + 1];

            if (score > alpha) {
                alpha = score;

                // update History Table
                if (!capture)
                    history_table[color][move.piece][move.from][move.to] += 2 * (32 * bonus - history_table[color][move.piece][move.from][move.to] * bonus / 512);

                if (score >= beta) {
                    // update Killer Moves
                    if (!capture) {
                        killerMoves[1][ply] = killerMoves[0][ply];
                        killerMoves[0][ply] = move;
                    }
                    break;
                }
            }
        }

        sortMoves(ml, i + 1);
    }
    // Store position in TT
    store_entry(index, depth, best, oldAlpha, beta, board.hashKey, ply);
    return best;
}

int Search::aspiration_search(int depth, int prev_eval) {
    int alpha = -VALUE_INFINITE;
    int beta = VALUE_INFINITE;
    int delta = 30;

    int result = 0;
    int ply = 0;
    int research = 0;

    if (depth == 1) {
        result = -absearch(1, alpha, beta, 0, false);
        return result;
    }
    else if (depth >= 5) {
        alpha = prev_eval - 50;
        beta = prev_eval + 50;
    }

    while (true) {
        if (alpha < -3500) alpha = -VALUE_INFINITE;
        if (beta  >  3500) beta  =  VALUE_INFINITE;
        result = absearch(depth, alpha, beta, ply, false);
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

int Search::iterative_deepening(int search_depth, Time time) {
    int result = 0;
    seldepth = 0;
    startAge = board.fullMoveNumber;
    Move prev_bestmove{};
    searchTime = time.optimum;
    maxTime = time.maximum;
    memset(spentEffort, 0, sizeof(unsigned long long) * 64 * 64);

    // reuse previous pv information
   if (pv_length[0] > 0) {
        for (int i = 1; i < pv_length[0]; i++) {
            pv[i-1] = pv[i];
        }
    }

    t0 = std::chrono::high_resolution_clock::now();

    U64 startNodes =  nodes;
    bool adjustedTime = false;
    Move reduceTimeMove = nullmove;
    int64_t startTime = searchTime;

    for (int depth = 1; depth <= search_depth; depth++) {
        result = aspiration_search(depth, result);
        // Can we exit the search?
        if (exit_early()) {
            std::string move = board.printMove(prev_bestmove);
            if (depth == 1) std::cout << "bestmove " << board.printMove(pv_table[0][0]) << std::endl;
            else std::cout << "bestmove " << move << std::endl;
            stopped = true;
            return 0;
        }

        if (rootSize == 1) {
            searchTime = std::min((int64_t)50, searchTime);
        }


        int effort = nodes - startNodes == 0 ? 0 : (spentEffort[prev_bestmove.from][prev_bestmove.to] * 100) / (nodes - startNodes);

        if (depth >= 8 && effort >= 95 && searchTime != 0 && !adjustedTime) {
            adjustedTime = true;

            searchTime = searchTime / 3 ;

            reduceTimeMove = prev_bestmove;
        }



        // if the bestmove changed after reducing the time we want to spent some more time on that move
        if (!(prev_bestmove == reduceTimeMove) && adjustedTime) {
            searchTime = startTime * 1.05f;
        }

        // Update the previous best move and print information
        prev_bestmove = pv_table[0][0];
        auto ms = elapsed();
        uci_output(result, depth, ms);
    }
    std::cout << "bestmove " << board.printMove(prev_bestmove) << std::endl;
    stopped = true;
    return 0;
}

int Search::mmlva(Move& move) {
    static constexpr int mvvlva[7][7] = { {0, 0, 0, 0, 0, 0, 0},
    {0, 2005, 2004, 2003, 2002, 2001, 2000},
    {0, 3005, 3004, 3003, 3002, 3001, 3000},
    {0, 4005, 4004, 4003, 4002, 4001, 4000},
    {0, 5005, 5004, 5003, 5002, 5001, 5000},
    {0, 6005, 6004, 6003, 6002, 6001, 6000},
    {0, 7005, 7004, 7003, 7002, 7001, 7000} };
    int attacker = board.piece_type(board.pieceAtB(move.from)) + 1;
    int victim = board.piece_type(board.pieceAtB(move.to)) + 1;
    if (victim == NONETYPE) {
        victim = PAWN;
    }
    return mvvlva[victim][attacker];
}

int Search::score_move(Move& move, int ply, bool ttMove) {
    if (move == pv[ply]) {
        return 2147483647;
    }
    else if (ttMove && move == TTable[board.hashKey % TT_SIZE].move) {
        return 2147483647 - 1;
    }
    else if (move.promoted) {
        return 2147483647 - 10 + move.piece;
    }
    else if (board.pieceAtB(move.to) != None) {
        return mmlva(move) * 1000;
    }
    else if (move == pv_table[0][ply]) {
        return 1000000;
    }
    else if (killerMoves[0][ply] == move) {
        return killerscore1;
    }
    else if (killerMoves[1][ply] == move) {
        return killerscore2;
    }
    else if (history_table[board.sideToMove][move.piece][move.from][move.to]) {
        return history_table[board.sideToMove][move.piece][move.from][move.to];
    }
    else {
        return 0;
    }
}

std::string Search::get_pv() {
    std::string line = "";
    for (int i = 0; i < pv_length[0]; i++) {
        pv[i] = pv_table[0][i];
        line += board.printMove(pv_table[0][i]);
        line += " ";
    }
    return line;
}

bool Search::store_entry(U64 index, int depth, int bestvalue, int old_alpha, int beta, U64 key, uint8_t ply) {
    if (!exit_early() && !(bestvalue >= 19000) && !(bestvalue <= -19000) &&
        (TTable[index].depth < depth || TTable[index].age + 3 <= startAge)) {
        TTable[index].flag = EXACT;
        // Upperbound
        if (bestvalue <= old_alpha) {
            TTable[index].flag = UPPERBOUND;
        }
        // Lowerbound
        else if (bestvalue >= beta) {
            TTable[index].flag = LOWERBOUND;
        }
        TTable[index].depth = depth;
        TTable[index].score = bestvalue;
        TTable[index].age = startAge;
        TTable[index].key = key;
        TTable[index].move = pv_table[0][ply];
        return true;
    }
    return false;
}

void sortMoves(Movelist& moves){
    int i = moves.size;
    for (int cmove = 1; cmove < moves.size; cmove++){
        Move temp = moves.list[cmove];
        for (i=cmove-1; i>=0 && (moves.list[i].value < temp.value ); i--){
            moves.list[i+1] = moves.list[i];
        }
        moves.list[i+1] = temp;
    }
}

void sortMoves(Movelist& moves, int sorted){
    int index = 0 + sorted;
    for (int i = 1 + sorted; i < moves.size; i++) {
        if (moves.list[i].value > moves.list[index].value) {
            index = i;
        }
    }
    std::swap(moves.list[index], moves.list[0 + sorted]);
}

bool Search::exit_early() {
    if (stopped) return true;
    if (nodes & 2047 && searchTime != 0) {
        auto ms = elapsed();
        if (ms >= searchTime || ms >= maxTime) {
            stopped = true;
            return true;
        }
    }
    return false;
}

void Search::uci_output(int score, int depth, int time) {
    std::cout << "info depth " << signed(depth)
    << " seldepth " << signed(seldepth);
    if (score >= VALUE_MATE_IN_PLY){
        std::cout << " score cp " << "mate " << ((VALUE_MATE - score) / 2) + ((VALUE_MATE - score) & 1);
    }
    else if (score <= VALUE_MATED_IN_PLY) {
        std::cout << " score cp " << "mate " << -((VALUE_MATE + score) / 2) + ((VALUE_MATE + score) & 1);
    }
    else{
        std::cout << " score cp " << score;
    }
    std::cout << " nodes " << nodes << " nps "
    << signed((nodes / (time + 1)) * 1000) << " time "
    << time
    << " pv " << get_pv() << std::endl;
}

long long Search::elapsed(){
    auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}
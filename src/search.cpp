#include "search.h"
#include "evaluation.h"

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

int Search::qsearch(int depth, int alpha, int beta, int player, int ply) {
    if (exit_early()) return 0;
    int stand_pat = evaluation(board) * player;
    Color color = player == 1 ? White : Black;

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
        int score = -qsearch(depth - 1, -beta, -alpha, -player, ply + 1);
        board.unmakeMove(move);
        if (score >= beta) return beta;
        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}

int Search::absearch(int depth, int alpha, int beta, int player, int ply, bool null) {
    if (exit_early()) return 0;
    if (ply > MAX_PLY - 1) return evaluation(board);

    int best = -VALUE_INFINITE;
    Color color = player == 1 ? White : Black;
    pv_length[ply] = ply;
    int oldAlpha = alpha;
    bool RootNode = ply == 0;

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
    if (depth <= 0) return qsearch(15, alpha, beta, player, ply);

    // Selective depth (heighest depth we have ever reached)
    if (ply > seldepth) seldepth = ply;

    U64 index = board.hashKey % TT_SIZE;
    bool ttMove = false;

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
        // use TT move
        ttMove = true;
    }

    // Razor-like pruning
    if (std::abs(beta) < VALUE_MATE_IN_PLY && !inCheck && !PvNode) {
        int staticEval = evaluation(board) * player;
        if (staticEval - 120 * depth >= beta) return beta;
    }

    // Null move pruning
    if (board.nonPawnMat(color) && !(ss[ply-1].currentmove == nullmove) && depth >= 3 && !inCheck) {
        int r = 4 + depth / 6;
        board.makeNullMove();
        ss[ply].currentmove = nullmove;
        int score = -absearch(depth - r, -beta, -beta + 1, -player, ply + 1, true);
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

    // sort the moves
    sortMoves(ml, 0);

    uint16_t madeMoves = 0;
    int score = 0;
    bool doFullSearch = false;

    for (int i = 0; i < ml.size; i++) {
        Move move = ml.list[i];
        bool capture = board.pieceAtB(move.to) != None;

        nodes++;
        madeMoves++;
        board.makeMove(move);
        ss[ply].currentmove = move;

        // late move pruning/movecount pruning
        if (depth <= 3 && !PvNode
            && !inCheck && madeMoves > lmpM[depth]
            && !(board.isSquareAttacked(color, board.KingSQ(~color)))
            && !capture
            && !move.promoted) {
            board.unmakeMove(move);
            continue;
        }

        // late move reduction
        if (depth >= 3 && !PvNode && !inCheck && madeMoves > 2 + 2 * RootNode) {
            int rdepth = reductions[madeMoves];
            rdepth = std::clamp(depth - 1 - rdepth, 1, depth - 2);
            score = -absearch(rdepth, -alpha - 1, -alpha, -player, ply + 1, false);
            doFullSearch = score > alpha;
        }
        else
            doFullSearch = !PvNode || madeMoves > 1;

        // do a full research if lmr failed or lmr was skipped
        if (doFullSearch) {
            score = -absearch(depth - 1, -alpha - 1, -alpha, -player, ply + 1, false);
        }

        // PVS search
        if (PvNode && ((score > alpha && score < beta) || madeMoves == 1)) {
            score = -absearch(depth - 1, -beta, -alpha, -player, ply + 1, false);
        }

        board.unmakeMove(move);

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
                if (!capture) {
                    history_table[color][move.piece][move.from][move.to] += depth;
                }
                
                if (score >= beta) {
                    // update Killer Moves
                    if (!capture) {
                        killerMoves[1][ply] = killerMoves[0][ply];
                        killerMoves[0][ply] = move;
                    }
                    return score;
                }
            }
        }
        sortMoves(ml, i + 1);
    }
    // Store position in TT
    store_entry(index, depth, best, oldAlpha, beta, board.hashKey, ply);
    return best;
}

int Search::aspiration_search(int player, int depth, int prev_eval) {
    int alpha = -VALUE_INFINITE;
    int beta = VALUE_INFINITE;
    int result = 0;
    int ply = 0;
    // Start search with full sized window
    if (depth == 1) {
        result = absearch(1, -VALUE_INFINITE, VALUE_INFINITE, player, ply, false);
    }
    else {
        // Use previous evaluation as a starting point and search with a smaller window
        alpha = prev_eval - 100;
        beta = prev_eval + 100;
        result = absearch(depth, alpha, beta, player, ply, false);
    }
    // In case the result is outside the bounds we have to do a research
    if (result <= alpha) {
        return absearch(depth, -VALUE_INFINITE, beta, player, ply, false);
    }
    if (result >= beta) {
        return absearch(depth, alpha, VALUE_INFINITE, player, ply, false);
    }
    return result;
}

int Search::iterative_deepening(int search_depth, bool bench, long long time) {
    int result = 0;
    Color color = board.sideToMove;
    auto t1 = std::chrono::high_resolution_clock::now();
    Move prev_bestmove{};
    searchTime = time;
    if (bench) {
        start_bench();
        return 0;
    }

    int player = color == White ? 1 : -1;
    seldepth = 0;
    startAge = board.fullMoveNumber;
    for (int depth = 1; depth <= search_depth; depth++) {
        searchDepth = depth;

        result = aspiration_search(player, depth, result);

        // Can we exit the search?
        if (exit_early()) {
            std::string move = board.printMove(prev_bestmove);
            if (depth == 1) std::cout << "bestmove " << board.printMove(pv_table[0][0]) << std::endl;
            else std::cout << "bestmove " << move << std::endl;
            stopped = true;
            return 0;
        }
        // Update the previous best move and print information
        prev_bestmove = pv_table[0][0];
        auto t2 = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
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
        return 2147483647 - 3;
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
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        if (ms >= searchTime) {
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

inline bool operator==(Move& m, Move& m2) {
    return m.piece == m2.piece && m.from == m2.from && m.to == m2.to && m.promoted == m2.promoted;
}

//Benchmarks from Bitgenie
std::string benchmarkfens[50] = {
    "r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
    "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
    "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
    "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
    "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
    "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
    "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
    "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
    "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
    "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
    "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
    "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
    "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
    "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
    "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
    "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
    "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
    "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
    "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
    "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
    "r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
    "r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
    "r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
    "r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
    "r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
    "r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
    "r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
    "3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
    "5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
    "8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
    "8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
    "8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
    "8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
    "8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
    "8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4 w - - 6 54",
    "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
    "8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
    "8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
    "1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
    "4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
    "r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
    "2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
    "6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
    "2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
    "r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
    "6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
    "rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
    "2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
    "3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
    "2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93"
};

int Search::start_bench() {
    U64 totalNodes = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int positions = 0; positions < 50; positions++) {
        board.applyFen(benchmarkfens[positions]);
        Color color = board.sideToMove;
        int player = color == White ? 1 : -1;
        seldepth = 0;
        nodes = 0;
        int result = 0;
        stopped = false;

        std::cout << "\nPosition: " << positions + 1 << "/50 " << benchmarkfens[positions] << std::endl;
        
        t0 = std::chrono::high_resolution_clock::now();
        for (int depth = 1; depth <= 7; depth++) {
            searchDepth = depth;
            result = aspiration_search(player, depth, result);
            auto s2 = std::chrono::high_resolution_clock::now();
            auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(s2 - t0).count();
            uci_output(result, depth, ms2);
        }
        totalNodes += nodes;
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\n" << totalNodes << " nodes " << signed((totalNodes / (ms + 1)) * 1000) << " nps " << std::endl;
    return 0;
}
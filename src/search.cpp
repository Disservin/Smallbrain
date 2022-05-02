#include "search.h"
#include "evaluation.h"

void Search::perf_Test(int depth, int max){
    perft(depth, max);
    std::cout << "Nodes: " << nodes << std::endl;
}

U64 Search::perft(int depth, int max){
    Movelist ml = board.legalmoves();
    if(depth == 1){
        return ml.size;
    }
    U64 nodesIt = 0;
    for(int i = 0; i < ml.size; i++){
        Move move = ml.list[i];
        board.makeMove(move);
        nodesIt += perft(depth - 1, depth);
        board.unmakeMove(move);
        if (depth == max){
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

    if (ply > MAX_PLY - 1) return stand_pat;
    
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    if (depth == 0) return alpha;

    Movelist ml = board.capturemoves();

    for (int i = 0; i < ml.size; i++) {
        ml.list[i].value = mmlva(ml.list[i]);
    }


    std::sort(std::begin(ml.list), ml.list + ml.size, [&](const Move& m1, const Move& m2)
        {return m1.value > m2.value; });

    for (int i = 0; i < (int)ml.size; i++) {
        Move move = ml.list[i];
        nodes++;
        board.makeMove(move);
        int score = -qsearch(depth - 1, -beta, -alpha, -player, ply + 1);
        board.unmakeMove(move);
        if (score > alpha) {
            alpha = score;
            if (score >= beta) break;
        }
    }
    return stand_pat;
}

int Search::absearch(int depth, int alpha, int beta, int player, int ply, bool null) {
    if (exit_early()) return 0;	
	
    int best = -VALUE_INFINITE;
    Color color = player == 1 ? White : Black;
    pv_length[ply] = ply;	
	
    if (ply == 0 && board.isRepetition(3)) return 0;
    if (ply >= 1 && board.isRepetition()) return 0;

    bool inCheck = board.isSquareAttacked(~color, board.KingSQ(color));
    bool PvNode = beta - alpha > 1;
    bool RootNode = ply == 0;
	
	if (inCheck && depth <= 0) depth++;
    if (depth == 0 && !inCheck) return qsearch(10, alpha, beta, player, ply);
	
    int staticEval = evaluation(board) * player;

    if (board.nonPawnMat(color) && !null && depth >= 3 && !inCheck && !PvNode) {
        int r = depth > 6 ? 3 : 2;
        board.makeNullMove();
        int score = -absearch(depth - 1 - r, -beta, -beta + 1, -player, ply + 1, true);
        board.unmakeNullMove();
        if (score >= beta) return beta;
    }

    if (std::abs(beta) < VALUE_MATE_IN_PLY && !inCheck && !PvNode) {
        if (staticEval - 150 * depth >= beta) return beta;
    }

    Movelist ml = board.legalmoves();
    
    if (ml.size == 0) {
        if (inCheck) return -VALUE_MATE + ply;
        return 0;
    }

    for (int i = 0; i < ml.size; i++) {
        ml.list[i].value = score_move(ml.list[i], ply);
    }

    std::sort(std::begin(ml.list), ml.list + ml.size, [&](const Move& m1, const Move& m2)
        {return m1.value > m2.value; });

    uint16_t madeMoves = 0;
    int score = 0;
    bool doFullSearch = false;

	for (int i = 0; i < ml.size; i++) {
        Move move = ml.list[i];
        bool capture = board.pieceAtB(move.to) != None;
        
        nodes++;
        madeMoves++;
		board.makeMove(move);
        
        if (depth >= 3 && !PvNode && !inCheck && madeMoves > 3 + 2 * RootNode) {
            score = -absearch(depth - 2, -alpha - 1, -alpha, -player, ply + 1, false);
            doFullSearch = score > alpha;
        }
        else
            doFullSearch = !PvNode || madeMoves > 1;

        if (doFullSearch) {
            score = -absearch(depth - 1, -alpha - 1, -alpha, -player, ply + 1, false);
        }

        if (PvNode && ((score > alpha && score < beta) || madeMoves == 1)) {
            score = -absearch(depth - 1, -beta, -alpha, -player, ply + 1, false);
        }
		
        board.unmakeMove(move);	

		if (score > best) {
            best = score;

            pv_table[ply][ply] = move;
            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++) {
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
            }
            pv_length[ply] = pv_length[ply + 1];

            if (score > alpha) {
                alpha = score;
                if (!capture) {
                    history_table[color][move.from][move.to] += depth * depth;
                }
                if (score >= beta) {
                    if (!capture) {
                        killerMoves[1][ply] = killerMoves[0][ply];
                        killerMoves[0][ply] = move;
                    }
                    return score;
                }
            }
		}
	}
    return best;
}

int Search::aspiration_search(int player, int depth, int prev_eval) {
    int alpha = -VALUE_INFINITE;
    int beta = VALUE_INFINITE;
    int result = 0;
    int ply = 0;
    if (depth == 1) {
        result = absearch(1, -VALUE_INFINITE, VALUE_INFINITE, player, ply, false);
    }
    else {
        alpha = prev_eval - 100;
        beta = prev_eval + 100;
        result = absearch(depth, alpha, beta, player, ply, false);
    }
    if (result <= alpha || result >= beta) {
        return absearch(depth, -VALUE_INFINITE, VALUE_INFINITE, player, ply, false);
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

    for (int depth = 1; depth <= search_depth; depth++) {
        searchDepth = depth;
        
        result = aspiration_search(player, depth, result);
        
        if (exit_early()) {
            std::string move = board.printMove(prev_bestmove);
            if (depth == 1) std::cout << "bestmove " << board.printMove(pv_table[0][0]) << std::endl; 
            else std::cout << "bestmove " << move << std::endl;
            stopped = true;
            return 0;
        }
        prev_bestmove = pv_table[0][0];
        std::string move = board.printMove(pv_table[0][0]);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "info depth " << signed(depth)
            << " score cp " << result
            << " nodes " << nodes << " nps "
            << signed((nodes / (ms + 1)) * 1000) << " time "
            << ms
            << " pv " << get_pv() << std::endl;
    }
    std::cout << "bestmove " << board.printMove(prev_bestmove) << std::endl;
    stopped = true;
    return 0;
}

int Search::start_bench() {
    int result = 0;
    Color color = board.sideToMove;
    int player = color == White ? 1 : -1;

    auto t1 = std::chrono::high_resolution_clock::now();
    for (int depth = 1; depth <= 5; depth++) {
        searchDepth = depth;
        result = aspiration_search(player, depth, result);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << nodes << " nodes " << signed((nodes / (ms + 1)) * 1000) << " nps " << std::endl;
    return 0;
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

int Search::mmlva(Move& move) {
    static constexpr int mvvlva[7][7] = { {0, 0, 0, 0, 0, 0, 0},
    {0, 205, 204, 203, 202, 201, 200},
    {0, 305, 304, 303, 302, 301, 300},
    {0, 405, 404, 403, 402, 401, 400},
    {0, 505, 504, 503, 502, 501, 500},
    {0, 605, 604, 603, 602, 601, 600},
    {0, 705, 704, 703, 702, 701, 700} };
    int attacker = board.piece_type(board.pieceAtB(move.from)) + 1;
    int victim = board.piece_type(board.pieceAtB(move.to)) + 1;
    if (victim == -1) {
        victim = 0;
    }
    return mvvlva[victim][attacker];
}

int Search::score_move(Move& move, int ply) {
    int IsWhite = board.sideToMove ? 0 : 1;
    if (move == pv_table[0][ply]) {
        return 100000;
    }
    else if (move.promoted) {
        return 80000;
    }
    else if (board.pieceAtBB(move.to) != None) {
        return mmlva(move) * 100;
    }
    else if (killerMoves[0][ply] == move) {
        return 5000;
    }
    else if (killerMoves[1][ply] == move) {
        return 4000;
    }
    else if (history_table[IsWhite][move.from][move.to]) {
        return history_table[IsWhite][move.from][move.to];
    }
    else {
        return 0;
    }
}

std::string Search::get_pv() {
    std::string line = "";
    for (int i = 0; i < pv_length[0]; i++) {
        line += board.printMove(pv_table[0][i]);
        line += " ";
    }
    return line;
}

inline bool operator==(Move& m, Move& m2) {
    return m.piece == m2.piece && m.from == m2.from && m.to == m2.to && m.promoted == m2.promoted;
}
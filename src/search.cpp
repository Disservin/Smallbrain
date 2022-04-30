#include "search.h"
#include "evaluation.h"

//int Search::qsearch(int depth, int alpha, int beta, int player, int ply) {
//    if (exit_early()) return 0;
//    int stand_pat = evaluation(board) * player;
//    Color color = player == 1 ? White : Black;
//    Square king_sq = board.KingSQ(color);
//    bool inCheck = board.isSquareAttacked(color, king_sq);
//    if (inCheck) {
//        stand_pat = -VALUE_MATE + ply;
//        Movelist moveList = board.legalmoves();
//        if (moveList.size == 0) return -VALUE_MATE + ply;
//    }
//    else {
//        if (stand_pat >= beta) return beta;
//        if (alpha < stand_pat) alpha = stand_pat;
//    }
//    if (depth == 0) return stand_pat;
//
//    Moves moveList = board.generateNonQuietLegalMoves<color>();
//
//    std::sort(std::begin(moveList.moves), moveList.moves + moveList.count, [&](const Move& m1, const Move& m2)
//        {return mmlva(m1) > mmlva(m2); });
//
//    for (int i = 0; i < (int)moveList.count; i++) {
//        Move move = moveList.moves[i];
//        nodes++;
//        board.makemove<color>(move);
//        int score = -qsearch<~color>(depth - 1, -beta, -alpha, -player, ply + 1);
//        board.unmakemove<color>(move);
//        if (score > stand_pat) {
//            stand_pat = score;
//            if (score > alpha) {
//                alpha = score;
//                if (score >= beta) break;
//            }
//        }
//    }
//    return stand_pat;
//}

int Search::absearch(int depth, int alpha, int beta, int player, int ply) {
    if (exit_early()) return 0;	
	
    int best = -VALUE_INFINITE;
    Color color = player == 1 ? White : Black;
    pv_length[ply] = ply;	
	
    if (depth == 0) return evaluation(board);
	
    Movelist ml = board.legalmoves();
    for (int i = 0; i < ml.size; i++) {
        ml.list[i].value = score_move(ml.list[i], ply);
    }
    std::sort(std::begin(ml.list), ml.list + ml.size, [&](const Move& m1, const Move& m2)
        {return m1.value > m2.value; });
	
	for (int i = 0; i < ml.size; i++) {
        Move move = ml.list[i];
        bool capture = board.pieceAtBB(move.to) != None;
		board.makeMove(move);
        nodes++;
		int score = -absearch(depth - 1, -beta, -alpha, -player, ply + 1);
		board.unmakeMove(move);
        		
		if (score > best) {
            best = score;
            if (depth == searchDepth) {
                bestMove = move;
            }

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
        result = absearch(1, -VALUE_INFINITE, VALUE_INFINITE, player, ply);
    }
    else {
        alpha = prev_eval - 100;
        beta = prev_eval + 100;
        result = absearch(depth, alpha, beta, player, ply);
    }
    if (result <= alpha || result >= beta) {
        return absearch(depth, -VALUE_INFINITE, VALUE_INFINITE, player, ply);
    }
    return result;
}

int Search::iterative_deepening(int search_depth, bool bench, unsigned long long time) {
    int result = 0;
    Color color = board.sideToMove;
    auto t1 = std::chrono::high_resolution_clock::now();
    Move prev_bestmove;
    searchTime = time;
    if (bench) {
        start_bench();
        return 0;
    }
    for (int depth = 1; depth <= search_depth; depth++) {
        searchDepth = depth;
        int player = color == White ? 1 : -1;
        result = absearch(depth, -VALUE_INFINITE, VALUE_INFINITE, player, 0);

        if (exit_early()) {
            std::string move = board.printMove(prev_bestmove);
            std::cout << "bestmove " << move << std::endl;
            break;
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
            << " pv " << move << std::endl;
    }
    return 0;
}

int Search::start_bench() {
    int result = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int depth = 1; depth <= 5; depth++) {
        searchDepth = depth;
        result = aspiration_search(-1, depth, result);
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

int Search::mmlva(Move move) {
    static constexpr int mvvlva[7][7] = { {0, 0, 0, 0, 0, 0, 0},
    {0, 205, 204, 203, 202, 201, 200},
    {0, 305, 304, 303, 302, 301, 300},
    {0, 405, 404, 403, 402, 401, 400},
    {0, 505, 504, 503, 502, 501, 500},
    {0, 605, 604, 603, 602, 601, 600},
    {0, 705, 704, 703, 702, 701, 700} };
    int attacker = board.piece_type(board.pieceAtBB(move.from)) + 1;
    int victim = board.piece_type(board.pieceAtBB(move.to)) + 1;
    if (victim == -1) {
        victim = 0;
    }
    return mvvlva[victim][attacker];
}

int Search::score_move(Move move, int ply) {
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

inline bool operator==(Move& m, Move& m2) {
    return m.piece == m2.piece && m.from == m2.from && m.to == m2.to && m.promoted == m2.promoted;
}
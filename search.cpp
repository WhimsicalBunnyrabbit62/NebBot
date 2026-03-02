#include "Types.h"
#include "search.h"
#include "eval.h"
#include "moveGen.h"
#include "board.h"
#include <vector>
#include <iostream>
#include <climits>
#include <chrono>
#include <algorithm>

static constexpr int NEG_INF = -30000;
static constexpr int POS_INF = 30000;

static int nodesLookedAt = 0;
bool stopSearch = false;
Move curBestMove;

static const int MvvLvaScores[7][7] = {
// Attackers: [placeholder, P, N, B, R, Q, K]
    {0,  0,  0,  0,  0,  0,  0},   // Victim: placeholder 
    {0, 15, 14, 13, 12, 11, 10},   // Victim: P
    {0, 25, 24, 23, 22, 21, 20},   // Victim: N
    {0, 35, 34, 33, 32, 31, 30},   // Victim: B
    {0, 45, 44, 43, 42, 41, 40},   // Victim: R
    {0, 55, 54, 53, 52, 51, 50},   // Victim: Q
    {0, 65, 64, 63, 62, 61, 60}    // Victim: K
};

Move search::startSearch(Board& board, int maxTimeMs) {
    auto start = std::chrono::steady_clock::now();
    nodesLookedAt = 0;
    stopSearch = false;

    MoveList allMoves;
    moveGen::generateMoves(board, allMoves);

    if (allMoves.empty()) {
        int kingSq = moveGen::findKing(board, board.turn);
        int enemy = (board.turn == WHITE) ? BLACK : WHITE;
        if (kingSq != -1 && moveGen::isSquareAttacked(kingSq, enemy, board)) {
            return {-1, -1, NONE, 0};
        } else {
            return {-1, -1, NONE, 0};
        }

        std::cout << "no move" << std::endl;
    }

    int bestEval = NEG_INF;
    Move bestMove = allMoves.moves[0];
    
    for (int curDepth = 1; curDepth <= 64; curDepth++) {
        int curBestEval = NEG_INF;
        Move curBestMove;

        for (Move m : allMoves) {
            StateInfo s = board.makeMove(m);

            nodesLookedAt++;
            int score = -negamax(curDepth - 1, board, NEG_INF, POS_INF, start, maxTimeMs);
            board.unmakeMove(m, s);

            if (stopSearch) break;
            
            if (score > curBestEval) {
                curBestEval = score;
                curBestMove = m;
            }
        }

        if (!stopSearch) {
            bestMove = curBestMove;
            std::cout << "depth: " << curDepth << ". Best Eval: " << curBestEval << std::endl;
        } else {
            break;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        if (elapsed > maxTimeMs/2) break;
    }

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Time taken (Engine Move) (ms): " << diff.count() << std::endl;
    std::cout << "Nodes looked at (Engine Move): " << nodesLookedAt << std::endl;

    return bestMove;
}

int search::negamax(int depth, Board& board, int alpha, int beta, std::chrono::steady_clock::time_point startTime, int limit) {
    // duration cast -> convert nanoseconds to manageable numbers in this case ms
    // chrono steady clock now -> get current time 
    // - start -> difference from the time at the beginning
    // .count() -> from a chrono object into a number (long here)
    if (nodesLookedAt % 2048 == 0) {
        long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= limit) stopSearch = true;
    }

    if (stopSearch) return 0;

    int best = NEG_INF;
    MoveList moves;
    moveGen::generateMoves(board, moves);

    if (depth <= 0) {
        return qSearch(board, alpha, beta);
    }

    if (moves.empty()) {
        int kingSq = moveGen::findKing(board, board.turn);
        int enemy = (board.turn == WHITE) ? BLACK : WHITE;

        if (kingSq != -1 && moveGen::isSquareAttacked(kingSq, enemy, board)) {
            return NEG_INF;
        } else {
            return 0;
        }
    }

    for (Move m : moves) {
        StateInfo s = board.makeMove(m);
        
        nodesLookedAt++;

        int score = -negamax(depth-1, board, -beta, -alpha, startTime, limit);
        board.unmakeMove(m, s);

        best = std::max(best, score);
        alpha = std::max(alpha, score);

        if (alpha >= beta) break;
    }

    return best;
}

inline int pieceTypeIndex(int piece) {
    if (piece == EMPTY) return 0;
    return (piece < 7) ? piece : piece - 8;
}

int getMvvLvaScore(Board& board, Move m) {
    int victim = pieceTypeIndex(board.squares[m.to]);
    int attacker = pieceTypeIndex(board.squares[m.from]);

    return MvvLvaScores[victim][attacker];
}

int search::qSearch(Board& board, int alpha, int beta) {
    int standPat = eval::evaluate(board) * board.turn; // Eval if player does nothing

    if (standPat >= beta) return beta; // beta -> lowest score opponent will let us reach
    if (standPat >= alpha) alpha = standPat; // alpha -> best we can get;

    MoveList captures;
    MoveList allMoves;
    moveGen::generateMoves(board, allMoves);
    moveGen::generateCaptures(board, allMoves, captures);

    //MVV-LVA sort here

    for (int i = 0; i < captures.size(); i++) {
        int bestIdx = i;    
        for (int j = i + 1; j < captures.size(); j++) {
            if (getMvvLvaScore(board, captures.moves[j]) > getMvvLvaScore(board, captures.moves[bestIdx])) {
                bestIdx = j;
            }
        }
        
        std::swap(captures.moves[i], captures.moves[bestIdx]);
        Move m = captures.moves[i];

        StateInfo s = board.makeMove(m);

        nodesLookedAt++;
        int score = -qSearch(board, -beta, -alpha);
        board.unmakeMove(m, s);

        if (score >= beta) return beta; // alpha beta prune
        if (score > alpha) alpha = score;
    }

    return alpha;
}

uint64_t perft(int depth, Board& board) {
    if (depth == 0) return 1;

    uint64_t nodes = 0;
    MoveList moves;
    moveGen::generateMoves(board, moves);

    for (const Move& m : moves) {
        StateInfo s = board.makeMove(m);
        nodes += perft(depth - 1, board);
        board.unmakeMove(m, s);
    }

    return nodes;
}

uint64_t search::timedPerft(int depth, Board& board) {
    auto start = std::chrono::steady_clock::now();

    uint64_t nodes = perft(depth, board);

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Time Taken (Engine Move) (ms): " << diff.count() << std::endl;

    std::cout << "Total positions looked at (perft): ";

    return nodes;
}
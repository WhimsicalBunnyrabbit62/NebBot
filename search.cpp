#include "Types.h"
#include "search.h"
#include "eval.h"
#include "moveGen.h"
#include "board.h"
#include <vector>
#include <iostream>
#include <climits>
#include <chrono>

static constexpr int NEG_INF = -30000;
static constexpr int POS_INF = 30000;

static int nodesLookedAt = 0;

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

Move search::findBestMove(int depth, Board& board) {
    auto start = std::chrono::steady_clock::now();
    nodesLookedAt = 0;

    Move bestMove;
    std::vector<Move> allMoves = moveGen::generateMoves(board);

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

    for (Move m : allMoves) {
        int capturedPiece = board.squares[m.to];
        int oldEP = board.enPassantSq;
        bool oldWKS = board.w_kingside;
        bool oldWQS = board.w_queenside;
        bool oldBKS = board.b_kingside;
        bool oldBQS = board.b_queenside;

        board.makeMove(m);
        nodesLookedAt++;
        int eval = -negamax(depth-1, board, NEG_INF, POS_INF);
        board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);

        if (eval >= bestEval) {
            bestEval = eval;
            bestMove = m;
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "time (ms): " << diff.count() << std::endl;
    std::cout << "nodes looked at: " << nodesLookedAt << std::endl;

    return bestMove;
}

int search::negamax(int depth, Board& board, int alpha, int beta) {
    if (depth <= 0) {
        return qSearch(board, alpha, beta);
    }

    int best = NEG_INF;
    std::vector<Move> moves = moveGen::generateMoves(board);

    if (moves.empty()) {
        int kingSq = moveGen::findKing(board, board.turn);
        int enemy = (board.turn == WHITE) ? BLACK : WHITE;

        if (kingSq != -1 && moveGen::isSquareAttacked(kingSq, enemy, board)) {
            return NEG_INF;
        } else  {
            return 0;
        }
    }

    for (Move m : moves) {
        int capturedPiece = board.squares[m.to];
        int oldEP = board.enPassantSq;
        bool oldWKS = board.w_kingside;
        bool oldWQS = board.w_queenside;
        bool oldBKS = board.b_kingside;
        bool oldBQS = board.b_queenside;
        nodesLookedAt++;

        board.makeMove(m);
        int score = -negamax(depth-1, board, -beta, -alpha);
        board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);

        best = std::max(best, score);
        alpha = std::max(alpha, score);

        if (alpha >= beta) break;
    }

    return best;
}

int getMvvLvaScore(Board& board, Move m) {
    int victim = board.squares[m.to];
    int attacker = board.squares[m.from];

    return MvvLvaScores[victim][attacker];
}

int search::qSearch(Board& board, int alpha, int beta) {
    int standPat = eval::evaluate(board) * board.turn; // Eval if player does nothing

    if (standPat >= beta) return beta; // beta -> lowest score opponent will let us reach
    if (standPat >= alpha) alpha = standPat; // alpha -> best we can get;

    std::vector<Move> captures = moveGen::generateCaptures(board);

    //MVV-LVA sort here

    for (int i = 0; i < captures.size(); i++) {
        Move m = captures.at(i);

        int bestIdx = i;
        for (int j = i + 1; j < captures.size(); j++) {
            if (getMvvLvaScore(board, captures.at(j)) > getMvvLvaScore(board, captures.at(bestIdx))) {
                bestIdx = j;
            }
        }
        
        std::swap(captures.at(i), captures.at(bestIdx));

        int capturedPiece = board.squares[m.to];
        int oldEP = board.enPassantSq;
        bool oldWKS = board.w_kingside;
        bool oldWQS = board.w_queenside;
        bool oldBKS = board.b_kingside;
        bool oldBQS = board.b_queenside;

        nodesLookedAt++;

        board.makeMove(m);
        int score = -qSearch(board, -beta, -alpha);
        board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);

        if (score >= beta) return beta; // alpha beta prune
        if (score > alpha) alpha = score;
    }

    return alpha;
}

uint64_t perft(int depth, Board& board) {
    if (depth == 0) return 1;

    uint64_t nodes = 0;
    std::vector<Move> moves = moveGen::generateMoves(board);

    for (const Move& m : moves) {
        int capturedPiece = board.squares[m.to];
        int oldEP = board.enPassantSq;
        bool oldWKS = board.w_kingside;
        bool oldWQS = board.w_queenside;
        bool oldBKS = board.b_kingside;
        bool oldBQS = board.b_queenside;

        board.makeMove(m);
        nodes += perft(depth - 1, board);
        board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);
    }

    return nodes;
}

uint64_t search::timedPerft(int depth, Board& board) {
    auto start = std::chrono::steady_clock::now();

    uint64_t nodes = perft(depth, board);

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "time (ms): " << diff.count() << std::endl;

    return nodes;
}
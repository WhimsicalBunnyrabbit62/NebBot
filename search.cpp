#include "Types.h"
#include "search.h"
#include "eval.h"
#include "moveGen.h"
#include "board.h"
#include <vector>
#include <iostream>
#include <climits>
#include <chrono>

Move search::findBestMove(int depth, Board board) {
    auto start = std::chrono::steady_clock::now();

    Move bestMove;
    std::vector<Move> allMoves = moveGen::generateMoves(board);

    int bestEval = INT_MIN;

    for (Move m : allMoves) {
        int capturedPiece = board.squares[m.to];
        int oldEP = board.enPassantSq;
        bool oldWKS = board.w_kingside;
        bool oldWQS = board.w_queenside;
        bool oldBKS = board.b_kingside;
        bool oldBQS = board.b_queenside;

        board.makeMove(m);
        int eval = -negamax(depth-1, board, INT_MIN, INT_MAX);
        board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);

        if (eval >= bestEval) {
            bestEval = eval;
            bestMove = m;
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "time (ms): " << diff.count() << std::endl;

    return bestMove;
}

int search::negamax(int depth, Board& board, int alpha, int beta) {
    if (depth == 0) {
        return eval::evaluate(board) * board.turn;
    }

    int best = INT_MIN;
    std::vector<Move> moves = moveGen::generateMoves(board);

    for (Move m : moves) {
        int capturedPiece = board.squares[m.to];
        int oldEP = board.enPassantSq;
        bool oldWKS = board.w_kingside;
        bool oldWQS = board.w_queenside;
        bool oldBKS = board.b_kingside;
        bool oldBQS = board.b_queenside;

        board.makeMove(m);
        int score = -negamax(depth-1, board, -beta, -alpha);
        board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);

        best = std::max(best, score);
        alpha = std::max(alpha, score);

        if (alpha > beta) break;
    }

    return alpha;
}

uint64_t search::perft(int depth, Board& board) {
    if (depth == 0) return 1;

    uint64_t nodes = 0;
    std::vector<Move> moves = moveGen::generateMoves(board);

    for (Move m : moves) {
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

#include "Types.h"
#include "search.h"
#include "eval.h"
#include "moveGen.h"
#include "board.h"
#include <vector>
#include <iostream>
#include <climits>

Move search::findBestMove(int depth, Board board) {
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
        int eval = -negamax(depth-1, board);
        //std::cout << "cur eval: " << eval << std::endl;
        board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);

        if (eval >= bestEval) {
            bestEval = eval;
            bestMove = m;
            //std::cout << "best eval: " << bestEval << std::endl;
        }
    }

    return bestMove;
}

int search::negamax(int depth, Board& board) {
    if (depth == 0) {
        return eval::evaluate(board) * board.turn;
    }

    int maxScore = INT_MIN;
    std::vector<Move> moves = moveGen::generateMoves(board);

    for (Move m : moves) {
        int capturedPiece = board.squares[m.to];
        int oldEP = board.enPassantSq;
        bool oldWKS = board.w_kingside;
        bool oldWQS = board.w_queenside;
        bool oldBKS = board.b_kingside;
        bool oldBQS = board.b_queenside;

        board.makeMove(m);
        int score = -negamax(depth-1, board);
        board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);

        if (score > maxScore) maxScore = score;
    }

    return maxScore;
}

int search::explore(int depth, Board board, bool isMaximizing) {
    if (depth == 0) {
        return eval::evaluate(board);
    }

    std::vector<Move> moves = moveGen::generateMoves(board);
    
    if (isMaximizing) {
        int maxEval = INT_MIN;

        for (Move m : moves) {
            int capturedPiece = board.squares[m.to];
            int oldEP = board.enPassantSq;
            bool oldWKS = board.w_kingside;
            bool oldWQS = board.w_queenside;
            bool oldBKS = board.b_kingside;
            bool oldBQS = board.b_queenside;

            board.makeMove(m);
            int eval = explore(depth-1, board, false);
            board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);

            maxEval = std::max(maxEval, eval);
        } 

        return maxEval;
    } else {
        int minEval = INT_MAX;

        for (Move m : moves) {
            int capturedPiece = board.squares[m.to];
            int oldEP = board.enPassantSq;
            bool oldWKS = board.w_kingside;
            bool oldWQS = board.w_queenside;
            bool oldBKS = board.b_kingside;
            bool oldBQS = board.b_queenside;

            board.makeMove(m);
            int eval = explore(depth-1, board, true);
            board.unmakeMove(m, capturedPiece, oldEP, oldWKS, oldWQS, oldBKS, oldBQS);

            minEval = std::min(minEval, eval);
        }

        return minEval;
    }
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

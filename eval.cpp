#include "Types.h"
#include "board.h"
#include "eval.h"

int eval::evaluate(Board board) {
    int score = 0;

    for (int piece : board.squares) {
        if (piece == W_PAWN) score += 100;
        if (piece == B_PAWN) score -= 100;
        if (piece == W_BISHOP) score += 300;
        if (piece == B_BISHOP) score -= 300;
        if (piece == W_KNIGHT) score += 300;
        if (piece == B_KNIGHT) score -= 300;
        if (piece == W_ROOK) score += 500;
        if (piece == B_ROOK) score -= 500;
        if (piece == W_QUEEN) score += 900;
        if (piece == B_QUEEN) score -= 900;
    }

    return score;
}


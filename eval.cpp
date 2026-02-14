#include "Types.h"
#include "board.h"
#include "eval.h"
#include "moveGen.h"

#include <vector>
#include <algorithm>
#include <limits>

static int pieceValue(int piece) {
    switch (piece) {
        case W_PAWN: case B_PAWN: return 100;
        case W_KNIGHT: case B_KNIGHT: return 300;
        case W_BISHOP: case B_BISHOP: return 300;
        case W_ROOK: case B_ROOK: return 500;
        case W_QUEEN: case B_QUEEN: return 900;
        case W_KING: case B_KING: return 20000;
        default: return 0;
    }
}

static bool isWhitePiece(int piece) {
    return piece >= W_PAWN && piece <= W_KING;
}

static bool isBlackPiece(int piece) {
    return piece >= B_PAWN && piece <= B_KING;
}

static bool isColorPiece(int piece, int color) {
    if (piece == EMPTY) return false;
    return (color == WHITE) ? isWhitePiece(piece) : isBlackPiece(piece);
}

static bool pawnAttacks(int from, int to, int piece) {
    int fromFile = from % 8;
    int toFile = to % 8;
    int fileDiff = toFile - fromFile;

    if (piece == W_PAWN) {
        return (to == from - 9 && fileDiff == -1) || (to == from - 7 && fileDiff == 1);
    }
    if (piece == B_PAWN) {
        return (to == from + 7 && fileDiff == -1) || (to == from + 9 && fileDiff == 1);
    }
    return false;
}

static bool knightAttacks(int from, int to) {
    int fromFile = from % 8;
    int fromRank = from / 8;
    int toFile = to % 8;
    int toRank = to / 8;
    int df = std::abs(fromFile - toFile);
    int dr = std::abs(fromRank - toRank);
    return (df == 1 && dr == 2) || (df == 2 && dr == 1);
}

static bool kingAttacks(int from, int to) {
    int fromFile = from % 8;
    int fromRank = from / 8;
    int toFile = to % 8;
    int toRank = to / 8;
    return std::abs(fromFile - toFile) <= 1 && std::abs(fromRank - toRank) <= 1;
}

static bool sliderAttacks(const Board& board, int from, int to, int step) {
    int cur = from;
    while (moveGen::canMoveInDirection(cur, step)) {
        cur += step;
        if (cur < 0 || cur >= 64) return false;
        if (cur == to) return true;
        if (board.squares[cur] != EMPTY) return false;
    }
    return false;
}

static bool pieceAttacksSquare(const Board& board, int from, int to, int piece) {
    if (piece == W_PAWN || piece == B_PAWN) return pawnAttacks(from, to, piece);
    if (piece == W_KNIGHT || piece == B_KNIGHT) return knightAttacks(from, to);
    if (piece == W_KING || piece == B_KING) return kingAttacks(from, to);

    if (piece == W_BISHOP || piece == B_BISHOP || piece == W_QUEEN || piece == B_QUEEN) {
        int diagSteps[] = {-9, -7, 7, 9};
        for (int step : diagSteps) {
            if (sliderAttacks(board, from, to, step)) return true;
        }
    }

    if (piece == W_ROOK || piece == B_ROOK || piece == W_QUEEN || piece == B_QUEEN) {
        int orthoSteps[] = {-8, 8, -1, 1};
        for (int step : orthoSteps) {
            if (sliderAttacks(board, from, to, step)) return true;
        }
    }

    return false;
}

static int findLeastValuableAttacker(const Board& board, int targetSq, int side) {
    int bestSq = -1;
    int bestVal = INT_MAX;

    for (int sq = 0; sq < 64; ++sq) {
        int piece = board.squares[sq];
        if (!isColorPiece(piece, side)) continue;
        if (!pieceAttacksSquare(board, sq, targetSq, piece)) continue;

        int val = pieceValue(piece);
        if (val < bestVal) {
            bestVal = val;
            bestSq = sq;
        }
    }

    return bestSq;
}

int eval::evaluate(Board& board) {
    int score = 0;

    for (int sq = 0; sq < 64; sq++) {
        int piece = board.squares[sq];
        if (piece == EMPTY) continue;
        int value = pieceValue(piece);
        int color = (piece < 7) ? WHITE : BLACK;

        (piece < 7) ? score += value : score -= value;
    }

    return score;
}
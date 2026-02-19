#include "Types.h"
#include "board.h"
#include "eval.h"
#include "moveGen.h"

#include <vector>
#include <algorithm>
#include <limits>

static const int mg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

static const int mg_knight_table[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

static const int mg_bishop_table[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

static const int mg_rook_table[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

static const int mg_queen_table[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};

static const int mg_king_table[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};

static const int* mg_pesto_table[6] =
{
    mg_pawn_table,
    mg_knight_table,
    mg_bishop_table,
    mg_rook_table,
    mg_queen_table,
    mg_king_table
};

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

int getTable(int piece) {
    switch(piece) {
        case W_PAWN: case B_PAWN: return 0;
        case W_KNIGHT: case B_KNIGHT: return 1;
        case W_BISHOP: case B_BISHOP: return 2;
        case W_ROOK: case B_ROOK: return 3;
        case W_QUEEN: case B_QUEEN: return 4;
        case W_KING: case B_KING: return 5;
        default: return 0;
    }
}

int eval::evaluate(Board& board) {
    int score = 0;

    for (int sq = 0; sq < 64; sq++) {
        int piece = board.squares[sq];
        if (piece == EMPTY) continue;
        
        int tableNum = getTable(piece);
        int pstSq = (piece < 7) ? sq : (sq ^ 56);
        int value = pieceValue(piece) + mg_pesto_table[tableNum][pstSq];
        (piece < 7) ? score += value : score -= value;
    }

    return score;
}
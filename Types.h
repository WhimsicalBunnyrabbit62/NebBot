#ifndef TYPES_H
#define TYPES_H

enum Piece {
    EMPTY = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14
};

enum Color {
    WHITE = 0, BLACK = 1
};

struct Move {
    int from;
    int to;
    int score;
};

#endif
#ifndef TYPES_H
#define TYPES_H

enum Piece {
    EMPTY = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14
};

enum Color {
    WHITE = 1, BLACK = -1
};

enum MoveFlag {
    NONE = 0,
    EN_PASSANT = 1,
    CASTLE_KING = 2,
    CASTLE_QUEEN = 3,
    PROMOTION_QUEEN = 4,
    PROMOTION_ROOK = 5,
    PROMOTION_BISHOP = 6,
    PROMOTION_KNIGHT = 7,
    DOUBLE_PAWN_PUSH = 8
};

struct Move {
    int from;
    int to;
    int flags = 0;
    int score;
};

inline bool isEnemy(int myPiece, int targetPiece) {
    if (targetPiece == EMPTY) return false;
    bool myColor = (myPiece < 7);
    bool targetColor = (targetPiece < 7);

    return myColor != targetColor;
}

#endif
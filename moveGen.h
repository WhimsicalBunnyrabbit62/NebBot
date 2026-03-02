#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "Types.h"
#include <vector>

const int WP = 0;
const int WN = 1;
const int WB = 2;
const int WR = 3;
const int WQ = 4;
const int WK = 5;
const int BP = 6;
const int BN = 7;
const int BB = 8;
const int BR = 9;
const int BQ = 10;
const int BK = 11;

struct MoveList {
    Move moves[256]; 
    int count = 0;

    void clear() { count = 0; }

    void push_back(Move m) {
        if (count < 256) moves[count++] = m;
    }

    Move* begin() { return moves; }
    Move* end() { return moves + count; }

    int size() const { return count; }
    bool empty() const { return count == 0; }
};

class moveGen {

public:
    static void generateMoves(Board& board, MoveList& moves);
    static void generateCaptures(Board& board, MoveList& allMoves, MoveList& captures);

    static bool isSquareAttacked(Board& board, int sq);
    static void initAll();
private:
    static std::string toAlgebraic(int index);

    static void genKingMoves(int sq, Board& board, MoveList& moves);
    static void genSlidingMoves(Board& board, MoveList& moves);
    static void genKnightMoves(Board& board, MoveList& moves);
};

#endif
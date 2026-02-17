#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "Types.h"
#include <vector>


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

    static bool canMoveInDirection(int sq, int offset);
    static bool isSquareAttacked(int sq, int attackerColor, Board& board);
    static int findKing(Board& board, int color);

private:
    static void genPawnMoves(int sq, Board& board, MoveList& moves);
    static void genKnightMoves(int sq, Board& board, MoveList& moves);
    static void genSlidingMoves(int sq, Board& board, MoveList& moves, const std::vector<int>& offsets);
    static void genKingMoves(int sq, Board& board, MoveList& moves);
    static std::string toAlgebraic(int index);

    static void addPromotionMoves(int from, int to, MoveList& moves);
    static bool attackedBySlider(int targetSq, int attackerColor, Board& board, const std::vector<int>& offsets, bool isRook);
    static bool isSafeJump(int startSq, int targetSq);
    static bool isSafeJumpKing(int startSq, int targetSq);
    static void genCastlingMoves(int sq, int color, Board& board, MoveList& moves);
};

#endif
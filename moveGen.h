#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "Types.h"
#include <vector>


class moveGen {
public:
    static std::vector<Move> generateMoves(Board& board);
    static bool canMoveInDirection(int sq, int offset);
    static bool isSquareAttacked(int sq, int attackerColor, Board& board);

private:
    static void genPawnMoves(int sq, Board& board, std::vector<Move>& moves);
    static void genKnightMoves(int sq, Board& board, std::vector<Move>& moves);
    static void genSlidingMoves(int sq, Board& board, std::vector<Move>& moves, const std::vector<int>& offsets);
    static void genKingMoves(int sq, Board& board, std::vector<Move>& moves);
    static std::string toAlgebraic(int index);

    static int findKing(Board& board, int color);
    static void addPromotionMoves(int from, int to, std::vector<Move>& moves);
    static bool attackedBySlider(int targetSq, int attackerColor, Board& board, const std::vector<int>& offsets, bool isRook);
    static bool isSafeJump(int startSq, int targetSq);
    static bool isSafeJumpKing(int startSq, int targetSq);
    static void genCastlingMoves(int sq, int color, Board& board, std::vector<Move>& moves);
};

#endif
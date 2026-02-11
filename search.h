#ifndef SEARCH_H
#define SEARCH_H

#include "Types.h"
#include "board.h"

class search {
public:
    static int negamax(int depth, Board& board, int alpha, int beta);
    static Move findBestMove(int depth, Board board);
    static uint64_t perft(int depth, Board& board);
};

#endif
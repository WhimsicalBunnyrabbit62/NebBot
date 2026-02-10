#ifndef SEARCH_H
#define SEARCH_H

#include "Types.h"
#include "board.h"

class search {
public:
    static int explore(int depth, Board board, bool isMaximizing);
    static Move findBestMove(int depth, Board board);
    static uint64_t perft(int depth, Board& board);
};

#endif
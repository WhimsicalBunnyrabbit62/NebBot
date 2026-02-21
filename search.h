#ifndef SEARCH_H
#define SEARCH_H

#include "Types.h"
#include "board.h"
#include "moveGen.h"
#include <chrono>

class search {
public:
    static int negamax(int depth, Board& board, int alpha, int beta, std::chrono::steady_clock::time_point startTime, int limit);
    static int qSearch(Board& board, int alpha, int beta);
    static Move startSearch(Board& board, int maxTimeMs = 3000);
    static uint64_t timedPerft(int depth, Board& board);
};

#endif

#ifndef SEARCH_H
#define SEARCH_H

#include "Types.h"
#include "board.h"
#include "moveGen.h"
#include <chrono>

enum BoundType { EXACT, LOWER_BOUND, UPPER_BOUND };

struct TableEntry {
    uint64_t hash;
    int eval;
    Move best;
    int depth;
    BoundType boundType;
};

class search {
public:
    static int negamax(int depth, Board& board, int alpha, int beta, std::chrono::steady_clock::time_point startTime, int limit, bool allowNullMove);
    static int qSearch(Board& board, int alpha, int beta);
    static Move startSearch(Board& board, int maxTimeMs = 3000);
    static uint64_t timedPerft(int depth, Board& board);
    static void initAll();
};

#endif

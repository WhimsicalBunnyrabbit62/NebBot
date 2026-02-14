#ifndef EVAL_H
#define EVAL_H

#include "Types.h"
#include "board.h"
#include <vector>

struct Attacker {
    int sq;
    int piece;
    int value;
};

class eval {
public: 
    static int evaluate(Board& board);
    static int exchangeEvaluation(Board& board, int from);
};

#endif
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
    static void initAll();
    static int evaluate(Board& board);

    const static bool useNNUE = true;

private:
    static void initPassedMasks();
};


#endif
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
    static void initNNUE();

    const static bool useNNUE = false;

    static float sharedlayerWeights[256][768];
    static float sharedlayerBiases[256];

    static float layerOneWeights[512][512];
    static float layerOneBiases[512];

    static float layerTwoWeights[256][512];
    static float layerTwoBiases[256];

    static float layerThreeWeights[128][256];
    static float layerThreeBiases[128];

    static float layerFourWeights[64][128];
    static float layerFourBiases[64];

    static float layerFiveWeights[64];
    static float layerFiveBias[1];

private:
    static void initPassedMasks();
};


#endif
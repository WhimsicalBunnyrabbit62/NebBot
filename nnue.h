#ifndef NNUE_H
#define NNUE_H

#include "Types.h"
#include "board.h"

class nnue {
public:
    struct Accumulator {
        alignas(32) float white[256];
        alignas(32) float black[256];
    };

    static void initNNUE();
    static void buildFeatureTable();
    static int evaluate(Board& board);

    static Accumulator accStack[256];
    static int accTop;

    static void accAdd(int idx, int sq);
    static void accRemove(int idx, int sq);
    static void refreshAccumulator(Board& board);

    static float featureWeights[768][256];

    static float sharedlayerWeights[256][768];
    static float sharedlayerBiases[256];

    static float layerOneWeights[32][512];
    static float layerOneBiases[32];

    static float layerTwoWeights[32][32];
    static float layerTwoBiases[32];

    static float layerThreeWeights[1][32];
    static float layerThreeBiases[1];
};

#endif

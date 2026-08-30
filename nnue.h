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
};

#endif

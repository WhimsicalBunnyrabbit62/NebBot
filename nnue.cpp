#include "nnue.h"
#include "Types.h"
#include "board.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

float nnue::featureWeights[768][256] = {};

float nnue::sharedlayerWeights[256][768] = {};
float nnue::sharedlayerBiases[256] = {};
float nnue::layerOneWeights[32][512] = {};
float nnue::layerOneBiases[32] = {};
float nnue::layerTwoWeights[32][32] = {};
float nnue::layerTwoBiases[32] = {};
float nnue::layerThreeWeights[1][32] = {};
float nnue::layerThreeBiases[1] = {};

nnue::Accumulator nnue::accStack[256] = {};
int nnue::accTop = 0;

void nnue::initNNUE() {
    accTop = 0;

static auto loadBinary = [](const char* path, void* dst, size_t bytes) -> bool {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            std::cerr << "Failed to open file: " << path << std::endl;
            return false;
        }

        input.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(bytes));
        if (!input) {
            std::cerr << "Error reading data from file: " << path << std::endl;
            return false;
        }

        const std::streamsize readFloats = input.gcount() / static_cast<std::streamsize>(sizeof(float));
        std::cout << "Successfully read " << readFloats << " floats from " << path << std::endl;
        return true;
    };

    if (!loadBinary("raw_layers/sharedLayer_weight.bin", sharedlayerWeights, sizeof(sharedlayerWeights))) return;
    if (!loadBinary("raw_layers/sharedLayer_bias.bin", sharedlayerBiases, sizeof(sharedlayerBiases))) return;
    if (!loadBinary("raw_layers/layerOne_weight.bin", layerOneWeights, sizeof(layerOneWeights))) return;
    if (!loadBinary("raw_layers/layerOne_bias.bin", layerOneBiases, sizeof(layerOneBiases))) return;
    if (!loadBinary("raw_layers/layerTwo_weight.bin", layerTwoWeights, sizeof(layerTwoWeights))) return;
    if (!loadBinary("raw_layers/layerTwo_bias.bin", layerTwoBiases, sizeof(layerTwoBiases))) return;
    if (!loadBinary("raw_layers/layerThree_weight.bin", layerThreeWeights, sizeof(layerThreeWeights))) return;
    if (!loadBinary("raw_layers/layerThree_bias.bin", layerThreeBiases, sizeof(layerThreeBiases))) return;
}

void nnue::buildFeatureTable() {
    for (size_t i{0}; i < 256; ++i) {
        for (size_t f{0}; f < 768; ++f) {
            featureWeights[f][i] = sharedlayerWeights[i][f];
        }
    }
}

inline int pop_lsb(uint64_t& bb) {
    int sq = __builtin_ctzll(bb);
    bb &= bb - 1;
    return sq;
}

void nnue::accAdd(int idx, int sq) {
    int wf = idx * 64 + (sq^56);
    int bp = (idx >= 6) ? idx - 6 : idx + 6;
    int bf = bp * 64 + sq;

    Accumulator& a = accStack[accTop];

    for (int i = 0; i < 256; ++i) a.white[i] += featureWeights[wf][i];
    for (int i = 0; i < 256; ++i) a.black[i] += featureWeights[bf][i];
}

void nnue::accRemove(int idx, int sq) {
    int wf = idx * 64 + (sq^56);
    int bp = (idx >= 6) ? idx - 6 : idx + 6;
    int bf = bp * 64 + sq;

    Accumulator& a = accStack[accTop];

    for (int i = 0; i < 256; ++i) a.white[i] -= featureWeights[wf][i];
    for (int i = 0; i < 256; ++i) a.black[i] -= featureWeights[bf][i];
}

void nnue::refreshAccumulator(Board& board) {
    accTop = 0;
    Accumulator& a = accStack[0];

    for (int i = 0; i < 256; ++i) { a.white[i] = sharedlayerBiases[i]; a.black[i] = sharedlayerBiases[i]; }

    for (int p = 0; p < 12; ++p) {
        uint64_t bb = board.pieces[p];
        while (bb) {int sq = pop_lsb(bb); accAdd(p, sq); }
    }
}

int nnue::evaluate(Board& board) {
    float whiteInput[768] = {};
    float blackInput[768] = {};

    for (int p = 0; p < 12; ++p) {
        uint64_t bb = board.pieces[p];

        while (bb) {
            int sq = pop_lsb(bb);

            int black_piece = (p >= 6) ? (p - 6) : (p + 6);

            whiteInput[p * 64 + (sq ^ 56)] = 1.0f;
            blackInput[black_piece * 64 + sq] = 1.0f;
        }
    }


    // std::cout << "WHITEIN";
    // for (float f : whiteInput) std::cout << f << " ";
    // std::cout << std::endl;

    // std::cout << "WEIGHTS";
    // for (float f : sharedlayerBiases) std::cout << f << " ";
    // std::cout << std::endl;

    const Accumulator& acc = accStack[accTop];
    bool whiteToMove = (board.turn == WHITE);
    const float* stm = whiteToMove ? acc.white : acc.black;
    const float* opp = whiteToMove ? acc.black : acc.white;

    float output[512];
    for (size_t i{0}; i < 256; ++i) output[i] = std::max(stm[i], 0.0f);
    for (size_t i{0}; i < 256; ++i) output[i+256] = std::max(opp[i], 0.0f);

    float outputOne[32];
    for (size_t i{0}; i < 32; ++i) {
        float sum = layerOneBiases[i];

        for (size_t j{0}; j < 512; ++j) {
            sum += layerOneWeights[i][j] * output[j];
        }

        outputOne[i] = std::max(sum, 0.0f);
    }

    float outputTwo[32];
    for (size_t i{0}; i < 32; ++i) {
        float sum = layerTwoBiases[i];

        for (size_t j{0}; j < 32; ++j) {
            sum += layerTwoWeights[i][j] * outputOne[j];
        }

        outputTwo[i] = std::max(sum, 0.0f);
    }

    float outputThree[1];
    for (size_t i{0}; i < 1; ++i) {
        float sum = layerThreeBiases[i];

        for (size_t j{0}; j < 32; ++j) {
            sum += layerThreeWeights[i][j] * outputTwo[j];
        }

        outputThree[i] = sum;
    }

    float answer = outputThree[0];
    answer = std::tanh(answer);

    // * 500 COULD BE A PROBLEM
    return answer * 200 * board.turn;
}

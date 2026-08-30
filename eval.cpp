#include "Types.h"
#include "board.h"
#include "eval.h"
#include "moveGen.h"

#include <vector>
#include <algorithm>
#include <limits>
#include <fstream>
#include <iostream>

static const int mg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

static const int eg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0,
};

static const int mg_knight_table[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

static const int eg_knight_table[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64,
};

static const int mg_bishop_table[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

static const int eg_bishop_table[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17,
};

static const int mg_rook_table[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

static const int eg_rook_table[64] = {
    13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20,
};

static const int mg_queen_table[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};

static const int eg_queen_table[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41,
};

static const int mg_king_table[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};

static const int eg_king_table[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43
};

static const int* mg_pesto_table[6] =
{
    mg_pawn_table,
    mg_knight_table,
    mg_bishop_table,
    mg_rook_table,
    mg_queen_table,
    mg_king_table
};

static const int* eg_pesto_table[6] =
{
    eg_pawn_table,
    eg_knight_table,
    eg_bishop_table,
    eg_rook_table,
    eg_queen_table,
    eg_king_table
};

static const int materialValues[6] = { 100, 300, 300, 500, 900, 20000 };
static const int phaseValues[6]    = { 0, 1, 1, 2, 4, 0 };

static const uint64_t neighborFilesMask[8] = {
    0x0202020202020202ULL, // File A neighbors: File B
    0x0505050505050505ULL, // File B neighbors: File A + C
    0x0A0A0A0A0A0A0A0AULL, // File C neighbors: File B + D
    0x1414141414141414ULL, // File D neighbors: File C + E
    0x2828282828282828ULL, // File E neighbors: File D + F
    0x5050505050505050ULL, // File F neighbors: File E + G
    0xA0A0A0A0A0A0A0A0ULL, // File G neighbors: File F + H
    0x4040404040404040ULL  // File H neighbors: File G
};

static const int passed_mg[8] = { 0, 10, 20, 40, 80, 150, 250, 0 };
static const int passed_eg[8] = { 0, 20, 40, 80, 150, 300, 500, 0 };

uint64_t passedMaskWhite[64];
uint64_t passedMaskBlack[64];

float eval::sharedlayerWeights[256][768] = {};
float eval::sharedlayerBiases[256] = {};
float eval::layerOneWeights[512][512] = {};
float eval::layerOneBiases[512] = {};
float eval::layerTwoWeights[256][512] = {};
float eval::layerTwoBiases[256] = {};
float eval::layerThreeWeights[128][256] = {};
float eval::layerThreeBiases[128] = {};
float eval::layerFourWeights[64][128] = {};
float eval::layerFourBiases[64] = {};
float eval::layerFiveWeights[64] = {};
float eval::layerFiveBias[1] = {};

void eval::initAll() {
    initPassedMasks();
}

void eval::initPassedMasks() {
    for (int sq = 0; sq < 64; sq++) {
        uint64_t whiteMask = 0;
        uint64_t blackMask = 0;

        int file = sq % 8;
        int rank = sq / 8;

        for (int r = 0; r < 8; r++) {
            for (int f = file - 1; f <= file + 1; f++) {
                if (f < 0 || f > 7) continue;

                uint64_t bit = (1ULL << (r * 8 + f));

                if (r < rank) whiteMask |= bit;
                if (r > rank) blackMask |= bit;
            }
        }

        passedMaskBlack[sq] = blackMask;
        passedMaskWhite[sq] = whiteMask;
    }
}

void eval::initNNUE() {
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
    if (!loadBinary("raw_layers/layerFour_weight.bin", layerFourWeights, sizeof(layerFourWeights))) return;
    if (!loadBinary("raw_layers/layerFour_bias.bin", layerFourBiases, sizeof(layerFourBiases))) return;
    if (!loadBinary("raw_layers/layerFive_weight.bin", layerFiveWeights, sizeof(layerFiveWeights))) return;
    if (!loadBinary("raw_layers/layerFive_bias.bin", layerFiveBias, sizeof(layerFiveBias))) return;
}

inline int pop_lsb(uint64_t& bb) {
    int sq = __builtin_ctzll(bb); 
    bb &= bb - 1;               
    return sq;
}

int eval::evaluate(Board& board) {
    if (!useNNUE) {
        int score = 0;
        int mgPhase = 0;
        int mgScore = 0;
        int egScore = 0;

        for (int i = 0; i < 6; i++) {
            uint64_t wPieces = board.pieces[i]; 
            int wCount = __builtin_popcountll(wPieces);

            if (wPieces) {
                score += (materialValues[i] * wCount);
                mgPhase += (wCount * phaseValues[i]);

                while (wPieces) {
                    int sq = moveGen::get_lsb(wPieces);
                    
                    mgScore += mg_pesto_table[i][sq];
                    egScore += eg_pesto_table[i][sq];
                    moveGen::pop_bit(wPieces);
                }
            }


            uint64_t bPieces = board.pieces[i + 6]; 
            int bCount = __builtin_popcountll(bPieces);

            if (bPieces) {
                score -= (materialValues[i] * bCount);
                mgPhase += (bCount * phaseValues[i]);

                while (bPieces) {
                    int sq = moveGen::get_lsb(bPieces);

                    mgScore -= mg_pesto_table[i][sq ^ 56];
                    egScore -= eg_pesto_table[i][sq ^ 56];
                    moveGen::pop_bit(bPieces);
                }
            }
        }

        // random stuff
        if (__builtin_popcountll(board.pieces[WB]) >= 2) score += 30;
        if (__builtin_popcountll(board.pieces[BB]) >= 2) score -= 30;

        const uint64_t allWhitePawns = board.pieces[WP];
        const uint64_t allBlackPawns = board.pieces[BP];

        uint64_t whitePawns = allWhitePawns;
        while (whitePawns) {
            int sq = moveGen::get_lsb(whitePawns);

            int file = sq % 8;
            int rank = sq / 8;

            if (!(neighborFilesMask[file] & allWhitePawns)) {
                mgScore -= 20;
                egScore -= 25;
            }
            if (!(passedMaskWhite[sq] & allBlackPawns)) {
                if (1ULL << (sq + 8) & board.allOcc) {
                    mgScore += passed_mg[rank] * 0.5;
                    egScore += passed_eg[rank] * 0.5;
                } else {
                    mgScore += passed_mg[rank];
                    egScore += passed_eg[rank];
                }
            } 

            moveGen::pop_bit(whitePawns);
        }
        
        uint64_t blackPawns = allBlackPawns;
        while (blackPawns) {
            int sq = moveGen::get_lsb(blackPawns);

            int file = sq % 8;
            int rank = sq / 8;

            if (!(neighborFilesMask[file] & allBlackPawns)) {
                mgScore += 20;
                egScore += 25;
            }

            if (!(passedMaskBlack[sq] & allWhitePawns)) {
                if (1ULL << (sq - 8) & board.allOcc) {
                    mgScore -= passed_mg[rank] * 0.5;
                    egScore -= passed_eg[rank] * 0.5;
                } else {
                    mgScore -= passed_mg[rank];
                    egScore -= passed_eg[rank];
                }
            }

            moveGen::pop_bit(blackPawns);
        }

        int phase = (mgPhase > 24) ? 24 : mgPhase;
        score += ((mgScore * phase) + (egScore * (24 - phase))) / 24;

        return score;
    } else {
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

        bool whiteFirst = board.turn == WHITE;
        float output[512];
        for (size_t i{0}; i < 256; ++i) {
            float sum = sharedlayerBiases[i];

            for (size_t j{0}; j < 768; ++j) {
                sum += sharedlayerWeights[i][j] * ((whiteFirst) ? whiteInput[j] : blackInput[j]);
            }

            output[i] = std::max(sum, 0.0f);
        }

        for (size_t i{0}; i < 256; ++i) {
            float sum = sharedlayerBiases[i];

            for (size_t j{0}; j < 768; ++j) {
                sum += sharedlayerWeights[i][j] * ((!whiteFirst) ? whiteInput[j] : blackInput[j]);
            }

            output[256+i] = std::max(sum, 0.0f);
        }

        float outputOne[512];
        for (size_t i{0}; i < 512; ++i) {
            float sum = layerOneBiases[i];

            for (size_t j{0}; j < 512; ++j) {
                sum += layerOneWeights[i][j] * output[j];
            }

            outputOne[i] = std::max(sum, 0.0f);
        }

        float outputTwo[256];
        for (size_t i{0}; i < 256; ++i) {
            float sum = layerTwoBiases[i];

            for (size_t j{0}; j < 512; ++j) {
                sum += layerTwoWeights[i][j] * outputOne[j];
            }

            outputTwo[i] = std::max(sum, 0.0f);
        }

        float outputThree[128];
        for (size_t i{0}; i < 128; ++i) {
            float sum = layerThreeBiases[i];

            for (size_t j{0}; j < 256; ++j) {
                sum += layerThreeWeights[i][j] * outputTwo[j];
            }

            outputThree[i] = std::max(sum, 0.0f);
        }

        float outputFour[64];
        for (size_t i{0}; i < 64; ++i) {
            float sum = layerFourBiases[i];

            for (size_t j{0}; j < 128; ++j) {
                sum += layerFourWeights[i][j] * outputThree[j];
            }

            outputFour[i] = std::max(sum, 0.0f);
        }

        float outputFive[1];
        outputFive[0] = layerFiveBias[0];

        for (size_t i{0}; i < 64; ++i) {
            outputFive[0] += layerFiveWeights[i] * outputFour[i];
        }

        float answer = outputFive[0];
        answer = std::tanh(answer);
        
        // * 500 COULD BE A PROBLEM
        return answer * 500 * board.turn;
    }
}

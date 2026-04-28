#include "moveGen.h"
#include "board.h"
#include <iostream>
#include <cstdint>

const uint64_t FILE_A = 0x0101010101010101;
const uint64_t FILE_H = 0x8080808080808080;
const uint64_t RANK_1 = 0xFF00000000000000ULL;
const uint64_t RANK_2 = 0x00FF000000000000ULL;
const uint64_t RANK_7 = 0x000000000000FF00ULL;
const uint64_t RANK_8 = 0x00000000000000FFULL;


// pradu kannan's magic numbers
const uint64_t RookMagics[64] = {
    0x0080001020400080ULL, 0x0040001000200040ULL, 0x0080081000200080ULL, 0x0080040800100080ULL,
    0x0080020400080080ULL, 0x0080010200040080ULL, 0x0080008001000200ULL, 0x0080002040800100ULL,
    0x0000800020400080ULL, 0x0000400020005000ULL, 0x0000801000200080ULL, 0x0000800800100080ULL,
    0x0000800400080080ULL, 0x0000800200040080ULL, 0x0000800100020080ULL, 0x0000800040800100ULL,
    0x0000208000400080ULL, 0x0000404000201000ULL, 0x0000808010002000ULL, 0x0000808008001000ULL,
    0x0000808004000800ULL, 0x0000808002000400ULL, 0x0000010100020004ULL, 0x0000020000408104ULL,
    0x0000208080004000ULL, 0x0000200040005000ULL, 0x0000100080200080ULL, 0x0000080080100080ULL,
    0x0000040080080080ULL, 0x0000020080040080ULL, 0x0000010080800200ULL, 0x0000800080004100ULL,
    0x0000204000800080ULL, 0x0000200040401000ULL, 0x0000100080802000ULL, 0x0000080080801000ULL,
    0x0000040080800800ULL, 0x0000020080800400ULL, 0x0000020001010004ULL, 0x0000800040800100ULL,
    0x0000204000808000ULL, 0x0000200040008080ULL, 0x0000100020008080ULL, 0x0000080010008080ULL,
    0x0000040008008080ULL, 0x0000020004008080ULL, 0x0000010002008080ULL, 0x0000004081020004ULL,
    0x0000204000800080ULL, 0x0000200040008080ULL, 0x0000100020008080ULL, 0x0000080010008080ULL,
    0x0000040008008080ULL, 0x0000020004008080ULL, 0x0000800100020080ULL, 0x0000800041000080ULL,
    0x00FFFCDDFCED714AULL, 0x007FFCDDFCED714AULL, 0x003FFFCDFFD88096ULL, 0x0000040810002101ULL,
    0x0001000204080011ULL, 0x0001000204000801ULL, 0x0001000082000401ULL, 0x0001FFFAABFAD1A2ULL
};

const uint64_t BishopMagics[64] = {
    0x0002020202020200ULL, 0x0002020202020000ULL, 0x0004010202000000ULL, 0x0004040080000000ULL,
    0x0001104000000000ULL, 0x0000821040000000ULL, 0x0000410410400000ULL, 0x0000104104104000ULL,
    0x0000040404040400ULL, 0x0000020202020200ULL, 0x0000040102020000ULL, 0x0000040400800000ULL,
    0x0000011040000000ULL, 0x0000008210400000ULL, 0x0000004104104000ULL, 0x0000002082082000ULL,
    0x0004000808080800ULL, 0x0002000404040400ULL, 0x0001000202020200ULL, 0x0000800802004000ULL,
    0x0000800400A00000ULL, 0x0000200100884000ULL, 0x0000400082082000ULL, 0x0000200041041000ULL,
    0x0002080010101000ULL, 0x0001040008080800ULL, 0x0000208004010400ULL, 0x0000404004010200ULL,
    0x0000840000802000ULL, 0x0000404002011000ULL, 0x0000808001041000ULL, 0x0000404000820800ULL,
    0x0001041000202000ULL, 0x0000820800101000ULL, 0x0000104400080800ULL, 0x0000020080080080ULL,
    0x0000404040040100ULL, 0x0000808100020100ULL, 0x0001010100020800ULL, 0x0000808080010400ULL,
    0x0000820820004000ULL, 0x0000410410002000ULL, 0x0000082088001000ULL, 0x0000002011000800ULL,
    0x0000080100400400ULL, 0x0001010101000200ULL, 0x0002020202000400ULL, 0x0001010101000200ULL,
    0x0000410410400000ULL, 0x0000208208200000ULL, 0x0000002084100000ULL, 0x0000000020880000ULL,
    0x0000001002020000ULL, 0x0000040408020000ULL, 0x0004040404040000ULL, 0x0002020202020000ULL,
    0x0000104104104000ULL, 0x0000002082082000ULL, 0x0000000020841000ULL, 0x0000000000208800ULL,
    0x0000000010020200ULL, 0x0000000404080200ULL, 0x0000040404040400ULL, 0x0002020202020200ULL
};

static uint64_t knightMasks[64];
static uint64_t kingMasks[64];
static uint64_t pawnAttacks[2][64];
static uint64_t rookMasks[64];
static uint64_t bishopMasks[64];

static uint64_t rookTable[64][4096];
static uint64_t bishopTable[64][512];

int moveGen::get_lsb(uint64_t bb) {
    return __builtin_ctzll(bb);
}

void moveGen::pop_bit(uint64_t& bb) {
    bb &= (bb - 1);
} 

void initRookMask() {
    for (int sq = 0; sq < 64; sq++) {
        uint64_t mask = 0;
        int r = sq / 8;
        int f = sq % 8;

        for (int i = r-1; i >= 1; i--) mask |= (1ULL << (i * 8 + f));
        for (int i = r+1; i <= 6; i++) mask |= (1ULL << (i * 8 + f));
        for (int i = f-1; i >= 1; i--) mask |= (1ULL << (r * 8 + i));
        for (int i = f+1; i <= 6; i++) mask |= (1ULL << (r * 8 + i));

        rookMasks[sq] = mask;
    } 
}

void initBishopMask() {
    int dr[] = {-1, 1, 1, -1};
    int df[] = {1, 1, -1, -1};

    for (int sq = 0; sq < 64; sq++) {
        uint64_t mask = 0;
        int r = sq / 8;
        int f = sq % 8;

        for (int i = 0; i < 4; i++) {
            int nr = r + dr[i];
            int nf = f + df[i];
            while (nr >= 1 && nr <= 6 && nf >= 1 && nf <= 6) {
                mask |= (1ULL << (nr * 8 + nf));
                nr += dr[i];
                nf += df[i];
            }
        }

        bishopMasks[sq] = mask;
    }
}

uint64_t moveGen::set_occupancy(int index, uint64_t mask) {
    uint64_t occupancy = 0ULL;
    int bitsInMask = __builtin_popcountll(mask);

    for (int i = 0; i < bitsInMask; i++) {
        int square = get_lsb(mask);
        pop_bit(mask);

        if (index & (1 << i)) {
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}

uint64_t slowBishopAttackGen(int sq, uint64_t blockers) {
    uint64_t attacks = 0ULL;
    int r = sq / 8;
    int f = sq % 8;

    int dr[] = {-1, 1, 1, -1};
    int df[] = {1, 1, -1, -1};

    for (int i = 0; i < 4; i++) {
        for (int d = 1; d < 8; d++) {
            int nr = r + dr[i] * d;
            int nf = f + df[i] * d;

            if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) break;

            uint64_t bit = (1ULL << (nr * 8 + nf));

            attacks |= bit;
            if (blockers & bit) break; // blocked
        }
    }
    
    return attacks;
}

uint64_t slowRookAttackGen(int sq, uint64_t blockers) {
    uint64_t attacks = 0ULL;
    int r = sq / 8;
    int f = sq % 8;

    int dr[] = {-1, 1, 0, 0};
    int df[] = {0, 0, 1, -1};

    for (int i = 0; i < 4; i++) {
        for (int d = 1; d < 8; d++) {
            int nr = r + dr[i] * d;
            int nf = f + df[i] * d;

            if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) break;

            uint64_t bit = (1ULL << (nr * 8 + nf));

            attacks |= bit;
            if (blockers & bit) break; // blocked
        }
    }

    return attacks;
}

void moveGen::initBishopTable() {
    for (int sq = 0; sq < 64; sq++) {
        uint64_t mask = bishopMasks[sq];
        int bitCount = __builtin_popcountll(mask);
        int totalPermutations = (1 << bitCount);

        for (int i = 0; i < totalPermutations; i++) {
            uint64_t occupancy = set_occupancy(i, mask);

            int magicIdx = (occupancy * BishopMagics[sq]) >> (64 - bitCount);
            bishopTable[sq][magicIdx] = slowBishopAttackGen(sq, occupancy);
        }
    }
}

void moveGen::initRookTable() {
    for (int sq = 0; sq < 64; sq++) {
        uint64_t mask = rookMasks[sq];
        int bitCount = __builtin_popcountll(mask);
        // some cool binary math here to get powers of 2 based on the number of relevant bits (1s)
        int totalPermutations = (1 << bitCount);

        for (int i = 0; i < totalPermutations; i++) {
            uint64_t occupancy = set_occupancy(i, mask);
            
            // magic math equation that gives possible attack pattern for every occupancy for every square
            // how does it work??? who knows...
            int magicIdx = (occupancy * RookMagics[sq]) >> (64 - bitCount);

            rookTable[sq][magicIdx] = slowRookAttackGen(sq, occupancy);
        }
    }
}

void initKnightMask() {
    int dr[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    int df[] = {-1, 1, -2, 2, -2, 2, -1, 1};

    for (int sq = 0; sq < 64; sq++) {
        uint64_t mask = 0;
        int rank = sq / 8;
        int file = sq % 8;

        for (int i = 0; i < 8; i++) {
            int r = rank + dr[i];
            int f = file + df[i];

            if (r >= 0 && r < 8 && f >= 0 && f < 8) {
                mask |= 1ULL << (r * 8 + f);
            }
        }

        knightMasks[sq] = mask;
    }
}

inline void initPawnAttacks() {
    for (int sq = 0; sq < 64; sq++) {
        uint64_t bit = (1ULL << sq);

        uint64_t wAttacks = 0;
        if (!(bit & FILE_A)) wAttacks |= (bit >> 9);
        if (!(bit & FILE_H)) wAttacks |= (bit >> 7);
        pawnAttacks[0][sq] = wAttacks;

        uint64_t bAttacks = 0;
        if (!(bit & FILE_A)) bAttacks |= (bit << 7);
        if (!(bit & FILE_H)) bAttacks |= (bit << 9);
        pawnAttacks[1][sq] = bAttacks;
    }
}

void initKingMask() {
    for (int sq = 0; sq < 64; sq++) {
        uint64_t mask = 0;
        uint64_t bit = (1ULL << sq); // king location

        if (!(bit & FILE_A)) {
            mask |= (bit >> 1);
            mask |= (bit >> 9);
            mask |= (bit << 7);
        } 
        if (!(bit & FILE_H)) {
            mask |= (bit << 1);
            mask |= (bit << 9);
            mask |= (bit >> 7);
        }

        mask |= (bit << 8);
        mask |= (bit >> 8); // can move up down w/o checks cuz of binary math

        kingMasks[sq] = mask;
    }
}

void moveGen::initAll() {
    initPawnAttacks();
    initKnightMask();
    initBishopMask();
    initBishopTable();
    initRookMask();
    initRookTable();
    initKingMask();
}

void moveGen::generateMoves(Board& board, MoveList& legalMoves) {
    legalMoves.clear();
    MoveList moves;

    const int sideToMove = board.turn;

    genPawnMoves(board, moves);
    genKnightMoves(board, moves);
    genSlidingMoves(board, moves);
    uint64_t kingBoard = (board.turn == WHITE) ? board.pieces[WK] : board.pieces[BK];
    if (kingBoard) {
        genKingMoves(get_lsb(kingBoard), board, moves);
    }

    for (Move m : moves) {
        StateInfo s = board.makeMove(m);

        uint64_t ownKingBoard = (sideToMove == WHITE) ? board.pieces[WK] : board.pieces[BK];
        int ownKingSq = ownKingBoard ? get_lsb(ownKingBoard) : -1;

        int restoreTurn = board.turn;
        board.turn = sideToMove;
        bool kingInCheck = (ownKingSq != -1) && isSquareAttacked(board, ownKingSq);
        board.turn = restoreTurn;

        if (!kingInCheck) legalMoves.push_back(m);

        board.unmakeMove(m, s);
    }
}

void moveGen::generateCaptures(Board& board, MoveList& allMoves, MoveList& captures) {
    captures.clear();
    
    for (Move m : allMoves) {
        if (board.squares[m.to] != EMPTY || m.flags == EN_PASSANT) {
            captures.push_back(m);
        }
    }
}

std::string moveGen::toAlgebraic(int index) {
    int file = index % 8;
    int rank = 8 - (index / 8);
    char fileChar = (char) ('a' + file);

    return std::string(1, fileChar) + std::to_string(rank);
}

void moveGen::genKnightMoves(Board& board, MoveList& moves) {
    uint64_t knights = (board.turn == WHITE) ? board.pieces[WN] : board.pieces[BN];
    uint64_t myOcc = (board.turn == WHITE) ? board.whiteOcc : board.blackOcc;

    while (knights) {
        int from = get_lsb(knights);

        uint64_t possibleMoves = knightMasks[from] & ~myOcc;

        while (possibleMoves) {
            int to = get_lsb(possibleMoves);
            
            moves.push_back({from, to});

            pop_bit(possibleMoves);
        }

        pop_bit(knights);
    }
}

void moveGen::genSlidingMoves(Board& board, MoveList& moves) {
    uint64_t bishops = (board.turn == WHITE) ? board.pieces[WB] : board.pieces[BB];
    uint64_t rooks = (board.turn == WHITE) ? board.pieces[WR] : board.pieces[BR];
    uint64_t queens = (board.turn == WHITE) ? board.pieces[WQ] : board.pieces[BQ];
    uint64_t ownPieces = (board.turn == WHITE) ? board.whiteOcc : board.blackOcc;

    while (bishops) {
        int from = get_lsb(bishops);
        int bitCount = __builtin_popcountll(bishopMasks[from]);

        uint64_t blockers = bishopMasks[from] & board.allOcc;

        int magicIdx = (blockers * BishopMagics[from]) >> (64 - bitCount);

        uint64_t possibleMoves = bishopTable[from][magicIdx] & ~ownPieces;

        while (possibleMoves) {
            int to = get_lsb(possibleMoves);
            
            moves.push_back({from, to});
            pop_bit(possibleMoves);
        }
        pop_bit(bishops);
    }

    while (rooks) {
        int from = get_lsb(rooks);
        int bitCount = __builtin_popcountll(rookMasks[from]);

        uint64_t blockers = rookMasks[from] & board.allOcc;

        int magicIdx = (blockers * RookMagics[from]) >> (64 - bitCount);

        uint64_t possibleMoves = rookTable[from][magicIdx] & ~ownPieces;

        while (possibleMoves) {
            int to = get_lsb(possibleMoves);

            moves.push_back({from, to});
            pop_bit(possibleMoves);
        }
        pop_bit(rooks);
    }

    while (queens) {
        int from = get_lsb(queens);
        int rookBitCount = __builtin_popcountll(rookMasks[from]);
        int bishopBitCount = __builtin_popcountll(bishopMasks[from]);

        uint64_t rookBlockers = rookMasks[from] & board.allOcc;
        uint64_t bishopBlockers = bishopMasks[from] & board.allOcc;
        
        int orthogonalIdx = (rookBlockers * RookMagics[from]) >> (64 - rookBitCount);
        int diagonalIdx = (bishopBlockers * BishopMagics[from]) >> (64 - bishopBitCount);

        uint64_t possibleMoves = (rookTable[from][orthogonalIdx] | bishopTable[from][diagonalIdx]) & ~ownPieces;

        while (possibleMoves) {
            int to = get_lsb(possibleMoves);

            moves.push_back({from, to});
            pop_bit(possibleMoves);
        }
        pop_bit(queens);
    }
}

void moveGen::genKingMoves(int sq, Board& board, MoveList& moves) {
    uint64_t moveMask = kingMasks[sq];

    uint64_t myOccupancy = (board.turn == WHITE) ? board.whiteOcc : board.blackOcc;
    moveMask &= ~myOccupancy;
    
    while (moveMask) {
        int to = get_lsb(moveMask);
        if (!isSquareAttacked(board, to)) moves.push_back({sq, to});
        pop_bit(moveMask);
    }

    if (board.turn == WHITE) {
        bool kingSideAttacked = isSquareAttacked(board, 60) || isSquareAttacked(board, 61) || isSquareAttacked(board, 62);
        bool queenSideAttacked = isSquareAttacked(board, 58) || isSquareAttacked(board, 59) || isSquareAttacked(board, 60);

        uint64_t betweenKingSide = (1ULL << 61) | (1ULL << 62);
        uint64_t betweenQueenSide = (1ULL << 57) | (1ULL << 58) | (1ULL << 59);

        if (sq == 60 && board.squares[60] == W_KING && board.squares[63] == W_ROOK &&
            board.w_kingside && !kingSideAttacked && ((board.allOcc & betweenKingSide) == 0)) {
            moves.push_back({sq, 62, CASTLE_KING});
        }
        if (sq == 60 && board.squares[60] == W_KING && board.squares[56] == W_ROOK &&
            board.w_queenside && !queenSideAttacked && ((board.allOcc & betweenQueenSide) == 0)) {
            moves.push_back({sq, 58, CASTLE_QUEEN});
        }
    } else {
        bool kingSideAttacked = isSquareAttacked(board, 4) || isSquareAttacked(board, 5) || isSquareAttacked(board, 6);
        bool queenSideAttacked = isSquareAttacked(board, 2) || isSquareAttacked(board, 3) || isSquareAttacked(board, 4);

        uint64_t betweenKingSide = (1ULL << 5) | (1ULL << 6);
        uint64_t betweenQueenSide = (1ULL << 1) | (1ULL << 2) | (1ULL << 3);

        if (sq == 4 && board.squares[4] == B_KING && board.squares[7] == B_ROOK &&
            board.b_kingside && !kingSideAttacked && ((board.allOcc & betweenKingSide) == 0)) {
            moves.push_back({sq, 6, CASTLE_KING});
        }
        if (sq == 4 && board.squares[4] == B_KING && board.squares[0] == B_ROOK &&
            board.b_queenside && !queenSideAttacked && ((board.allOcc & betweenQueenSide) == 0)) {
            moves.push_back({sq, 2, CASTLE_QUEEN});
        }
    }
}

void addPromotions(int from, int to, MoveList& moves) {
    moves.push_back({from, to, PROMOTION_QUEEN});
    moves.push_back({from, to, PROMOTION_BISHOP});
    moves.push_back({from, to, PROMOTION_KNIGHT});
    moves.push_back({from, to, PROMOTION_ROOK});
}

void moveGen::genPawnMoves(Board& board, MoveList& moves) {
    uint64_t emptyMask = ~board.allOcc;
    uint64_t epMask = (board.enPassantSq == -1) ? 0 : (1ULL << board.enPassantSq);

    if (board.turn == WHITE) {
        uint64_t whiteMask = board.pieces[WP];

        uint64_t whiteSingleMask = (whiteMask >> 8) & emptyMask;
        uint64_t whiteDoubleMask = ((whiteMask & RANK_2) >> 8 & emptyMask) >> 8 & emptyMask;

        uint64_t promoPush = whiteSingleMask & RANK_8;
        uint64_t quietPush = whiteSingleMask & ~RANK_8;
        
        while (quietPush) {
            int to = get_lsb(quietPush);
            
            moves.push_back({to+8, to});
            pop_bit(quietPush);
        }

        while (promoPush) {
            int to = get_lsb(promoPush);

            addPromotions(to + 8, to, moves);

            pop_bit(promoPush);
        }

        while (whiteDoubleMask) {
            int to = get_lsb(whiteDoubleMask);
            moves.push_back({to+16,to, DOUBLE_PAWN_PUSH});
            pop_bit(whiteDoubleMask);
        }

        uint64_t capL = (whiteMask & ~FILE_A) >> 9 & (board.blackOcc | epMask);
        while (capL) {
            int to = get_lsb(capL);
            if ((1ULL << to) & RANK_8) addPromotions(to + 9, to, moves);
            else {
                if ((1ULL << to) == epMask) moves.push_back({to + 9, to, EN_PASSANT});
                else moves.push_back({to + 9, to});
            }
            pop_bit(capL); 
        }

        uint64_t capR = (whiteMask & ~FILE_H) >> 7 & (board.blackOcc | epMask);
        while (capR) {
            int to = get_lsb(capR);
            if ((1ULL << to) & RANK_8) addPromotions(to + 7, to, moves);
            else {
                if ((1ULL << to) == epMask) moves.push_back({to + 7, to, EN_PASSANT});
                else moves.push_back({to + 7, to});
            }
            pop_bit(capR);
        }

    } else {
        uint64_t blackMask = board.pieces[BP];

        uint64_t blackSingleMask = (blackMask << 8) & emptyMask;
        uint64_t blackDoubleMask = ((blackMask & RANK_7) << 8 & emptyMask) << 8 & emptyMask;

        uint64_t promoPush = blackSingleMask & RANK_1;
        uint64_t quietPush = blackSingleMask & ~RANK_1;
        
        while (quietPush) {
            int to = get_lsb(quietPush);
            moves.push_back({to-8, to});
            pop_bit(quietPush);
        }

        while (promoPush) {
            int to = get_lsb(promoPush);

            addPromotions(to-8, to, moves);

            pop_bit(promoPush);
        }

        while (blackDoubleMask) {
            int to = get_lsb(blackDoubleMask);
            moves.push_back({to-16, to, DOUBLE_PAWN_PUSH});
            pop_bit(blackDoubleMask);
        }

        uint64_t capR = (blackMask & ~FILE_A) << 7 & (board.whiteOcc | epMask);
        while (capR) {
            int to = get_lsb(capR);
            if ((1ULL << to) & RANK_1) addPromotions(to-7, to, moves);
            else {
                if ((1ULL << to) == epMask) moves.push_back({to - 7, to, EN_PASSANT});
                else moves.push_back({to - 7, to});
            }
            pop_bit(capR); 
        }

        uint64_t capL = (blackMask & ~FILE_H) << 9 & (board.whiteOcc | epMask);
        while (capL) {
            int to = get_lsb(capL);
            if ((1ULL << to) & RANK_1) addPromotions(to-9, to, moves);
            else {
                if ((1ULL << to) == epMask) moves.push_back({to - 9, to, EN_PASSANT});
                else moves.push_back({to - 9, to});
            }
            pop_bit(capL);
        }
    }
}

bool moveGen::isSquareAttacked(Board& board, int sq) {
    uint64_t bit = (1ULL << sq);
    uint64_t enemyKnights = (board.turn == BLACK) ? board.pieces[WN] : board.pieces[BN];
    uint64_t enemyBishops = (board.turn == BLACK) ? board.pieces[WB] : board.pieces[BB];
    uint64_t enemyRooks = (board.turn == BLACK) ? board.pieces[WR] : board.pieces[BR];
    uint64_t enemyQueens = (board.turn == BLACK) ? board.pieces[WQ] : board.pieces[BQ];
    uint64_t enemyKing = (board.turn == BLACK) ? board.pieces[WK] : board.pieces[BK];
    
    if (board.turn == BLACK && (pawnAttacks[1][sq] & board.pieces[WP])) return true;
    if (board.turn == WHITE && (pawnAttacks[0][sq] & board.pieces[BP])) return true;
    
    uint64_t bishopBlockers = bishopMasks[sq] & board.allOcc;
    uint64_t bishopBitCount = __builtin_popcountll(bishopMasks[sq]);
    uint64_t bishopMagicIdx = (bishopBlockers * BishopMagics[sq]) >> (64 - bishopBitCount);

    uint64_t rookBlockers = rookMasks[sq] & board.allOcc;
    uint64_t rookBitCount = __builtin_popcountll(rookMasks[sq]);
    uint64_t rookMagicIdx = (rookBlockers * RookMagics[sq]) >> (64 - rookBitCount);

    if (knightMasks[sq] & enemyKnights) return true;
    if (kingMasks[sq] & enemyKing) return true;
    if (bishopTable[sq][bishopMagicIdx] & (enemyBishops | enemyQueens)) return true;
    if (rookTable[sq][rookMagicIdx] & (enemyRooks | enemyQueens)) return true;

    return false;
}   
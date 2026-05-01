#include "Types.h"
#include "search.h"
#include "eval.h"
#include "moveGen.h"
#include "board.h"
#include <vector>
#include <iostream>
#include <climits>
#include <chrono>
#include <algorithm>
#include <random>

static constexpr int NEG_INF = -30000;
static constexpr int POS_INF = 30000;

static int nodesLookedAt = 0;
bool stopSearch = false;
Move curBestMove;

static TableEntry TT[8388608];

static const int MvvLvaScores[7][7] = {
// Attackers: [placeholder, P, N, B, R, Q, K]
    {0,  0,  0,  0,  0,  0,  0},   // Victim: placeholder 
    {0, 15, 14, 13, 12, 11, 10},   // Victim: P
    {0, 25, 24, 23, 22, 21, 20},   // Victim: N
    {0, 35, 34, 33, 32, 31, 30},   // Victim: B
    {0, 45, 44, 43, 42, 41, 40},   // Victim: R
    {0, 55, 54, 53, 52, 51, 50},   // Victim: Q
    {0, 65, 64, 63, 62, 61, 60}    // Victim: K
};

static int counter = 0;

void search::initAll() {
    memset(TT, 0, sizeof(TT));
}

Move search::startSearch(Board& board, int maxTimeMs) {
    auto start = std::chrono::steady_clock::now();
    nodesLookedAt = 0;
    stopSearch = false;
    const int rootTurn = board.turn;

    MoveList allMoves;
    moveGen::generateMoves(board, allMoves);

    uint64_t hash = board.currentHash;
    int index = hash & 0x7FFFFF;
    TableEntry entry = TT[index];

    if (entry.hash != 0 && entry.hash == hash) {
        counter++;
        for (int i = 0; i < allMoves.size(); i++) {
            if (allMoves.moves[i] == entry.best) {
                std::swap(allMoves.moves[0], allMoves.moves[i]);
                break;
            }
        }
    }

    if (allMoves.empty()) {
        int kingSq = (board.turn == WHITE) ? moveGen::get_lsb(board.pieces[WK]) : moveGen::get_lsb(board.pieces[BK]);
        int enemy = (board.turn == WHITE) ? BLACK : WHITE;
        if (kingSq != -1 && moveGen::isSquareAttacked(board, kingSq)) {
            return {-1, -1, CHECKMATE_ENGINE, 0};
        } else {
            return {-1, -1, STALEMATE, 0};
        }

        std::cout << "no move" << std::endl;
    }

    int bestEval = NEG_INF;
    Move bestMove = allMoves.moves[0];
    
    for (int curDepth = 1; curDepth <= 64; curDepth++) {
        int curBestEval = NEG_INF;
        Move curBestMove;

        int bestInd = 0;
        for (int i = 0; i < allMoves.size(); i++) {
            Move m = allMoves.moves[i];
            StateInfo s = board.makeMove(m);

            nodesLookedAt++;
            int score = -negamax(curDepth - 1, board, NEG_INF, POS_INF, start, maxTimeMs, true);
            
            board.unmakeMove(m, s);

            if (stopSearch) break;
            
            if (score > curBestEval) {
                curBestEval = score;
                bestInd = i;
                curBestMove = m;
            }
        }

        if (!stopSearch) {
            std::swap(allMoves.moves[0], allMoves.moves[bestInd]);
            bestMove = curBestMove;
            int whitePerspectiveEval = (rootTurn == WHITE) ? curBestEval : -curBestEval;
            std::cout << "depth: " << curDepth
                      << ". Best Eval (stm): " << curBestEval
                      << " | Best Eval (white): " << whitePerspectiveEval
                      << std::endl;
        } else {
            break;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        if (elapsed >= maxTimeMs) break;
    }

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Time taken (Engine Move) (ms): " << diff.count() << std::endl;
    std::cout << "Nodes looked at (Engine Move): " << nodesLookedAt << std::endl;
    std::cout << "TT got " << counter << " hits." << std::endl;

    if (std::abs(bestMove.score) > 29000) bestMove.flags = CHECKMATE_PLAYER;

    return bestMove;
}

void storeEntry(uint64_t hash, int eval, Move best, int depth, BoundType b) {
    int index = hash & 0x7FFFFF;
    
    TT[index] = {hash, eval, best, depth, b};
} 

bool nonKPPresent(Board& board) {
    for (int i = 1; i < 11; i++) {
        if (i == 5 || i == 6) continue;
        if (board.pieces[i] != 0ULL) return true;
    }

    return false;
}

int search::negamax(int depth, Board& board, int alpha, int beta, std::chrono::steady_clock::time_point startTime, int limit, bool allowNullMove) {
    // duration cast -> convert nanoseconds to manageable numbers in this case ms
    // chrono steady clock now -> get current time 
    // - start -> difference from the time at the beginning
    // .count() -> from a chrono object into a number (long here)
    if (nodesLookedAt % 2048 == 0) {
        long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= limit) stopSearch = true;
    }

    if (stopSearch) return 0;

    if (board.isThreefold()) return 0;

    int best = NEG_INF - depth;
    MoveList moves;
    moveGen::generateMoves(board, moves);

    uint64_t hash = board.currentHash;
    int index = hash & 0x7FFFFF;
    TableEntry entry = TT[index];

    if (entry.hash != 0 && entry.hash == hash) {
        counter++;
        if (entry.depth >= depth) {
            if (entry.boundType == EXACT) {
                return entry.eval;
            } else if (entry.boundType == LOWER_BOUND && entry.eval > alpha) {
                alpha = entry.eval;
            } else if (entry.boundType == UPPER_BOUND && entry.eval < beta) {
                beta = entry.eval;
            }

            if (alpha >= beta) {
                return entry.eval;
            }
        }

        for (int i = 0; i < moves.size(); i++) {
            if (moves.moves[i] == entry.best) {
                std::swap(moves.moves[0], moves.moves[i]);
                break;
            }
        }
    }

    if (depth <= 0) {
        return qSearch(board, alpha, beta);
    }

    if (moves.empty()) {
        int kingSq = (board.turn == WHITE) ? moveGen::get_lsb(board.pieces[WK]) : moveGen::get_lsb(board.pieces[BK]);

        if (kingSq != -1 && moveGen::isSquareAttacked(board, kingSq)) {
            return NEG_INF - depth;
        } else {
            return 0;
        }
    }
    
    int originalAlpha = alpha; 
    Move bestMove = moves.moves[0];

    int kingSq = (board.turn == WHITE) ? moveGen::get_lsb(board.pieces[WK]) : moveGen::get_lsb(board.pieces[BK]);
    bool otherPiecesPresent = nonKPPresent(board);
    int originalTurn = board.turn;
    int originalEPsq = board.enPassantSq;
    uint64_t originalHash = board.currentHash;
    bool inCheck = moveGen::isSquareAttacked(board, kingSq);

    if ((kingSq != -1 && !inCheck && depth >= 3 && otherPiecesPresent && allowNullMove)) {
        board.turn *= -1;
        board.currentHash ^= sideKey;
        if (board.enPassantSq != -1) {
            board.currentHash ^= enPassantKeys[board.enPassantSq % 8];
            board.enPassantSq = -1;
        }

        // zero window for more pruning
        if (-negamax(depth - 3, board, -beta, -beta + 1, startTime, limit, false) >= beta) {
            board.turn = originalTurn;
            board.enPassantSq = originalEPsq;
            board.currentHash = originalHash;

            return beta;
        }
    }

    board.turn = originalTurn;
    board.enPassantSq = originalEPsq;
    board.currentHash = originalHash;

    int ind = 0;
    for (Move m : moves) {
        StateInfo s = board.makeMove(m);
        
        nodesLookedAt++;

        int score;
        if (ind >= 3 && s.capturedPiece == EMPTY && !inCheck && (m.flags < 4 || m.flags > 7)) {
            score = -negamax(depth - 3, board, -beta, -alpha, startTime, limit, true);

            if (score > alpha) {
                score = -negamax(depth - 1, board, -beta, -alpha, startTime, limit, true);
            }
        } else {
            score = -negamax(depth - 1, board, -beta, -alpha, startTime, limit, true);
        }

        board.unmakeMove(m, s);

        // best = std::max(best, score);
        if (score > best) {
            best = score;
            bestMove = m;
        }

        alpha = std::max(alpha, score);
        ind++;

        if (alpha >= beta) break;
    }

    BoundType bound;
    if (best <= originalAlpha) bound = UPPER_BOUND;
    else if (best >= beta) bound = LOWER_BOUND;
    else bound = EXACT;
    storeEntry(board.currentHash, best, bestMove, depth, bound);

    return best;
}

inline int pieceTypeIndex(int piece) {
    if (piece == EMPTY) return 0;
    return (piece < 7) ? piece : piece - 8;
}

int getMvvLvaScore(Board& board, Move m) {
    int victim = pieceTypeIndex(board.squares[m.to]);
    int attacker = pieceTypeIndex(board.squares[m.from]);

    return MvvLvaScores[victim][attacker];
}

int search::qSearch(Board& board, int alpha, int beta) {
    if (board.isThreefold()) return 0;

    int standPat = eval::evaluate(board) * board.turn; // Eval if player does nothing

    if (standPat >= beta) return beta; // beta -> lowest score opponent will let us reach
    if (standPat >= alpha) alpha = standPat; // alpha -> best we can get;

    MoveList captures;
    MoveList allMoves;
    moveGen::generateMoves(board, allMoves);
    moveGen::generateCaptures(board, allMoves, captures);

    //MVV-LVA sort here

    for (int i = 0; i < captures.size(); i++) {
        int bestIdx = i;    
        for (int j = i + 1; j < captures.size(); j++) {
            if (getMvvLvaScore(board, captures.moves[j]) > getMvvLvaScore(board, captures.moves[bestIdx])) {
                bestIdx = j;
            }
        }
        
        std::swap(captures.moves[i], captures.moves[bestIdx]);
        Move m = captures.moves[i];

        StateInfo s = board.makeMove(m);

        nodesLookedAt++;
        int score = -qSearch(board, -beta, -alpha);
        board.unmakeMove(m, s);

        if (score >= beta) return beta; // alpha beta prune
        if (score > alpha) alpha = score;
    }

    return alpha;
}

uint64_t perft(int depth, Board& board) {
    if (depth == 0) return 1;

    uint64_t nodes = 0;
    MoveList moves;
    moveGen::generateMoves(board, moves);

    for (const Move& m : moves) {
        StateInfo s = board.makeMove(m);
        nodes += perft(depth - 1, board);
        board.unmakeMove(m, s);
    }

    return nodes;
}

uint64_t search::timedPerft(int depth, Board& board) {
    auto start = std::chrono::steady_clock::now();

    uint64_t nodes = perft(depth, board);

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Time Taken (Engine Move) (ms): " << diff.count() << std::endl;

    std::cout << "Total positions looked at (perft): ";

    return nodes;
}

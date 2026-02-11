#include "Types.h"
#include "board.h"
#include "eval.h"
#include "moveGen.h"

#include <vector>
#include <algorithm>
#include <limits>

static int pieceValue(int piece) {
    switch (piece) {
        case W_PAWN: case B_PAWN: return 100;
        case W_KNIGHT: case B_KNIGHT: return 300;
        case W_BISHOP: case B_BISHOP: return 300;
        case W_ROOK: case B_ROOK: return 500;
        case W_QUEEN: case B_QUEEN: return 900;
        case W_KING: case B_KING: return 20000;
        default: return 0;
    }
}


int eval::evaluate(Board board) {
    int score = 0;

    for (int sq = 0; sq < 64; sq++) {
        int piece = board.squares[sq];
        if (piece == EMPTY) continue;
        int value = pieceValue(piece);
        int color = (piece < 7) ? WHITE : BLACK;

        (piece < 7) ? score += value : score -= value;

        if (moveGen::isSquareAttacked(sq, -color, board)) {
            int SEE = exchangeEvaluation(board, sq) / 4;
            
            if (piece < 7) {
                score -= SEE;
            } else {
                score += SEE;
            }
        }
    }

    return score;
}

int eval::exchangeEvaluation(Board& board, int start) {
    std::vector<Attacker> whiteAttackers;
    std::vector<Attacker> blackAttackers;

    int knightOffsets[] = {-17, -15, -10, -6, 6, 10, 15, 17};
    int kingOffsets[] = {-9, -8, -7, -1, 1, 7, 8, 9};
    int bishopOffsets[] = {-9, -7, 7, 9};
    int rookOffsets[] = {-8, 8, -1, 1};

    for (int offset : knightOffsets) {
        int target = start + offset;

        int startCol = start % 8;
        int startRow = start / 8;
        int endCol = target % 8;
        int endRow = target / 8;

        int colDiff = std::abs(startCol - endCol);
        int rowDiff = std::abs(startRow - endRow);

        if (target >= 0 && target < 64 && ((colDiff == 1 && rowDiff == 2) || (colDiff == 2 && rowDiff == 1))) {
            if (board.squares[target] == W_KNIGHT) {
                whiteAttackers.push_back({target, W_KNIGHT, 300});
            }

            if (board.squares[target] == B_KNIGHT) {
                blackAttackers.push_back({target, B_KNIGHT, 300});
            }
        }
    }

    for (int offset : kingOffsets) {
        int target = start + offset;

        if (target >= 0 && target < 64) {
            if (board.squares[target] == W_KING) {
                whiteAttackers.push_back({target, W_KING, 20000});
            }

            if (board.squares[target] == B_KING) {
                blackAttackers.push_back({target, B_KING, 20000});
            }
        }
    }

    int file = start % 8;
    int wPawnLeft = start + 7;
    int wPawnRight = start + 9;
    if (file > 0 && wPawnLeft >= 0 && wPawnLeft < 64 && board.squares[wPawnLeft] == W_PAWN) {
        whiteAttackers.push_back({wPawnLeft, W_PAWN, 100});
    }
    if (file < 7 && wPawnRight >= 0 && wPawnRight < 64 && board.squares[wPawnRight] == W_PAWN) {
        whiteAttackers.push_back({wPawnRight, W_PAWN, 100});
    }

    int bPawnLeft = start - 7;
    int bPawnRight = start - 9;
    if (file < 7 && bPawnLeft >= 0 && bPawnLeft < 64 && board.squares[bPawnLeft] == B_PAWN) {
        blackAttackers.push_back({bPawnLeft, B_PAWN, 100});
    }
    if (file > 0 && bPawnRight >= 0 && bPawnRight < 64 && board.squares[bPawnRight] == B_PAWN) {
        blackAttackers.push_back({bPawnRight, B_PAWN, 100});
    }

    for (int offset : bishopOffsets) {
        int cur = start;

        while (true) {
            if (!moveGen::canMoveInDirection(cur, offset)) break;

            cur += offset; 

            if (cur < 0 || cur >= 64) break;
            int piece = board.squares[cur];

            if (piece != EMPTY) {
                if (piece == W_BISHOP) {
                    whiteAttackers.push_back({cur, W_BISHOP, 300});
                }

                if (piece == B_BISHOP) {
                    blackAttackers.push_back({cur, B_BISHOP, 300});
                }

                if (piece == W_QUEEN) {
                    whiteAttackers.push_back({cur, W_QUEEN, 900});
                }

                if (piece == B_QUEEN) {
                    blackAttackers.push_back({cur, B_QUEEN, 900});
                }

                break;
            }
        }
    }

    for (int offset : rookOffsets) {
        int cur = start;

        while (true) {
            if (!moveGen::canMoveInDirection(cur, offset)) break;

            cur += offset; 

            if (cur < 0 || cur >= 64) break;
            int piece = board.squares[cur];

            if (piece != EMPTY) {
                if (piece == W_ROOK) {
                    whiteAttackers.push_back({cur, W_ROOK, 500});
                }

                if (piece == B_ROOK) {
                    blackAttackers.push_back({cur, B_ROOK, 500});
                }

                if (piece == W_QUEEN) {
                    whiteAttackers.push_back({cur, W_QUEEN, 900});
                }

                if (piece == B_QUEEN) {
                    blackAttackers.push_back({cur, B_QUEEN, 900});
                }

                break;
            }
        }
    }

    std::sort(whiteAttackers.begin(), whiteAttackers.end(),
        [](const Attacker& a, const Attacker& b) { return a.value < b.value; });
    std::sort(blackAttackers.begin(), blackAttackers.end(),
        [](const Attacker& a, const Attacker& b) { return a.value < b.value; });

    int targetPiece = board.squares[start];
    if (targetPiece == EMPTY) return 0;

    int gain[64];
    int depth = 0;
    gain[0] = pieceValue(targetPiece);

    int side = (targetPiece < 7) ? BLACK : WHITE;
    size_t wIdx = 0;
    size_t bIdx = 0;

    while (true) {
        if (side == WHITE) {
            if (wIdx >= whiteAttackers.size()) break;
            int attackerVal = whiteAttackers[wIdx++].value;
            gain[++depth] = attackerVal - gain[depth - 1];
        } else {
            if (bIdx >= blackAttackers.size()) break;
            int attackerVal = blackAttackers[bIdx++].value;
            gain[++depth] = attackerVal - gain[depth - 1];
        }

        side = (side == WHITE) ? BLACK : WHITE;
        if (depth + 1 >= 63) break;
    }

    for (int i = depth - 1; i >= 0; --i) {
        gain[i] = std::max(gain[i], -gain[i + 1]);
    }

    return gain[0];
}

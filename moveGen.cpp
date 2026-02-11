#include "moveGen.h"
#include <iostream>

std::vector<Move> moveGen::generateMoves(Board& board) {
    std::vector<Move> pseudoMoves;
    std::vector<Move> legalMoves;

    for (int i = 0; i < 64; i++) {
        int piece = board.squares[i];
        if (piece == EMPTY) continue;

        bool isWhitePiece = (piece < 7);
        bool isWhiteTurn = (board.turn == WHITE);

        if (isWhitePiece == isWhiteTurn) {
            if (piece == W_PAWN || piece == B_PAWN) genPawnMoves(i, board, pseudoMoves);
            else if (piece == W_KNIGHT || piece == B_KNIGHT) genKnightMoves(i, board, pseudoMoves);
            else if (piece == W_ROOK || piece == B_ROOK) genSlidingMoves(i, board, pseudoMoves, {-8, 8, -1, 1});
            else if (piece == W_BISHOP || piece == B_BISHOP) genSlidingMoves(i, board, pseudoMoves, {-9, -7, 7, 9});
            else if (piece == W_QUEEN || piece == B_QUEEN) genSlidingMoves(i, board, pseudoMoves, {-8, 8, -1, 1, -9, -7, 7, 9});
            else if (piece == W_KING || piece == B_KING) genKingMoves(i, board, pseudoMoves);
        }
    }

    int myColor = board.turn;
    int enemyColor = (myColor == WHITE) ? BLACK : WHITE;

    for (Move m : pseudoMoves) {
        int capturedPiece = board.squares[m.to];
        int oldEP = board.enPassantSq;
        bool oWKS = board.w_kingside;
        bool oWQS = board.w_queenside;
        bool oBKS = board.b_kingside;
        bool oBQS = board.b_queenside;

        board.makeMove(m);
        int kingSq = findKing(board, myColor);
                
        if (!isSquareAttacked(kingSq, enemyColor, board)) {
            legalMoves.push_back(m);
        }

        board.unmakeMove(m, capturedPiece, oldEP, oWKS, oWQS, oBKS, oBQS);
    }

    return legalMoves;
}

std::string moveGen::toAlgebraic(int index) {
    int file = index % 8;
    int rank = 8 - (index / 8);
    char fileChar = (char) ('a' + file);

    return std::string(1, fileChar) + std::to_string(rank);
}

bool moveGen::isSquareAttacked(int targetSq, int attackerColor, Board& board) {
    // knight check
    int knightOffsets[] = {-17, -15, -10, -6, 6, 10, 15, 17};
    int enemyKnight = (attackerColor == 1) ? W_KNIGHT : B_KNIGHT;

    for (int offset : knightOffsets) {
        int sq = targetSq + offset;
        
        if (sq >= 0 && sq < 64 && isSafeJump(sq, targetSq)) {
            if (board.squares[sq] == enemyKnight) return true;
        }
    }

    // sliding check
    if (attackedBySlider(targetSq, attackerColor, board, { -8, 8, -1, 1 }, true)) return true; // rook queen check
    if (attackedBySlider(targetSq, attackerColor, board, { -9, -7, 7, 9 }, false)) return true; // bishop queen check

    //pawn check
    int enemyPawn = (attackerColor == 1) ? W_PAWN : B_PAWN;

    // Pawns attack "forward" from their perspective; to test attacks on targetSq,
    // we look from targetSq back to the pawn's source squares.
    std::vector<int> pawnOffsets = (attackerColor == WHITE) ? std::vector<int>{7, 9} : std::vector<int>{-7, -9};
    for (int offset : pawnOffsets) {
        int sq = targetSq + offset;

        if (sq >= 0 && sq < 64 && std::abs((sq % 8) - (targetSq % 8)) == 1) {
            if (board.squares[sq] == enemyPawn) return true;
        }
    }

    // king check
    int enemyKing = (attackerColor == 1) ? W_KING : B_KING;

    int kingOffsets[] = {-9, -8, -7, -1, 1, 7, 8, 9};
    for (int offset : kingOffsets) {
        int sq = targetSq + offset;

        if (sq >= 0 && sq < 64 && std::abs((sq % 8) - (targetSq % 8)) <= 1) {
            if (board.squares[sq] == enemyKing) return true;
        }
    }
    
    // passed
    return false;
}

void moveGen::genPawnMoves(int sq, Board& board, std::vector<Move>& moves) {
    int rank = sq / 8;
    int file = sq % 8;

    // WHITE pawn
    if (board.turn == WHITE) {
        int forward = sq - 8;
        
        if (forward >= 0 && board.squares[forward] == EMPTY) {
            if (rank == 1) {
                addPromotionMoves(sq, forward, moves);
            } else {
                moves.push_back({sq, forward});

                if (rank == 6 && board.squares[sq - 16] == EMPTY) {
                    moves.push_back({sq, sq - 16, DOUBLE_PAWN_PUSH});
                }
            }
        }

        if (file > 0) {
            int diagLeft = sq - 9;
            if (isEnemy(W_PAWN, board.squares[diagLeft])) {
                if (rank == 1) {
                    addPromotionMoves(sq, diagLeft, moves);
                } else {
                    moves.push_back({sq, diagLeft});
                }
            } else if (diagLeft == board.enPassantSq) {
                int victimSq = diagLeft + 8;
                if (victimSq >= 0 && victimSq < 64 && board.squares[victimSq] == B_PAWN) {
                    moves.push_back({sq, diagLeft, EN_PASSANT});
                }
            }
        }

        if (file < 7) {
            int diagRight = sq - 7;
            if (isEnemy(W_PAWN, board.squares[diagRight])) {
                if (rank == 1) {
                    addPromotionMoves(sq, diagRight, moves);
                } else {
                    moves.push_back({sq, diagRight});
                }
            } else if (diagRight == board.enPassantSq) {
                int victimSq = diagRight + 8;
                if (victimSq >= 0 && victimSq < 64 && board.squares[victimSq] == B_PAWN) {
                    moves.push_back({sq, diagRight, EN_PASSANT});
                }
            }
        }
    }

    // BLACK pawn
    if (board.turn == BLACK) {
        int forward = sq + 8;

        if (forward < 64 && board.squares[forward] == EMPTY) {
            if (rank == 6) {
                addPromotionMoves(sq, forward, moves);
            } else {
                moves.push_back({sq, forward});

                if (rank == 1 && board.squares[sq + 16] == EMPTY) {
                    moves.push_back({sq, sq + 16, DOUBLE_PAWN_PUSH});
                }
            }
        }

        if (file > 0) {
            int diagLeft = sq + 7; 
            if (isEnemy(B_PAWN, board.squares[diagLeft])) {
                if (rank == 6) {
                    addPromotionMoves(sq, diagLeft, moves);
                } else {
                    moves.push_back({sq, diagLeft});
                }
            } else if (diagLeft == board.enPassantSq) {
                int victimSq = diagLeft - 8;
                if (victimSq >= 0 && victimSq < 64 && board.squares[victimSq] == W_PAWN) {
                    moves.push_back({sq, diagLeft, EN_PASSANT});
                }
            }
        }

        if (file < 7) {
            int diagRight = sq + 9;
            if (isEnemy(B_PAWN, board.squares[diagRight])) {
                if (rank == 6) {
                    addPromotionMoves(sq, diagRight, moves);
                } else {
                    moves.push_back({sq, diagRight});
                }
            } else if (diagRight == board.enPassantSq) {
                int victimSq = diagRight - 8;
                if (victimSq >= 0 && victimSq < 64 && board.squares[victimSq] == W_PAWN) {
                    moves.push_back({sq, diagRight, EN_PASSANT});
                }
            }
        }
    }
}

void moveGen::genKnightMoves(int sq, Board& board, std::vector<Move>& moves) {
    int knightOffsets[] = {-17, -15, -10, -6, 6, 10, 15, 17};
    int startFile = sq % 8;
    int startRank = sq / 8;

    for (int offset : knightOffsets) {
        int target = sq + offset;
        if (target >= 0 && target < 64) {
            int targetFile = target % 8;
            int targetRank = target / 8;

            if (std::abs(startFile - targetFile) <= 2 && std::abs(startRank - targetRank) <= 2) {
                int pieceAtTarget = board.squares[target];

                if (pieceAtTarget == EMPTY || isEnemy(board.squares[sq], pieceAtTarget)) {
                    moves.push_back({sq, target});
                }
            }
        }
    }
}

void moveGen::genSlidingMoves(int sq, Board& board, std::vector<Move>& moves, const std::vector<int>& offsets) {
    for (int offset : offsets) {
        int target = sq;

        while (true) {
            if (!canMoveInDirection(target, offset)) break;

            target += offset;

            if (target < 0 || target >= 64) break;

            int pieceAtTarget = board.squares[target];

            if (pieceAtTarget == EMPTY) {
                moves.push_back({sq, target});
            } else if (isEnemy(board.squares[sq], board.squares[target])) {
                moves.push_back({sq, target});
                break;
            } else {
                break;
            }
        }
    }
}

void moveGen::genKingMoves(int sq, Board& board, std::vector<Move>& moves) {
    int kingOffsets[] = {-9, -8, -7, -1, 1, 7, 8, 9};
    int myColor = board.turn;  
    int enemyColor = (myColor == WHITE) ? BLACK : WHITE;
    
    for (int offset: kingOffsets) {
        int target = sq + offset;

        if (target >= 0 && target < 64 && isSafeJumpKing(sq, target)) {
            int pieceAtTarget = board.squares[target];

            if (pieceAtTarget == EMPTY || isEnemy(board.squares[sq], pieceAtTarget)) {
                moves.push_back({sq, target});
            }
        }
    }

    if (myColor == WHITE && sq == 60) {
        genCastlingMoves(sq, 1, board, moves);
    } else if (myColor == BLACK && sq == 4) {
        genCastlingMoves(sq, 2, board, moves);
    }
}

void moveGen::genCastlingMoves(int sq, int color, Board& board, std::vector<Move>& moves) {
    int enemyColor = (color == WHITE) ? BLACK : WHITE;
    if (isSquareAttacked(sq, enemyColor, board)) return;

    if (color == WHITE) {
        // king side white
        if (board.w_kingside && board.squares[61] == EMPTY && board.squares[62] == EMPTY) {
            if (!isSquareAttacked(61, enemyColor, board) && !isSquareAttacked(62, enemyColor, board)) {
                moves.push_back({60, 62, CASTLE_KING});
            }
        }

        // queen side white
        if (board.w_queenside && board.squares[59] == EMPTY && board.squares[58] == EMPTY && board.squares[57] == EMPTY) {
            if (!isSquareAttacked(59, enemyColor, board) && !isSquareAttacked(58, enemyColor, board)) {
                moves.push_back({60, 58, CASTLE_QUEEN});
            }
        }
    }

    if (color == BLACK) {
        // king side black
        if (board.b_kingside && board.squares[5] == EMPTY && board.squares[6] == EMPTY) {
            if (!isSquareAttacked(5, enemyColor, board) && !isSquareAttacked(6, enemyColor, board)) {
                moves.push_back({4, 6, CASTLE_KING});
            }
        }

        // queen side black
        if (board.b_queenside && board.squares[3] == EMPTY && board.squares[2] == EMPTY && board.squares[1] == EMPTY) {
            if (!isSquareAttacked(3, enemyColor, board) && !isSquareAttacked(2, enemyColor, board)) {
                moves.push_back({4, 2, CASTLE_QUEEN});
            }
        }
    }
}

bool moveGen::canMoveInDirection(int sq, int offset) {
    int file = sq % 8;
    int rank = sq / 8;
    if (file == 7 && (offset == 1 || offset == -7 || offset == 9)) return false;
    if (file == 0 && (offset == -1 || offset == 7 || offset == -9)) return false;

    return true;
}

bool moveGen::isSafeJump(int startSq, int targetSq) {
    int startFile = startSq % 8;
    int startRank = startSq / 8;
    int endFile = targetSq % 8;
    int endRank = targetSq / 8;

    int df = std::abs(startFile - endFile);
    int dr = std::abs(startRank - endRank);
    return (df == 1 && dr == 2) || (df == 2 && dr == 1);
}

bool moveGen::isSafeJumpKing(int startSq, int targetSq) {
    int startFile = startSq % 8;
    int startRank = startSq / 8;
    int endFile = targetSq % 8;
    int endRank = targetSq / 8;

    return std::abs(startFile - endFile) <= 1 && std::abs(startRank - endRank) <= 1;
}

bool moveGen::attackedBySlider(int targetSq, int attackerColor, Board& board, const std::vector<int>& offsets, bool isRook) {
    for (int offset : offsets) {
        int currentSq = targetSq;
        while (true) {
            if (!canMoveInDirection(currentSq, offset)) break;

            currentSq += offset;
            if (currentSq < 0 || currentSq >= 64) break;
            int piece = board.squares[currentSq];

            if (piece != EMPTY) {
                bool isEnemyPiece = (attackerColor == WHITE && piece < 7) || (attackerColor == BLACK && piece > 8);

                if (isEnemyPiece) {
                    if (isRook) {
                        if (piece == W_ROOK || piece == B_ROOK || piece == W_QUEEN || piece == B_QUEEN) return true;
                    } else {
                        if (piece == W_BISHOP || piece == B_BISHOP || piece == W_QUEEN || piece == B_QUEEN) return true;
                    }
                }
                break;
            } 
        }
    }

    return false;
}

int moveGen::findKing(Board& board, int color) {
    int targetKing = (color == 1) ? W_KING : B_KING;
    for (int i = 0; i < 64; i++) {
        if (board.squares[i] == targetKing) return i;
    }
    return -1; 
}

void moveGen::addPromotionMoves(int from, int to, std::vector<Move>& moves) {
    moves.push_back({from, to, PROMOTION_QUEEN});
    moves.push_back({from, to, PROMOTION_ROOK});
    moves.push_back({from, to, PROMOTION_BISHOP});
    moves.push_back({from, to, PROMOTION_KNIGHT});
}

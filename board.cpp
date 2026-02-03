#include "board.h"

Board::Board() {
    reset();
}

void Board::reset() {
    for (int i = 0; i < 64; i++) squares[i] = EMPTY;

    turn = WHITE;
    enPassantSq = -1;
    w_kingside = w_queenside = b_kingside = b_queenside = true;
}

void Board::makeMove(Move m) {
    squares[m.to] = squares[m.from];
    squares[m.from] = EMPTY;

    turn = (turn == WHITE) ? BLACK : WHITE; 
}

void Board::unmakeMove(Move m, int capturedPiece, int oldEP, bool oldWKS, bool oldWQS, bool oldBKS, bool oldBQS) {
    squares[m.from] = squares[m.to];
    squares[m.to] = capturedPiece;

    enPassantSq = oldEP;
    w_kingside = oldWKS;
    w_queenside = oldWQS;
    b_kingside = oldBKS;
    b_queenside = oldBQS;
    turn = (turn == WHITE) ? BLACK : WHITE;
}
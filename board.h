#ifndef BOARD_H
#define BOARD_H

#include <string>
#include "Types.h"

class Board{
public: 
    int squares[64];
    int turn;
    int enPassantSq;

    bool w_kingside, w_queenside;
    bool b_kingside, b_queenside;

    Board();
    void reset();
    void makeMove(Move m);
    void unmakeMove(Move m, int capturedPiece, int oldEP, bool oldWKS, bool oldWQS, bool oldBKS, bool oldBQS);

    void loadFromFen(std::string fen);
    int charToPiece(char c);
};

#endif
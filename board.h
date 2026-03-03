#ifndef BOARD_H
#define BOARD_H

#include <string>
#include "Types.h"

struct StateInfo {
   int capturedPiece;
   int enPassantSq;
   bool w_kingside;
   bool w_queenside;
   bool b_kingside;
   bool b_queenside;
};

class Board{
public: 
    int squares[64];
    int turn;
    int enPassantSq;

    const int WP = 0;
    const int WN = 1;
    const int WB = 2;
    const int WR = 3;
    const int WQ = 4;
    const int WK = 5;
    const int BP = 6;
    const int BN = 7;
    const int BB = 8;
    const int BR = 9;
    const int BQ = 10;
    const int BK = 11;

    uint64_t pieces[12];
    uint64_t whiteOcc; // pieces[WP] | pieces[WN] | pieces[WB] | pieces[WR] | pieces[WQ] | pieces[WK];
    uint64_t blackOcc; // = pieces[BP] | pieces[BN] | pieces[BB] | pieces[BR] | pieces[BQ] | pieces[BK];
    uint64_t allOcc;

    bool w_kingside, w_queenside;
    bool b_kingside, b_queenside;

    Board();
    void reset();
    StateInfo makeMove(Move m);
    void unmakeMove(Move m, StateInfo s);

    void resetBb();
    void loadFromFen(std::string fen);
    int charToPiece(char c);
    bool validate() const;
};

#endif

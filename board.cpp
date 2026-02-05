#include "board.h"
#include <sstream>

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
    int piece = squares[m.from];
    squares[m.to] = piece;
    squares[m.from] = EMPTY;

    if (m.flags == PROMOTION_QUEEN) {
        squares[m.to] = (turn == WHITE) ? W_QUEEN : B_QUEEN;
    } 
    else if (m.flags == EN_PASSANT) {
        int victimSq = (turn == WHITE) ? m.to + 8 : m.to - 8;
        squares[victimSq] = EMPTY;
    } 
    else if (m.flags == DOUBLE_PAWN_PUSH) {
        enPassantSq = (turn == WHITE) ? m.to + 8 : m.to - 8;
    } 
    else if (m.flags == CASTLE_KING) {
        if (turn == WHITE) { squares[61] = W_ROOK; squares[63] = EMPTY; }
        else { squares[5] = B_ROOK; squares[7] = EMPTY; }
    }

    if (m.flags != DOUBLE_PAWN_PUSH) enPassantSq = -1;
    turn = (turn == WHITE) ? BLACK : WHITE; 
}

void Board::unmakeMove(Move m, int capturedPiece, int oldEP, bool oldWKS, bool oldWQS, bool oldBKS, bool oldBQS) {
    turn = (turn == WHITE) ? BLACK : WHITE;

    if (m.flags == PROMOTION_QUEEN) {
        squares[m.from] = (turn == WHITE) ? W_PAWN : B_PAWN;
    } else {
        squares[m.from] = squares[m.to];
    }

    squares[m.to] = capturedPiece;

    if (m.flags == EN_PASSANT) {
        squares[m.to] = EMPTY;
        int victimSq = (turn == WHITE) ? m.to + 8 : m.to - 8;
        squares[victimSq] = (turn == WHITE) ? B_PAWN : W_PAWN;
    } else if (m.flags == CASTLE_KING) {
        if (turn == WHITE) { 
            squares[63] = W_ROOK; squares[61] = EMPTY;
        } else {
            squares[7] = B_ROOK; squares[5] = EMPTY;
        }
    }
    enPassantSq = oldEP;
    w_kingside = oldWKS;
    w_queenside = oldWQS;
    b_kingside = oldBKS;
    b_queenside = oldBQS;
}

void Board::loadFromFen(std::string fen) {
    reset();
    std::stringstream ss(fen);
    std::string position, activeColor, castling, enPassant;
    ss >> position >> activeColor >> castling >> enPassant;

    int square = 0;
    for (char c : position) {
        if (c == '/') continue;
        if (isdigit(c)) {
            square += (c - '0');
        } else {
            squares[square++] = charToPiece(c);
        }
    }

    turn = (activeColor == "w") ? 1 : 2;
}

int Board::charToPiece(char c) {
    switch (c) {
        case 'P': return W_PAWN;   case 'p': return B_PAWN;
        case 'N': return W_KNIGHT; case 'n': return B_KNIGHT;
        case 'B': return W_BISHOP; case 'b': return B_BISHOP;
        case 'R': return W_ROOK;   case 'r': return B_ROOK;
        case 'Q': return W_QUEEN;  case 'q': return B_QUEEN;
        case 'K': return W_KING;   case 'k': return B_KING;
        default: return EMPTY;
    }
}
#include "board.h"
#include <string>
#include <sstream>
#include <cassert>

Board::Board() {
    reset();
}

static int pieceToBbIndex(int piece) {
    switch(piece) {
        case W_PAWN: return 0;
        case W_KNIGHT: return 1;
        case W_BISHOP: return 2;
        case W_ROOK: return 3;
        case W_QUEEN: return 4;
        case W_KING: return 5;
        case B_PAWN: return 6;
        case B_KNIGHT: return 7;
        case B_BISHOP: return 8;
        case B_ROOK: return 9;
        case B_QUEEN: return 10;
        case B_KING: return 11;
        default: return -1;
    }
}

void Board::reset() {
    for (int i = 0; i < 64; i++) squares[i] = EMPTY;

    turn = WHITE;
    enPassantSq = -1;
    w_kingside = w_queenside = b_kingside = b_queenside = true;
}

StateInfo Board::makeMove(Move m) {
    StateInfo s = {squares[m.to], enPassantSq, w_kingside, w_queenside, b_kingside, b_queenside};

    uint64_t fromMask = 1ULL << (m.from);
    uint64_t toMask = 1ULL << (m.to);

    int original = m.from;
    int piece = squares[m.from];
    int movingInd = pieceToBbIndex(piece);
    assert(movingInd >= 0);
    if (movingInd < 0) return s;
    pieces[movingInd] &= ~fromMask;
    pieces[movingInd] |= toMask;

    if (s.capturedPiece != EMPTY && m.flags != EN_PASSANT) pieces[pieceToBbIndex(s.capturedPiece)] &= ~toMask;

    squares[m.to] = piece;
    squares[m.from] = EMPTY;
    

    if ((piece == W_KING) && m.flags != CASTLE_KING && m.flags != CASTLE_QUEEN) {
        w_kingside = w_queenside = false;
    }

    if (piece == W_ROOK && original == 63 && m.flags != CASTLE_KING) {
        w_kingside = false;
    }

    if (piece == W_ROOK && original == 56 && m.flags != CASTLE_KING) {
        w_queenside = false;
    }


    if ((piece == B_KING) && m.flags != CASTLE_KING && m.flags != CASTLE_QUEEN) {
        b_kingside = b_queenside = false;
    }

    if (piece == B_ROOK && original == 7 && m.flags != CASTLE_KING) {
        b_kingside = false;
    }

    if (piece == B_ROOK && original == 0 && m.flags != CASTLE_KING) {
        b_queenside = false;
    }

    if (m.flags == PROMOTION_QUEEN || m.flags == PROMOTION_ROOK ||
        m.flags == PROMOTION_BISHOP || m.flags == PROMOTION_KNIGHT) {

        if (m.flags == PROMOTION_QUEEN) {
            if (turn == WHITE) {
                squares[m.to] = W_QUEEN;

                pieces[WP] &= ~toMask;
                pieces[WQ] |= toMask;
            } else {
                squares[m.to] = B_QUEEN;

                pieces[BP] &= ~toMask;
                pieces[BQ] |= toMask;
            }
        } else if (m.flags == PROMOTION_ROOK) {
            if (turn == WHITE) {
                squares[m.to] = W_ROOK;

                pieces[WP] &= ~toMask;
                pieces[WR] |= toMask;
            } else {
                squares[m.to] = B_ROOK;

                pieces[BP] &= ~toMask;
                pieces[BR] |= toMask;
            }
        } else if (m.flags == PROMOTION_BISHOP) {
            if (turn == WHITE) {
                squares[m.to] = W_BISHOP;

                pieces[WP] &= ~toMask;
                pieces[WB] |= toMask;
            } else {
                squares[m.to] = B_BISHOP;

                pieces[BP] &= ~toMask;
                pieces[BB] |= toMask;
            }
        } else if (m.flags == PROMOTION_KNIGHT) {
            if (turn == WHITE) {
                squares[m.to] = W_KNIGHT;

                pieces[WP] &= ~toMask;
                pieces[WN] |= toMask;
            } else {
                squares[m.to] = B_KNIGHT;

                pieces[BP] &= ~toMask;
                pieces[BN] |= toMask;
            }
        }
    } else if (m.flags == EN_PASSANT) {
        int victimSq = (turn == WHITE) ? m.to + 8 : m.to - 8;
        squares[victimSq] = EMPTY;
        uint64_t victimMask = 1ULL << (victimSq);
        int enemyPawnIdx = (turn == WHITE) ? BP : WP;

        pieces[enemyPawnIdx] &= ~victimMask;
        
    } 
    else if (m.flags == DOUBLE_PAWN_PUSH) {
        if (turn == WHITE) {
            enPassantSq = m.to + 8;
        } else {
            enPassantSq = m.to - 8;
        }
    } 
    else if (m.flags == CASTLE_KING) {
        if (turn == WHITE) { 
            squares[61] = W_ROOK; squares[63] = EMPTY; 

            uint64_t rookFromMask = 1ULL << 7;
            uint64_t rookToMask = 1ULL << 5;

            pieces[WR] &= ~rookFromMask;
            pieces[WR] |= rookToMask;
        } else { 
            squares[5] = B_ROOK; 
            squares[7] = EMPTY; 

            uint64_t rookFromMask = 1ULL << 63;
            uint64_t rookToMask = 1ULL << 61;

            pieces[BR] &= ~rookFromMask;
            pieces[BR] |= rookToMask;
        }
    }
    else if (m.flags == CASTLE_QUEEN) {
        if (turn == WHITE) { 
            squares[59] = W_ROOK; 
            squares[56] = EMPTY; 

            uint64_t rookFromMask = 1ULL << 0;
            uint64_t rookToMask = 1ULL << 3;

            pieces[WR] &= ~rookFromMask;
            pieces[WR] |= rookToMask;
        } else { 
            squares[3] = B_ROOK; 
            squares[0] = EMPTY; 
            
            uint64_t rookFromMask = 1ULL << 56;
            uint64_t rookToMask = 1ULL << 59;

            pieces[BR] &= ~rookFromMask;
            pieces[BR] |= rookToMask;
        }
    }

    if (m.flags != DOUBLE_PAWN_PUSH) enPassantSq = -1;
    turn = (turn == WHITE) ? BLACK : WHITE; 

    whiteOcc = pieces[WP] | pieces[WN] | pieces[WB] | pieces[WR] | pieces[WQ] | pieces[WK];
    blackOcc = pieces[BP] | pieces[BN] | pieces[BB] | pieces[BR] | pieces[BQ] | pieces[BK];
    allOcc = whiteOcc | blackOcc;

    return s;
}

void Board::unmakeMove(Move m, StateInfo s) {
    turn = (turn == WHITE) ? BLACK : WHITE;
    int piece = squares[m.to];

    if (m.flags == EN_PASSANT) {
        const int victimSq = (turn == WHITE) ? m.to + 8 : m.to - 8;

        const uint64_t victimMask = 1ULL << (victimSq);
        const uint64_t toMask = 1ULL << (m.to);
        const uint64_t fromMask = 1ULL << (m.from);

        const int moverPawnIdx = (turn == WHITE) ? WP : BP;
        const int victimPawnIdx = (turn == WHITE) ? BP : WP;

        pieces[moverPawnIdx] &= ~toMask;
        pieces[moverPawnIdx] |= fromMask;
        pieces[victimPawnIdx] |= victimMask;

        squares[m.from] = (turn == WHITE) ? W_PAWN : B_PAWN;
        squares[m.to] = EMPTY;
        squares[victimSq] = (turn == WHITE) ? B_PAWN : W_PAWN;
    } else if (m.flags == CASTLE_KING) {
        if (turn == WHITE) { 
            squares[60] = W_KING;
            squares[62] = EMPTY;
            squares[63] = W_ROOK; 
            squares[61] = EMPTY;

            uint64_t kingFromMask = 1ULL << (62);
            uint64_t kingToMask = 1ULL << (60);
            uint64_t rookFromMask = 1ULL << 7;
            uint64_t rookToMask = 1ULL << 5;

            pieces[WK] &= ~kingFromMask;
            pieces[WK] |= kingToMask;
            pieces[WR] &= ~rookToMask;
            pieces[WR] |= rookFromMask;
        } else {
            squares[4] = B_KING;
            squares[6] = EMPTY;
            squares[7] = B_ROOK; 
            squares[5] = EMPTY;

            uint64_t kingFromMask = 1ULL << (6);
            uint64_t kingToMask = 1ULL << (4);
            uint64_t rookFromMask = 1ULL << 63;
            uint64_t rookToMask = 1ULL << 61;

            pieces[BK] &= ~kingFromMask;
            pieces[BK] |= kingToMask;
            pieces[BR] &= ~rookToMask;
            pieces[BR] |= rookFromMask;
        }
    } else if (m.flags == CASTLE_QUEEN) {
        if (turn == WHITE) {
            squares[60] = W_KING;
            squares[58] = EMPTY;
            squares[56] = W_ROOK; 
            squares[59] = EMPTY;

            uint64_t kingFromMask = 1ULL << (58);
            uint64_t kingToMask = 1ULL << (60);
            uint64_t rookFromMask = 1ULL << 0;
            uint64_t rookToMask = 1ULL << 3;

            pieces[WK] &= ~kingFromMask;
            pieces[WK] |= kingToMask;
            pieces[WR] &= ~rookToMask;
            pieces[WR] |= rookFromMask;
        } else {
            squares[4] = B_KING;
            squares[2] = EMPTY;
            squares[0] = B_ROOK; 
            squares[3] = EMPTY;

            uint64_t kingFromMask = 1ULL << (2);
            uint64_t kingToMask = 1ULL << (4);
            uint64_t rookFromMask = 1ULL << 56;
            uint64_t rookToMask = 1ULL << 59;

            pieces[BK] &= ~kingFromMask;
            pieces[BK] |= kingToMask;
            pieces[BR] &= ~rookToMask;
            pieces[BR] |= rookFromMask;
        }
    } else if (m.flags == PROMOTION_QUEEN || m.flags == PROMOTION_ROOK ||
        m.flags == PROMOTION_BISHOP || m.flags == PROMOTION_KNIGHT) {

        if (turn == WHITE) {
            squares[m.from] = W_PAWN;

            uint64_t toMask = 1ULL << (m.to);
            uint64_t fromMask = 1ULL << (m.from);

            int promotionInd = WQ;
            if (m.flags == PROMOTION_QUEEN) {
                promotionInd = WQ;
            } else if (m.flags == PROMOTION_ROOK) {
                promotionInd = WR;
            } else if (m.flags == PROMOTION_BISHOP) {
                promotionInd = WB;
            } else if (m.flags == PROMOTION_KNIGHT) {
                promotionInd = WN;
            }

            pieces[promotionInd] &= ~toMask;
            pieces[WP] |= fromMask;
        } else {
            squares[m.from] = B_PAWN;

            uint64_t toMask = 1ULL << (m.to);
            uint64_t fromMask = 1ULL << (m.from);

            int promotionInd = BQ;
            if (m.flags == PROMOTION_QUEEN) {
                promotionInd = BQ;
            } else if (m.flags == PROMOTION_ROOK) {
                promotionInd = BR;
            } else if (m.flags == PROMOTION_BISHOP) {
                promotionInd = BB;
            } else if (m.flags == PROMOTION_KNIGHT) {
                promotionInd = BN;
            }

            pieces[promotionInd] &= ~toMask;
            pieces[BP] |= fromMask;
        }
    } else {
        uint64_t fromMask = 1ULL << (m.from);
        uint64_t toMask = 1ULL << (m.to);

        int moveIdx = pieceToBbIndex(piece);

        if (moveIdx != -1) {
            pieces[moveIdx] &= ~toMask;
            pieces[moveIdx] |= fromMask;
        } 

        squares[m.from] = squares[m.to];

        if (s.capturedPiece != EMPTY && m.flags != EN_PASSANT) pieces[pieceToBbIndex(s.capturedPiece)] |= toMask;
    }

    if (m.flags != EN_PASSANT) squares[m.to] = s.capturedPiece;
    enPassantSq = s.enPassantSq;
    w_kingside = s.w_kingside;
    w_queenside = s.w_queenside;
    b_kingside = s.b_kingside;
    b_queenside = s.b_queenside;

    whiteOcc = pieces[WP] | pieces[WN] | pieces[WB] | pieces[WR] | pieces[WQ] | pieces[WK];
    blackOcc = pieces[BP] | pieces[BN] | pieces[BB] | pieces[BR] | pieces[BQ] | pieces[BK];
    allOcc = whiteOcc | blackOcc;
}

void Board::resetBb() {
    for (int i = 0; i < 12; i++) {
        pieces[i] = 0ULL;
    }

    whiteOcc = 0ULL;
    blackOcc = 0ULL;
    allOcc = 0ULL;
}

void Board::loadFromFen(std::string fen) {
    reset();
    resetBb();

    std::stringstream ss(fen);
    std::string position, activeColor, castling, enPassant;
    ss >> position >> activeColor >> castling >> enPassant;

    w_kingside = (castling.find('K') != std::string::npos);
    w_queenside = (castling.find('Q') != std::string::npos);
    b_kingside = (castling.find('k') != std::string::npos);
    b_queenside = (castling.find('q') != std::string::npos);

    if (enPassant != "-") {
        int col = enPassant[0] - 'a';
        int row = '8' - enPassant[1];
        enPassantSq = row * 8 + col;
    } else {
        enPassantSq = -1;
    }

    int square = 0;
    for (char c : position) {
        int mailboxPiece = charToPiece(c);

        if (c == '/') continue;
        if (isdigit(c)) {
            square += (c - '0');
            continue;
        } 
        
        squares[square] = mailboxPiece;
        
        int bbIndex = pieceToBbIndex(mailboxPiece);
        uint64_t bit = (1ULL << (square));
        
        pieces[bbIndex] |= bit;
        if (mailboxPiece < 7) {
            whiteOcc |= bit;
        } else {
            blackOcc |= bit;
        }

        square++;
    }
    allOcc = whiteOcc | blackOcc;

    turn = (activeColor == "w") ? WHITE : BLACK;
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

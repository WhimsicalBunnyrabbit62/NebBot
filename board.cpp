#include "board.h"
#include <string>
#include <sstream>
#include <cassert>
#include <random>
#include <iostream>

uint64_t pieceKeys[12][64];
uint64_t sideKey;
uint64_t enPassantKeys[8];
uint64_t castleKeys[16];
static void initZobrist();

static int castleRightsMask(const Board& board) {
    int rights = 0;
    if (board.w_kingside) rights |= 1;
    if (board.w_queenside) rights |= 2;
    if (board.b_kingside) rights |= 4;
    if (board.b_queenside) rights |= 8;
    return rights;
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

static int promotionPieceForMove(int sideToMove, int moveFlag) {
    if (sideToMove == WHITE) {
        if (moveFlag == PROMOTION_QUEEN) return W_QUEEN;
        if (moveFlag == PROMOTION_ROOK) return W_ROOK;
        if (moveFlag == PROMOTION_BISHOP) return W_BISHOP;
        if (moveFlag == PROMOTION_KNIGHT) return W_KNIGHT;
    } else {
        if (moveFlag == PROMOTION_QUEEN) return B_QUEEN;
        if (moveFlag == PROMOTION_ROOK) return B_ROOK;
        if (moveFlag == PROMOTION_BISHOP) return B_BISHOP;
        if (moveFlag == PROMOTION_KNIGHT) return B_KNIGHT;
    }
    return EMPTY;
}

Board::Board() {
    reset();
}

void Board::initAll() {
    initZobrist();
}

static void initZobrist() {
    std::mt19937_64 rng(123456789ULL);

    for (int p = 0; p < 12; p++) {
        for (int s = 0; s < 64; s++) {
            pieceKeys[p][s] = rng();
        }
    }

    sideKey = rng();
    for (int i = 0; i < 16; i++) castleKeys[i] = rng();
    for (int i = 0; i < 8; i++) enPassantKeys[i] = rng();
}

uint64_t Board::generateHash() const {
    uint64_t posKey = 0;

    for (int sq = 0; sq < 64; sq++) {
        int piece = squares[sq];
        if (piece != EMPTY) {
            int bbIdx = pieceToBbIndex(piece);
            if (bbIdx >= 0) {
                posKey ^= pieceKeys[bbIdx][sq];
            }
        }
    }

    posKey ^= castleKeys[castleRightsMask(*this)];
    if (turn == BLACK) posKey ^= sideKey;
    if (enPassantSq != -1) posKey ^= enPassantKeys[enPassantSq % 8];

    return posKey;
}

bool Board::validate() const {
    uint64_t expectedPieces[12] = {};

    for (int sq = 0; sq < 64; sq++) {
        int piece = squares[sq];
        if (piece == EMPTY) continue;

        int bbIdx = pieceToBbIndex(piece);
        if (bbIdx < 0) return false;

        expectedPieces[bbIdx] |= (1ULL << sq);
    }

    for (int i = 0; i < 12; i++) {
        if (pieces[i] != expectedPieces[i]) return false;
    }

    uint64_t expectedWhite = expectedPieces[WP] | expectedPieces[WN] | expectedPieces[WB] |
                             expectedPieces[WR] | expectedPieces[WQ] | expectedPieces[WK];
    uint64_t expectedBlack = expectedPieces[BP] | expectedPieces[BN] | expectedPieces[BB] |
                             expectedPieces[BR] | expectedPieces[BQ] | expectedPieces[BK];
    uint64_t expectedAll = expectedWhite | expectedBlack;

    if (whiteOcc != expectedWhite) return false;
    if (blackOcc != expectedBlack) return false;
    if (allOcc != expectedAll) return false;
    if (whiteOcc & blackOcc) return false;

    return true;
}

bool Board::isThreefold() const {
    if (hashHistory.empty()) return false;

    const uint64_t key = hashHistory.back();
    int matches = 0;

    for (int i = static_cast<int>(hashHistory.size()) - 1; i >= 0; i -= 2) {
        if (hashHistory[i] == key) {
            matches++;
            if (matches >= 3) return true;
        }
    }

    return false;
}

void Board::reset() {
    for (int i = 0; i < 64; i++) squares[i] = EMPTY;
    for (int i = 0; i < 12; i++) pieces[i] = 0ULL;

    turn = WHITE;
    enPassantSq = -1;
    w_kingside = w_queenside = b_kingside = b_queenside = true;
    whiteOcc = 0ULL;
    blackOcc = 0ULL;
    allOcc = 0ULL;
    currentHash = generateHash();
    hashHistory.clear();
}

StateInfo Board::makeMove(Move m) {
    StateInfo s = {
        squares[m.to],
        enPassantSq,
        w_kingside,
        w_queenside,
        b_kingside,
        b_queenside,
        currentHash
    };
    if (m.flags == EN_PASSANT) {
        s.capturedPiece = (turn == WHITE) ? B_PAWN : W_PAWN;
    }

    const int oldCastleMask = castleRightsMask(*this);
    const int oldEpSq = enPassantSq;
    uint64_t newHash = currentHash;

    const uint64_t fromMask = 1ULL << m.from;
    const uint64_t toMask = 1ULL << m.to;

    const int original = m.from;
    const int piece = squares[m.from];
    const int movingInd = pieceToBbIndex(piece);
    assert(movingInd >= 0);
    if (movingInd < 0) return s;

    const int promoPiece = promotionPieceForMove(turn, m.flags);
    const int movedToPiece = (promoPiece != EMPTY) ? promoPiece : piece;
    const int movedToIdx = pieceToBbIndex(movedToPiece);

    if (oldEpSq != -1) newHash ^= enPassantKeys[oldEpSq % 8];
    newHash ^= castleKeys[oldCastleMask];
    newHash ^= sideKey;

    newHash ^= pieceKeys[movingInd][m.from];
    if (movedToIdx >= 0) {
        newHash ^= pieceKeys[movedToIdx][m.to];
    }

    if (s.capturedPiece != EMPTY) {
        const int capSq = (m.flags == EN_PASSANT) ? ((turn == WHITE) ? (m.to + 8) : (m.to - 8)) : m.to;
        const int capIdx = pieceToBbIndex(s.capturedPiece);
        if (capIdx >= 0) {
            newHash ^= pieceKeys[capIdx][capSq];
        }
    }

    if (m.flags == CASTLE_KING) {
        if (turn == WHITE) {
            newHash ^= pieceKeys[WR][63];
            newHash ^= pieceKeys[WR][61];
        } else {
            newHash ^= pieceKeys[BR][7];
            newHash ^= pieceKeys[BR][5];
        }
    } else if (m.flags == CASTLE_QUEEN) {
        if (turn == WHITE) {
            newHash ^= pieceKeys[WR][56];
            newHash ^= pieceKeys[WR][59];
        } else {
            newHash ^= pieceKeys[BR][0];
            newHash ^= pieceKeys[BR][3];
        }
    }
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

    if (s.capturedPiece == W_ROOK) {
        if (m.to == 63) w_kingside = false;
        if (m.to == 56) w_queenside = false;
    } else if (s.capturedPiece == B_ROOK) {
        if (m.to == 7) b_kingside = false;
        if (m.to == 0) b_queenside = false;
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

            uint64_t rookFromMask = 1ULL << 63;
            uint64_t rookToMask = 1ULL << 61;

            pieces[WR] &= ~rookFromMask;
            pieces[WR] |= rookToMask;
        } else { 
            squares[5] = B_ROOK; 
            squares[7] = EMPTY; 

            uint64_t rookFromMask = 1ULL << 7;
            uint64_t rookToMask = 1ULL << 5;

            pieces[BR] &= ~rookFromMask;
            pieces[BR] |= rookToMask;
        }
    }
    else if (m.flags == CASTLE_QUEEN) {
        if (turn == WHITE) { 
            squares[59] = W_ROOK; 
            squares[56] = EMPTY; 

            uint64_t rookFromMask = 1ULL << 56;
            uint64_t rookToMask = 1ULL << 59;

            pieces[WR] &= ~rookFromMask;
            pieces[WR] |= rookToMask;
        } else { 
            squares[3] = B_ROOK; 
            squares[0] = EMPTY; 
            
            uint64_t rookFromMask = 1ULL << 0;
            uint64_t rookToMask = 1ULL << 3;

            pieces[BR] &= ~rookFromMask;
            pieces[BR] |= rookToMask;
        }
    }

    if (m.flags != DOUBLE_PAWN_PUSH) enPassantSq = -1;

    newHash ^= castleKeys[castleRightsMask(*this)];
    if (enPassantSq != -1) {
        newHash ^= enPassantKeys[enPassantSq % 8];
    }

    turn = (turn == WHITE) ? BLACK : WHITE; 

    whiteOcc = pieces[WP] | pieces[WN] | pieces[WB] | pieces[WR] | pieces[WQ] | pieces[WK];
    blackOcc = pieces[BP] | pieces[BN] | pieces[BB] | pieces[BR] | pieces[BQ] | pieces[BK];
    allOcc = whiteOcc | blackOcc;
    currentHash = newHash;
    hashHistory.push_back(currentHash);

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
            uint64_t rookFromMask = 1ULL << 61;
            uint64_t rookToMask = 1ULL << 63;

            pieces[WK] &= ~kingFromMask;
            pieces[WK] |= kingToMask;
            pieces[WR] &= ~rookFromMask;
            pieces[WR] |= rookToMask;
        } else {
            squares[4] = B_KING;
            squares[6] = EMPTY;
            squares[7] = B_ROOK; 
            squares[5] = EMPTY;

            uint64_t kingFromMask = 1ULL << (6);
            uint64_t kingToMask = 1ULL << (4);
            uint64_t rookFromMask = 1ULL << 5;
            uint64_t rookToMask = 1ULL << 7;

            pieces[BK] &= ~kingFromMask;
            pieces[BK] |= kingToMask;
            pieces[BR] &= ~rookFromMask;
            pieces[BR] |= rookToMask;
        }
    } else if (m.flags == CASTLE_QUEEN) {
        if (turn == WHITE) {
            squares[60] = W_KING;
            squares[58] = EMPTY;
            squares[56] = W_ROOK; 
            squares[59] = EMPTY;

            uint64_t kingFromMask = 1ULL << (58);
            uint64_t kingToMask = 1ULL << (60);
            uint64_t rookFromMask = 1ULL << 59;
            uint64_t rookToMask = 1ULL << 56;

            pieces[WK] &= ~kingFromMask;
            pieces[WK] |= kingToMask;
            pieces[WR] &= ~rookFromMask;
            pieces[WR] |= rookToMask;
        } else {
            squares[4] = B_KING;
            squares[2] = EMPTY;
            squares[0] = B_ROOK; 
            squares[3] = EMPTY;

            uint64_t kingFromMask = 1ULL << (2);
            uint64_t kingToMask = 1ULL << (4);
            uint64_t rookFromMask = 1ULL << 3;
            uint64_t rookToMask = 1ULL << 0;

            pieces[BK] &= ~kingFromMask;
            pieces[BK] |= kingToMask;
            pieces[BR] &= ~rookFromMask;
            pieces[BR] |= rookToMask;
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
            if (s.capturedPiece != EMPTY) {
                pieces[pieceToBbIndex(s.capturedPiece)] |= toMask;
            }
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
            if (s.capturedPiece != EMPTY) {
                pieces[pieceToBbIndex(s.capturedPiece)] |= toMask;
            }
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
    if (!hashHistory.empty()) {
        hashHistory.pop_back();
    }
    currentHash = s.previousHash;
    if (hashHistory.empty()) {
        hashHistory.push_back(currentHash);
    }
}

void Board::resetBb() {
    for (int i = 0; i < 12; i++) {
        pieces[i] = 0ULL;
    }

    whiteOcc = 0ULL;
    blackOcc = 0ULL;
    allOcc = 0ULL;
    currentHash = 0ULL;
    hashHistory.clear();
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

    currentHash = generateHash();
    hashHistory.clear();
    hashHistory.push_back(currentHash);
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

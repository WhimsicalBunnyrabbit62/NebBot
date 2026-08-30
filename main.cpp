#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include "board.h"
#include "moveGen.h"
#include "eval.h"
#include "nnue.h"
#include "search.h"

std::string toAlgebraic(int sq) {
    char file = 'a' + (sq % 8);
    char rank = '8' - (sq / 8); 
    return std::string(1, file) + std::string(1, rank);
}

void init() {
    Board::initAll();
    moveGen::initAll();
    eval::initAll();
    search::initAll();
    if (eval::useNNUE) { nnue::initNNUE(); nnue::buildFeatureTable(); }
}

int main() {
    init();

    Board board;
    std::string line;
    srand(time(0));

    while (std::getline(std::cin, line)) {
        if (line == "uci") {
            std::cout << "id name Bazingine" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (line == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (line.find("position fen ") == 0) {
            std::string fen = line.substr(13);
            board.loadFromFen(fen);
            nnue::refreshAccumulator(board);
        } else if (line.find("go") == 0) {
            MoveList moves;
            moveGen::generateMoves(board, moves);

            int evalWhite = eval::evaluate(board);
            int evalSideToMove = evalWhite * board.turn;
            std::cout << "Evaluation (white): " << evalWhite
                      << " | Evaluation (side-to-move): " << evalSideToMove
                      << std::endl;

            
            int maxTimeMs = 500;
            Move bestMove = search::startSearch(board, maxTimeMs);

            if (bestMove.flags == CHECKMATE_ENGINE) {
                std::cout << "bestmove checkmate" << std::endl;
            } else if (bestMove.flags == STALEMATE) {
                std::cout << "bestmove stalemate" << std::endl;
            } else {
                std::cout << "bestmove " << toAlgebraic(bestMove.from) << toAlgebraic(bestMove.to); 
                std::cout << "\n-----------------------------------------------------------newl" << std::endl;
            }

        } else if (line.rfind("perft", 0) == 0) {
            int depth = 1;
            if (line.size() > 6) depth = std::stoi(line.substr(6));
        
            std::cout << search::timedPerft(depth, board) << std::endl;
        } else if (line == "quit") break;
        else if (line == "checkmated") {
            MoveList moves;
            moveGen::generateMoves(board, moves);
            
            if (moves.empty()) {
                int kingSq = (board.turn == WHITE) ? moveGen::get_lsb(board.pieces[WK]) : moveGen::get_lsb(board.pieces[BK]);
                if (kingSq != -1 && moveGen::isSquareAttacked(board, kingSq)) {
                    std::cout << "result checkmate" << std::endl;
                } else {
                    std::cout << "result stalemate" << std::endl;
                }
            } else {
                std::cout << "result none" << std::endl;
            }
        }
    }

    return 0;
}

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include "board.h"
#include "moveGen.h"
#include "eval.h"
#include "search.h"

std::string toAlgebraic(int sq) {
    char file = 'a' + (sq % 8);
    char rank = '8' - (sq / 8);
    return std::string(1, file) + std::string(1, rank);
}

int main() {
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
        } else if (line.find("go") == 0) {
            MoveList moves;
            moveGen::generateMoves(board, moves);

            std::cout << "Evaluation: " << eval::evaluate(board) << std::endl;

            if (!moves.empty()) {
                int maxTimeMs = 5000;
                Move bestMove = search::startSearch(board, maxTimeMs);

                std::cout << "bestmove " << toAlgebraic(bestMove.from) << toAlgebraic(bestMove.to); 
                std::cout << "\n-----------------------------------------------------------newl" << std::endl;
            } else {
                std::cout << "bestmove none" << std::endl;
            }
        } else if (line.rfind("perft", 0) == 0) {
            int depth = 1;
            if (line.size() > 6) depth = std::stoi(line.substr(6));
        
            std::cout << search::timedPerft(depth, board) << std::endl;
        } else if (line == "quit") break;
    }

    return 0;
}

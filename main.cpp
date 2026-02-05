#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include "board.h"
#include "moveGen.h"

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
            std::vector<Move> moves = moveGen::generateMoves(board);

            if (!moves.empty()) {
                Move m = moves[rand() % moves.size()];

                std::cout << "bestmove " << toAlgebraic(m.from) << toAlgebraic(m.to);

                if (m.flags == PROMOTION_QUEEN) std::cout << "q";
                std::cout << std::endl;
            } else {
                std::cout << "bestmove none" << std::endl;
            }
        } else if (line == "quit") break;
    }

    return 0;
}
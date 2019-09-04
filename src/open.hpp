#ifndef SFINDER_EXP_OPEN_HPP
#define SFINDER_EXP_OPEN_HPP

#include <fstream>

#include "types.hpp"

namespace sfexp {
    void openIndexes(const std::string &file, std::vector<PieceIndex> &indexes) {
        std::ifstream fin(file, std::ios::in | std::ios::binary);
        if (!fin) {
            std::cout << "Failed to open file" << std::endl;
            return;
        }

        while (true) {
            sfexp::PieceIndex pieceIndex{};
            fin.read(reinterpret_cast<char *>(&pieceIndex), sizeof(PieceIndex));
            if (fin.eof()) {
                break;
            }
            indexes.push_back(pieceIndex);
        }
    }

    void openSolutions(const std::string &file, std::vector<Solution> &solutions) {
        std::ifstream fin(file, std::ios::in | std::ios::binary);
        if (!fin) {
            std::cout << "Failed to open file" << std::endl;
            return;
        }

        while (true) {
            Solution solution{};
            fin.read(reinterpret_cast<char *>(&solution), sizeof(Solution));
            if (fin.eof()) {
                break;
            }
            solutions.push_back(solution);
        }
    }
}

#endif //SFINDER_EXP_OPEN_HPP

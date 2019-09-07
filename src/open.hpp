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

    void openSolutions(const std::string &file, std::vector<std::vector<uint16_t>> &solutions) {
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

            std::vector<uint16_t> vector(solution.cbegin(), solution.cend());
            solutions.push_back(vector);
        }
    }

    void openMarker(const std::string &file, std::vector<sfinder::Marker::FlagType> &flags) {
        std::ifstream fin(file, std::ios::in | std::ios::binary);
        if (!fin) {
            std::cout << "Failed to open file" << std::endl;
            return;
        }

        while (true) {
            sfinder::Marker::FlagType flag{};
            fin.read(reinterpret_cast<char *>(&flag), sizeof(sfinder::Marker::FlagType));
            if (fin.eof()) {
                break;
            }
            flags.push_back(flag);
        }
    }
}

#endif //SFINDER_EXP_OPEN_HPP

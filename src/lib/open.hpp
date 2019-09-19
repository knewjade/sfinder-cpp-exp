#ifndef SFINDER_EXP_LIB_OPEN_HPP
#define SFINDER_EXP_LIB_OPEN_HPP

#include <fstream>
#include <stdexcept>

#include "types.hpp"

#include "sfinder/marker.hpp"

namespace sfexp {
    void openIndexes(const std::string &file, std::vector<PieceIndex> &indexes) {
        std::ifstream fin(file, std::ios::in | std::ios::binary);
        if (!fin) {
            throw std::runtime_error("Failed to open file: " + file);
        }

        while (true) {
            sfexp::PieceIndex pieceIndex{};
            fin.read(reinterpret_cast<char *>(&pieceIndex), sizeof(PieceIndex));
            if (fin.eof()) {
                break;
            }
            indexes.push_back(pieceIndex);
        }

        fin.close();
    }

    void openSolutions(const std::string &file, std::vector<std::vector<uint16_t>> &solutions) {
        std::ifstream fin(file, std::ios::in | std::ios::binary);
        if (!fin) {
            throw std::runtime_error("Failed to open file: " + file);
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

        fin.close();
    }

    void openBits(const std::string &file, std::vector<sfinder::Bits::FlagType> &flags) {
        std::ifstream fin(file, std::ios::in | std::ios::binary);
        if (!fin) {
            throw std::runtime_error("Failed to open file: " + file);
        }

        while (true) {
            sfinder::Bits::FlagType flag{};
            fin.read(reinterpret_cast<char *>(&flag), sizeof(sfinder::Bits::FlagType));
            if (fin.eof()) {
                break;
            }
            flags.push_back(flag);
        }

        fin.close();
    }

    void openBitsFixedSize(const std::string &file, std::vector<sfinder::Bits::FlagType> &flags, int size) {
        assert(0 < size);

        std::ifstream fin(file, std::ios::in | std::ios::binary);
        if (!fin) {
            throw std::runtime_error("Failed to open file: " + file);
        }

        flags.resize(size);

        fin.read(reinterpret_cast<char *>(&flags[0]), sizeof(sfinder::Bits::FlagType) * size);

        fin.close();
    }

    bool fileExists(const std::string &file) {
        std::ifstream fin(file, std::ios::in | std::ios::binary);
        bool result = fin.is_open();
        fin.close();
        return result;
    }
}

#endif //SFINDER_EXP_LIB_OPEN_HPP

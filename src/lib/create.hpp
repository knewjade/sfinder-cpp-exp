#ifndef SFINDER_EXP_LIB_CREATE_HPP
#define SFINDER_EXP_LIB_CREATE_HPP

#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

#include "core/factory.hpp"
#include "sfinder/marker.hpp"

#include "types.hpp"
#include "parse.hpp"

namespace sfexp {
    PieceIndex createPieceIndex(core::Factory &factory, std::string &str) {
        std::stringstream ss{str};
        std::string buf;

        int index;
        if (std::getline(ss, buf, ',')) {
            index = std::stoi(buf);
        } else {
            throw std::runtime_error("Invalid index");
        }

        core::PieceType piece;
        if (std::getline(ss, buf, ',')) {
            piece = sfexp::parseToPiece(buf[0]);
        } else {
            throw std::runtime_error("Invalid piece");
        }

        core::RotateType rotate;
        if (std::getline(ss, buf, ',')) {
            rotate = sfexp::parseToRotate(buf[0]);
        } else {
            throw std::runtime_error("Invalid rotate");
        }

        int x;
        if (std::getline(ss, buf, ',')) {
            x = std::stoi(buf);
        } else {
            throw std::runtime_error("Invalid x");
        }

        int lowerY;
        if (std::getline(ss, buf, ',')) {
            lowerY = std::stoi(buf);
        } else {
            throw std::runtime_error("Invalid lower y");
        }

        auto &block = factory.get(piece, rotate);
        int y = lowerY - block.minY;

        std::getline(ss, buf, ',');  // skip

        uint64_t deletedLine;
        if (std::getline(ss, buf, ',')) {
            deletedLine = sfexp::parseToMask(buf);
        } else {
            throw std::runtime_error("Invalid deleted line");
        }

        return PieceIndex{
                index,
                piece,
                rotate,
                x,
                y,
                deletedLine,
        };
    }

    void createIndexes(const std::string &inputFile, const std::string &outputFile) {
        auto pieceIndexes = std::vector<PieceIndex>{};

        {
            // Open file
            std::ifstream ifs(inputFile);
            if (ifs.fail()) {
                std::cout << "Failed to open file" << std::endl;
                throw std::runtime_error("Failed to open file: " + inputFile);
            }

            // Parse
            auto factory = core::Factory::create();

            std::string str;
            while (getline(ifs, str)) {
                auto item = createPieceIndex(factory, str);
                pieceIndexes.push_back(item);
            }

            std::cout << pieceIndexes.size() << std::endl;

            ifs.close();
        }

        {
            // Create binary
            std::ofstream out(outputFile, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!out) {
                std::cout << "Failed to open file" << std::endl;
                throw std::runtime_error("Failed to open file: " + outputFile);
            }

            for (auto &index : pieceIndexes) {
                out.write(reinterpret_cast<char *>(&index), sizeof(PieceIndex));
            }

            out.close();
        }
    }

    void createSolutions(const std::string &fileName, const std::string &outputFile) {
        auto solutions = std::vector<Solution>{};

        {
            // Open file
            std::ifstream ifs(fileName);
            if (ifs.fail()) {
                std::cout << "Failed to open file: " << fileName << std::endl;
                throw std::runtime_error("Failed to open file: " + fileName);
            }

            // Parse
            std::string str;
            while (getline(ifs, str)) {
                Solution solution{};

                std::stringstream ss{str};
                std::string buf;

                auto index = 0;
                while (std::getline(ss, buf, ',')) {
                    solution[index] = std::stoi(buf);
                    index += 1;
                }

                assert(index == 10);

                std::sort(solution.begin(), solution.end());

                solutions.push_back(solution);
            }

            std::cout << solutions.size() << std::endl;

            ifs.close();
        }

        {
            // Create binary
            std::ofstream out(outputFile, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!out) {
                std::cout << "Failed to open file" << std::endl;
                throw std::runtime_error("Failed to open file: " + outputFile);
            }

            for (auto &solution : solutions) {
                out.write(reinterpret_cast<char *>(&solution), sizeof(Solution));
            }

            out.close();
        }
    }

    void createBits(const std::string &fileName, const sfinder::Bits &bits) {
        auto &flags = bits.flags();

        // Create binary
        std::ofstream out(fileName, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out) {
            std::cout << "Failed to open file" << std::endl;
            throw std::runtime_error("Failed to open file: " + fileName);
        }

        out.write(reinterpret_cast<const char *>(&flags[0]), sizeof(sfinder::Bits::FlagType) * flags.size());

        out.close();
    }
}

#endif //SFINDER_EXP_LIB_CREATE_HPP

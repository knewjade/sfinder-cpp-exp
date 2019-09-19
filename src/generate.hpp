#ifndef SFINDER_EXP_GENERATE_HPP
#define SFINDER_EXP_GENERATE_HPP

#include <vector>
#include <set>

#include "core/pieces.hpp"
#include "lib/create.hpp"

template<int N, class Operator>
void generate(
        std::vector<core::PieceType> &pieces, Operator op
) {
    if constexpr (N == 0) {
        assert(!pieces.empty());
        op(pieces);
    } else {
        for (const auto &pieceType : core::kAllPieceType) {
            pieces.push_back(pieceType);
            generate<N - 1, Operator>(pieces, op);
            pieces.pop_back();
        }
    }
}

// used is sorted by TILJSZO
template<class UnaryOperation>
void parse(
        const std::vector<sfexp::PieceIndex> &pieceIndexSolutions,
        const std::vector<core::PieceType> &used,
        int usedIndex,
        int startIndex,
        std::vector<int> &selected,
        UnaryOperation op
) {
    assert(0 <= usedIndex && usedIndex < used.size());

    auto pieceType = used[usedIndex];

    for (int index = startIndex; index < pieceIndexSolutions.size(); ++index) {
        auto &pieceIndex = pieceIndexSolutions[index];

        if (pieceIndex.piece == pieceType) {
            selected.emplace_back(index);
            if (used.size() == selected.size()) {
                op();
            } else if (usedIndex + 1 < used.size()) {
                parse(pieceIndexSolutions, used, usedIndex + 1, index + 1, selected, op);
            }
            selected.pop_back();
        }
    }
}

void generate() {
    const int depth = 1;

    // Load
    auto indexes = std::vector<sfexp::PieceIndex>{};
    sfexp::openIndexes("../../bin/index.bin", indexes);
    std::cout << "indexes: " << indexes.size() << std::endl;

    auto solutions = std::vector<sfexp::SolutionVector>{};
    sfexp::openSolutions("../../bin/indexed_solutions_10x4_SRS7BAG.bin", solutions);
    std::cout << "solutions: " << solutions.size() << std::endl;

    // Parse
    auto pieceIndexSolutions = std::vector<std::vector<sfexp::PieceIndex>>{};
    std::transform(
            solutions.cbegin(), solutions.cend(), std::back_inserter(pieceIndexSolutions),
            [&](sfexp::SolutionVector solution) -> std::vector<sfexp::PieceIndex> {
                auto pieceIndexes = std::vector<sfexp::PieceIndex>{};
                std::transform(
                        solution.cbegin(), solution.cend(), std::back_inserter(pieceIndexes),
                        [&](int index) -> sfexp::PieceIndex {
                            return indexes[index];
                        }
                );
                return pieceIndexes;
            }
    );
    std::cout << "pieceIndexSolutions: " << pieceIndexSolutions.size() << std::endl;

    auto pieceCounters = std::vector<core::PieceCounter>{};
    std::transform(
            pieceIndexSolutions.cbegin(), pieceIndexSolutions.cend(), std::back_inserter(pieceCounters),
            [&](std::vector<sfexp::PieceIndex> solution) -> core::PieceCounter {
                auto used = std::vector<core::PieceType>{};
                std::transform(
                        solution.cbegin(), solution.cend(), std::back_inserter(used),
                        [&](sfexp::PieceIndex pieceIndex) -> core::PieceType {
                            return pieceIndex.piece;
                        }
                );
                return core::PieceCounter::create(used.cbegin(), used.cend());
            }
    );
    std::cout << "pieceCounters: " << pieceCounters.size() << std::endl;

    auto rest = 10 - depth;

    auto factory = core::Factory::create();
    auto pieces = std::vector<core::PieceType>{};
    generate<depth>(pieces, [&](const std::vector<core::PieceType> &pieces) -> void {
        std::string name{};
        for (const auto &piece : pieces) {
            name += factory.get(piece).name;
        }
        std::cout << name;

        auto used = core::PieceCounter::create(pieces);
        auto usedVector = used.vector();
        std::sort(usedVector.begin(), usedVector.end());

        int counter = 0;

        std::set<std::vector<uint16_t>> indexSet{};
        for (int index = 0; index < pieceCounters.size(); ++index) {
            auto &pieceCounter = pieceCounters[index];

            if (pieceCounter.containsAll(used)) {
                auto &solution = solutions[index];
                auto &pieceIndexSolution = pieceIndexSolutions[index];

                auto selected = std::vector<int>{};
                parse(pieceIndexSolution, usedVector, 0, 0, selected, [&]() -> void {
                    auto v = std::vector<uint16_t>{};
                    std::transform(
                            selected.cbegin(), selected.cend(), std::back_inserter(v),
                            [&](int index) -> uint16_t {
                                return solution[index];
                            }
                    );
                    indexSet.emplace(v);
                    counter++;
                });
            }
        }

        std::cout << " " << counter << " " << indexSet.size() << std::endl;

        // Create new bits
        int size = static_cast<int>(std::pow(7, rest));
        int length = sfinder::Bits::calculateLength(size);

        auto vector = std::vector<sfinder::Bits::FlagType>(length);

        for (const auto &indexes : indexSet) {
            std::string file = "../../bin" + std::to_string(depth) + "/";
            for (const auto &index : indexes) {
                file += std::to_string(index) + "_";
            }
            file += "output.bin";

            if (!sfexp::fileExists(file)) {
                continue;
            }

            // Open file
            auto vector2 = std::vector<sfinder::Bits::FlagType>{};
            sfexp::openBitsFixedSize(file, vector2, length);

            assert(vector.size() == vector2.size());

            for (int i = 0; i < vector2.size(); ++i) {
                vector[i] |= vector2[i];
            }
        }

        std::string outputFile = "../../bin" + std::to_string(depth) + "-step2/" + name + "_output.bin";
        auto bits = sfinder::Bits(vector);
        sfexp::createBits(outputFile, bits);
    });
}

#endif //SFINDER_EXP_GENERATE_HPP
#ifndef SFINDER_EXP_REDUCE_HPP
#define SFINDER_EXP_REDUCE_HPP

#include <stdexcept>
#include <vector>
#include <thread>

#include "core/pieces.hpp"
#include "sfinder/permutations.hpp"
#include "sfinder/marker.hpp"
#include "sfinder/lookup.hpp"

#include "lib/open.hpp"
#include "lib/pieces.hpp"

struct PiecePopPair {
    const core::PieceCounter counter;
    const int pop;
};

auto kEmptyPairs = std::vector<PiecePopPair>{};

template<class InputIterator>
std::vector<PiecePopPair> reduce(
        const std::vector<PiecePopPair> &pairs,
        InputIterator begin,
        typename core::is_input_iterator<InputIterator, core::PieceType>::type end
) {
    auto newPairs = std::vector<PiecePopPair>{};

    auto usedCounter = core::PieceCounter::create(begin, end);
    auto remainder = core::PieceCounter::create();

    for (int index = 0; index < pairs.size(); index++) {
        const auto pair = pairs[index];

        auto pop = pair.pop;

        auto nextRemainder = remainder + pair.counter;
        auto newPieceCounter = core::PieceCounter(pair.counter);

        for (const auto &pieceType : usedCounter.vector()) {
            const auto counter = core::PieceCounter::create(pieceType);
            if (nextRemainder.containsAll(counter)) {
                nextRemainder = nextRemainder - counter;
                usedCounter = usedCounter - counter;
                newPieceCounter = newPieceCounter - counter;
                pop -= 1;
            }
        }

        if (!newPieceCounter.empty()) {
            newPairs.push_back(PiecePopPair{newPieceCounter, pop});
        }

        if (usedCounter.empty()) {
            for (int j = index + 1; j < pairs.size(); j++) {
                newPairs.push_back(pairs[j]);
            }
            return newPairs;
        }

        if (1 < nextRemainder.size()) {
            // Error
            return kEmptyPairs;
        }
    }

    return newPairs;
}

std::vector<sfinder::Permutation> createPermutationVector(const std::vector<PiecePopPair> &pairs) {
    auto permutations = std::vector<sfinder::Permutation>{};
    std::transform(pairs.cbegin(), pairs.cend(), std::back_inserter(permutations), [](PiecePopPair pair) {
        auto vector = pair.counter.vector();
        return sfinder::Permutation::create(vector, pair.pop);
    });
    return permutations;
}

template<int Depth>
void reduce2(
        const core::Factory &factory, const std::vector<sfexp::PieceIndex> &indexes,
        const std::vector<PiecePopPair> &pairs, const sfinder::ForwardOrderLookUp &forwardOrderLookup,
        std::string dir, std::vector<int> &selected, const core::Field &field
) {
    static_assert(0 <= Depth);

    if constexpr (0 < Depth) {
        auto filledKey = field.filledKey();

        for (int i = 0; i < indexes.size(); ++i) {
            auto &pieceIndex = indexes.at(i);

            if ((pieceIndex.deletedLine & filledKey) != pieceIndex.deletedLine) {
                continue;
            }

            auto blocks = factory.get(pieceIndex.piece, pieceIndex.rotate);

            auto mino = core::Field{};
            mino.put(blocks, pieceIndex.x, pieceIndex.y);
            mino.insertWhiteLineWithKey(pieceIndex.deletedLine);

            if (!field.canMerge(mino)) {
                continue;
            }

            auto freeze = core::Field(field);
            freeze.merge(mino);

            selected.push_back(i);
            reduce2<Depth - 1>(factory, indexes, pairs, forwardOrderLookup, dir, selected, freeze);
            selected.pop_back();
        }
    } else {
        auto v = std::vector(selected.cbegin(), selected.cend());
        std::sort(v.begin(), v.end());

        std::string name{};
        for (const auto &index : v) {
            name += std::to_string(index) + "_";
        }
        name += "output.bin";

        // Checks file
        std::string inputFileName = "../../bin" + std::to_string(selected.size()) + "/" + name;

        std::cout << inputFileName << std::endl;

        if (!sfexp::fileExists(inputFileName)) {
            return;
        }

        std::string output = dir + "/" + name;

        if (sfexp::fileExists(output)) {
            return;
        }

        auto used = std::vector<core::PieceType>{};
        for (const auto &index : selected) {
            auto &pieceIndex = indexes.at(index);
            used.emplace_back(pieceIndex.piece);
        }

        int toDepth = 10 - selected.size();
        int size = static_cast<int>(std::pow(7, toDepth));
        int length = sfinder::Bits::calculateLength(size);

        // Open file
        auto vector = std::vector<sfinder::Bits::FlagType>{};
        sfexp::openBitsFixedSize(inputFileName, vector, length);
        auto bits = sfinder::Bits(vector);

        auto reducedPairs = reduce(pairs, used.cbegin(), used.cend());
        if (reducedPairs.empty()) {
            return;
        }

        auto permutationVector = createPermutationVector(reducedPairs);
        auto permutations = sfinder::Permutations::create(permutationVector);

        assert(10 - selected.size() == permutations.depth() || 10 - selected.size() + 1 == permutations.depth());

        auto bits2 = sfinder::Bits::create(permutations.size());

        assert(forwardOrderLookup.toDepth == toDepth);

        // same size
        for (int i = 0; i < permutations.size(); ++i) {
            auto pieces = permutations.pieces(i);
            assert(forwardOrderLookup.fromDepth == pieces.size());

            bool success = false;
            forwardOrderLookup.foreachUntilTrue(pieces, [&](std::vector<core::PieceType> &pieces2) -> bool {
                auto value = sfexp::toValue(pieces2);
                if (bits.checks(value)) {
                    success = true;
                    return true;
                }
                return false;
            });

            bits2.set(i, success);
        }

        // Output
        sfexp::createBits(output, bits2);
    }
}

template<int MaxDepth>
void reduce2(
        const core::Factory &factory, const std::vector<sfexp::PieceIndex> &indexes,
        const std::vector<PiecePopPair> &pairs, const sfinder::ForwardOrderLookUp &forwardOrderLookup,
        std::string dir
) {
    auto field = core::Field{};
    auto selected = std::vector<int>{};
    std::cout << dir << std::endl;
    reduce2<MaxDepth>(factory, indexes, pairs, forwardOrderLookup, dir, selected, field);
}

template<int MaxDepth>
void reduceE() {
    auto all = core::PieceCounter::create(core::kAllPieceType.cbegin(), core::kAllPieceType.cend());

    // Config
    const int toDepth = 10 - MaxDepth;

    // sum == `toDepth`
    auto pairs1 = std::vector{
//            PiecePopPair{core::PieceCounter::create(core::PieceType::T), 1},
            PiecePopPair{all, 7},
            PiecePopPair{all, 3},
    };

    // sum == `toDepth + 1`
    auto pairs2 = std::vector{
//            PiecePopPair{core::PieceCounter::create(core::PieceType::T), 1},
            PiecePopPair{all, 7},
            PiecePopPair{all, 4},
    };

    std::string prefix = "../../../cy1/bin" + std::to_string(MaxDepth) + "-E";

    // Open indexes
    auto indexes = std::vector<sfexp::PieceIndex>{};
    sfexp::openIndexes("../../bin/index.bin", indexes);
    std::cout << indexes.size() << std::endl;

    auto factory = core::Factory::create();

    auto forwardOrderLookup1 = sfinder::ForwardOrderLookUp::create(toDepth, toDepth, false, false);
    reduce2<MaxDepth>(factory, indexes, pairs1, forwardOrderLookup1, prefix + "h" + std::to_string(toDepth));

    auto forwardOrderLookup1f = sfinder::ForwardOrderLookUp::create(toDepth, toDepth, false, true);
    reduce2<MaxDepth>(factory, indexes, pairs1, forwardOrderLookup1, prefix + "h" + std::to_string(toDepth) + "f");

    auto forwardOrderLookup2 = sfinder::ForwardOrderLookUp::create(toDepth, toDepth + 1, false, false);
    reduce2<MaxDepth>(factory, indexes, pairs2, forwardOrderLookup2, prefix + "h" + std::to_string(toDepth + 1));

    auto forwardOrderLookup2f = sfinder::ForwardOrderLookUp::create(toDepth, toDepth + 1, false, true);
    reduce2<MaxDepth>(factory, indexes, pairs2, forwardOrderLookup2f, prefix + "h" + std::to_string(toDepth + 1) + "f");
}

template<int MaxDepth, core::PieceType Hold>
void reduceHold() {
    auto all = core::PieceCounter::create(core::kAllPieceType.cbegin(), core::kAllPieceType.cend());

    // Config
    const int toDepth = 10 - MaxDepth;

    // Open indexes
    auto indexes = std::vector<sfexp::PieceIndex>{};
    sfexp::openIndexes("../../bin/index.bin", indexes);
    std::cout << indexes.size() << std::endl;

    auto factory = core::Factory::create();

    // sum == `toDepth`
    auto pairs1 = std::vector{
            PiecePopPair{core::PieceCounter::create(Hold), 1},
            PiecePopPair{all, 7},
            PiecePopPair{all, 2},
    };

    // sum == `toDepth + 1`
    auto pairs2 = std::vector{
            PiecePopPair{core::PieceCounter::create(Hold), 1},
            PiecePopPair{all, 7},
            PiecePopPair{all, 3},
    };

    std::string prefix = "../../../cy1/bin" + std::to_string(MaxDepth) + "-" + factory.get(Hold).name;

    auto forwardOrderLookup1 = sfinder::ForwardOrderLookUp::create(toDepth, toDepth, false, false);
    reduce2<MaxDepth>(factory, indexes, pairs1, forwardOrderLookup1, prefix + "h" + std::to_string(toDepth));

    auto forwardOrderLookup1f = sfinder::ForwardOrderLookUp::create(toDepth, toDepth, false, true);
    reduce2<MaxDepth>(factory, indexes, pairs1, forwardOrderLookup1, prefix + "h" + std::to_string(toDepth) + "f");

    auto forwardOrderLookup2 = sfinder::ForwardOrderLookUp::create(toDepth, toDepth + 1, false, false);
    reduce2<MaxDepth>(factory, indexes, pairs2, forwardOrderLookup2, prefix + "h" + std::to_string(toDepth + 1));

    auto forwardOrderLookup2f = sfinder::ForwardOrderLookUp::create(toDepth, toDepth + 1, false, true);
    reduce2<MaxDepth>(
            factory, indexes, pairs2, forwardOrderLookup2f, prefix + "h" + std::to_string(toDepth + 1) + "f"
    );
}

void reduce() {
    const int depth = 1;

    auto threads = std::array{
            std::thread([]() {
                reduceE<depth>();
                reduceHold<depth, core::PieceType::T>();
            }),
            std::thread([]() {
                reduceHold<depth, core::PieceType::I>();
                reduceHold<depth, core::PieceType::L>();
                reduceHold<depth, core::PieceType::J>();
            }),
            std::thread([]() {
                reduceHold<depth, core::PieceType::S>();
                reduceHold<depth, core::PieceType::Z>();
                reduceHold<depth, core::PieceType::O>();
            }),
    };

    for (auto &thread : threads) {
        thread.join();
    }
}

#endif //SFINDER_EXP_REDUCE_HPP

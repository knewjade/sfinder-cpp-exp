#ifndef SFINDER_EXP_FIND_HPP
#define SFINDER_EXP_FIND_HPP

#include <thread>

#include "core/types.hpp"
#include "core/factory.hpp"
#include "core/field.hpp"
#include "core/moves.hpp"

#include "lib/open.hpp"
#include "lib/runner.hpp"

namespace {
    struct Shared {
        const std::vector<sfexp::PieceIndex> &indexes;
        bool aborted = false;
    };
}

void find(
        Shared &shared,
        const std::vector<sfexp::SolutionVector> &solutions,
        const core::Factory &factory,
        const core::Field &field,
        int maxDepth,
        const std::string &prefixFileName
) {
    auto output = prefixFileName + "output.bin";

    if (!sfexp::fileExists(output)) {
        return;
    }

    const int max = static_cast<int>(std::pow(7, maxDepth));
    auto bits = sfinder::Bits::create(max);

    // Runner
    auto moveGenerator = core::srs::MoveGenerator(factory);
    auto currentOperations = std::vector<core::PieceType>{};
    auto runner = sfexp::Runner(factory, field, bits, moveGenerator);

    // run
    for (const auto &solution : solutions) {
        assert(solution.size() == maxDepth);

        auto vector = std::vector<sfexp::PieceIndex>{};
        for (const auto &index : solution) {
            auto pieceIndex = shared.indexes[index];
            vector.push_back(pieceIndex);
        }

        runner.find(vector);
    }

    // Output
    sfexp::createBits(output, bits);
}

template<int Start, int End, bool FirstCall = false>
void find(
        Shared &shared,
        const std::vector<sfexp::SolutionVector> &solutions,
        int maxDepth,
        const std::string &prefixFileName,
        const core::Factory &factory,
        const core::Field &field,
        std::vector<int> &selected,
        int depth
) {
    if (shared.aborted) {
        return;
    }

    assert(!solutions.empty());

    if (depth == maxDepth) {
        auto v = std::vector<int>(selected.cbegin(), selected.cend());
        std::sort(v.begin(), v.end());

        auto s = prefixFileName;
        for (const auto &item : v) {
            s += std::to_string(item) + "_";
        }

        find(shared, solutions, factory, field, maxDepth, s);
        return;
    }

    auto filledKey = field.filledKey();

    int endIndex = 0 <= End ? End : shared.indexes.size();
    for (int index = Start; index < endIndex; ++index) {
        if constexpr (FirstCall) {
            std::cout << index << std::endl;
        }

        auto &pieceIndex = shared.indexes[index];

        if ((filledKey & pieceIndex.deletedLine) != pieceIndex.deletedLine) {
            continue;
        }

        auto mino = core::Field{};
        auto &blocks = factory.get(pieceIndex.piece, pieceIndex.rotate);
        mino.put(blocks, pieceIndex.x, pieceIndex.y);
        mino.insertWhiteLineWithKey(pieceIndex.deletedLine);

        if (!field.canMerge(mino)) {
            continue;
        }

        {
            auto freeze2 = core::Field{field};
            auto deletedKey = freeze2.clearLineReturnKey();
            auto slideY = core::bitCount(deletedKey & core::getUsingKeyBelowY(pieceIndex.y + blocks.minY));
            auto y = pieceIndex.y - slideY;
            if (!freeze2.canPut(blocks, pieceIndex.x, y) || !freeze2.isOnGround(blocks, pieceIndex.x, y)) {
                continue;
            }
        }

        auto freeze = core::Field{field};
        freeze.merge(mino);

        selected.push_back(index);

        // Filter solutions
        auto filteredSolutions = std::vector<sfexp::SolutionVector>{};
        for (auto solution : solutions) {
            if (!std::binary_search(solution.cbegin(), solution.cend(), index)) {
                continue;
            }

            auto itrNewEnd = std::remove_if(
                    solution.begin(), solution.end(), [&](int i) -> bool { return i == index; }
            );
            solution.erase(itrNewEnd, solution.end());
            assert(solution.size() == depth - 1);

            filteredSolutions.push_back(solution);
        }

        if (!filteredSolutions.empty()) {
            find<0, -1>(shared, filteredSolutions, maxDepth, prefixFileName, factory, freeze, selected, depth - 1);
        }

        selected.pop_back();
    }
}

template<int Start, int End>
void find(
        Shared &shared, const std::vector<sfexp::SolutionVector> &solutions, int maxDepth
) {
    auto factory = core::Factory::create();
    auto selected = std::vector<int>{};
    auto field = core::Field{};

    find<Start, End, true>(shared, solutions, maxDepth, "../../bin/", factory, field, selected, 10);
}

void find() {
    auto indexes = std::vector<sfexp::PieceIndex>{};
    sfexp::openIndexes("../../bin/index.bin", indexes);
    std::cout << indexes.size() << std::endl;

    auto solutions = std::vector<sfexp::SolutionVector>{};
    sfexp::openSolutions("../../bin/indexed_solutions_10x4_SRS7BAG.bin", solutions);
    std::cout << solutions.size() << std::endl;

    auto shared = Shared{indexes, false};

    // 10
    find<0, -1>(shared, solutions, 10);

    // 9
//    find<0, -1>(shared, solutions, 9);

    // 8
//    const int N = 7;
//    std::array<std::thread, N> threads;
//    threads[0] = std::thread([&]() { find<0, 100>(shared, solutions, 8); });
//    threads[1] = std::thread([&]() { find<100, 180>(shared, solutions, 8); });
//    threads[2] = std::thread([&]() { find<180, 250>(shared, solutions, 8); });
//    threads[3] = std::thread([&]() { find<250, 350>(shared, solutions, 8); });
//    threads[4] = std::thread([&]() { find<350, 480>(shared, solutions, 8); });
//    threads[5] = std::thread([&]() { find<480, 620>(shared, solutions, 8); });
//    threads[6] = std::thread([&]() { find<620, -1>(shared, solutions, 8); });
}

#endif //SFINDER_EXP_FIND_HPP

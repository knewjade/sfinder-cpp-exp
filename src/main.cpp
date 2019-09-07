#include <list>
#include <chrono>

#include "core/bits.hpp"
#include "sfinder/marker.hpp"
#include "sfinder/permutations.hpp"
#include "sfinder/lookup.hpp"
#include "sfinder/perfect_clear/checker.hpp"

#include "core/field.hpp"

#include "create.hpp"
#include "open.hpp"
#include "pieces.hpp"
#include "runner.hpp"

void create() {
    sfexp::createIndexes("../../csv/index.csv", "../../bin/index.bin");
    sfexp::createSolutions(
            "../../csv/indexed_solutions_10x4_SRS7BAG.csv",
            "../../bin/indexed_solutions_10x4_SRS7BAG.bin"
    );
}

void find(
        const core::Factory &factory,
        const core::Field &field,
        const std::vector<sfexp::PieceIndex> &indexes,
        const std::vector<sfexp::SolutionVector> &solutions,
        int maxDepth,
        const std::string &prefixFileName
) {
    auto output = prefixFileName + "output.bin";
    std::cout << output << std::endl;

    std::ifstream ifs(output);
    if (ifs.is_open()) {
        std::cout << "already exists. skip" << std::endl;
        return;
    }

    std::cout << solutions.size() << std::endl;
    std::cout << field.toString(4) << std::endl;

    const int max = static_cast<int>(std::pow(7, maxDepth));
    auto bits = sfinder::Bits::create(max);

    // Runner
    auto moveGenerator = core::srs::MoveGenerator(factory);
    auto currentOperations = std::vector<core::PieceType>{};
    auto runner = sfexp::Runner(factory, field, bits, moveGenerator);

    // run
    auto start = std::chrono::system_clock::now();

    int point = solutions.size() / 20;
    for (int i = 0, size = solutions.size(); i < size; ++i) {
        auto &solution = solutions[i];
        assert(solution.size() == maxDepth);

        auto vector = std::vector<sfexp::PieceIndex>{};
        for (const auto &index : solution) {
            auto pieceIndex = indexes[index];
            vector.push_back(pieceIndex);
        }

        runner.find(vector);

        if (i % point == point - 1) {
            auto s = i + 1;
            auto elapsed = std::chrono::system_clock::now() - start;
            auto count = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
            auto average = count / s;
            std::cout << "count=" << s << " "
                      << "elapsed=" << std::chrono::duration_cast<std::chrono::minutes>(elapsed).count() << "mins "
                      << "remind=" << average * (size - s) / 60.0 << "mins "
                      << std::endl;
        }
    }

    // Output
    sfexp::createBits(output, bits);
}

template<int Start, int End>
void find(
        const std::vector<sfexp::PieceIndex> &indexes,
        const std::vector<sfexp::SolutionVector> &solutions,
        int maxDepth,
        const std::string &prefixFileName,
        const core::Factory &factory,
        const core::Field &field,
        std::vector<int> &selected,
        int depth
) {
    if (depth == maxDepth) {
        auto v = std::vector<int>(selected.cbegin(), selected.cend());
        std::sort(v.begin(), v.end());

        auto s = prefixFileName;
        for (const auto &item : v) {
            s += std::to_string(item) + "_";
        }

        find(factory, field, indexes, solutions, maxDepth, s);
        return;
    }

    auto filledKey = field.filledKey();

    int endIndex = 0 <= End ? End : indexes.size();
    for (int index = Start; index < endIndex; ++index) {
        auto &pieceIndex = indexes[index];

        if ((filledKey & pieceIndex.deletedLine) != pieceIndex.deletedLine) {
            continue;
        }

        auto mino = core::Field{field};
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

        find<0, -1>(indexes, filteredSolutions, maxDepth, prefixFileName, factory, freeze, selected, depth - 1);

        selected.pop_back();
    }
}

template<int Start, int End>
void find(int maxDepth) {
    auto indexes = std::vector<sfexp::PieceIndex>{};
    sfexp::openIndexes("../../bin/index.bin", indexes);
    std::cout << indexes.size() << std::endl;

    auto solutions = std::vector<sfexp::SolutionVector>{};
    sfexp::openSolutions("../../bin/indexed_solutions_10x4_SRS7BAG.bin", solutions);
    std::cout << solutions.size() << std::endl;

    auto factory = core::Factory::create();
    auto selected = std::vector<int>{};
    auto field = core::Field{};

    find<Start, End>(indexes, solutions, maxDepth, "../../bin/", factory, field, selected, 10);
}

void verify() {
    std::vector<sfinder::Marker::FlagType> flags{};
    sfexp::openMarker("output.bin", flags);

    auto factory = core::Factory::create();
    auto marker = sfinder::Marker(flags);
    auto moveGenerator = core::srs::MoveGenerator(factory);
    auto finder = sfinder::perfect_clear::Checker<core::srs::MoveGenerator>(factory, moveGenerator);

    // Create permutations
    auto permutation = std::vector<sfinder::Permutation>{
            sfinder::Permutation::create<7>(core::kAllPieceType, 7),
            sfinder::Permutation::create<7>(core::kAllPieceType, 3),
    };
    auto permutations = sfinder::Permutations::create(permutation);
    std::cout << permutations.size() << std::endl;

    auto start = std::chrono::system_clock::now();

    int s = 0;
    int f = 0;
    int point = 10000;
    for (int index = 0, size = permutations.size(); index < size; ++index) {
        auto pieces = permutations.pieces(index);
        auto value = sfexp::toValue(pieces);
        if (!marker.succeed(value)) {
            auto success = finder.run<false>(core::Field{}, pieces, 10, 4, true);
            if (success) {
                for (const auto &piece : pieces) {
                    std::cout << factory.get(piece).name;
                }
                std::cout << std::endl;
            }

            f++;
        } else {
            s++;
        }

        if (index % point == point - 1) {
            auto s = index + 1;
            auto elapsed = std::chrono::system_clock::now() - start;
            auto count = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
            auto average = count / s;
            std::cout << "count=" << s << " "
                      << "elapsed=" << std::chrono::duration_cast<std::chrono::minutes>(elapsed).count() << "mins "
                      << "remind=" << average * (size - s) / 60.0 << "mins "
                      << std::endl;
        }
    }

    std::cout << "-------" << std::endl;

    std::cout << s << std::endl;
    std::cout << f << std::endl;
}

void check() {
    std::vector<sfinder::Marker::FlagType> flags{};
    sfexp::openMarker("output.bin", flags);

    auto factory = core::Factory::create();
    auto marker = sfinder::Marker(flags);

    // Create permutations
    auto permutation = std::vector<sfinder::Permutation>{
            sfinder::Permutation::create<7>(core::kAllPieceType, 7),
            sfinder::Permutation::create<7>(core::kAllPieceType, 3),
    };
    auto permutations = sfinder::Permutations::create(permutation);
    std::cout << permutations.size() << std::endl;

    // Create values
    auto permutation2 = std::vector<sfinder::Permutation>{
            sfinder::Permutation::create<7>(core::kAllPieceType, 7),
            sfinder::Permutation::create<7>(core::kAllPieceType, 4),
    };
    auto permutations2 = sfinder::Permutations::create(permutation2);

    auto values = std::vector<uint64_t>{};
    for (int i = 0; i < permutations2.size(); ++i) {
        auto pieces = permutations2.pieces(i);
        auto piecesValue = sfexp::PiecesValue::create(pieces);
        values.emplace_back(piecesValue.value());
    }

    std::sort(values.begin(), values.end());
    std::cout << values.size() << std::endl;

    // Create forward order lookUp
    auto forwardLookup = sfinder::ForwardOrderLookUp::create(10, 11, true, true);

    int s = 0;
    int f = 0;
    for (int index = 0; index < permutations.size(); ++index) {
        auto pieces = permutations.pieces(index);
        auto value = sfexp::toValue(pieces);
        if (!marker.succeed(value)) {
//            for (const auto &piece : pieces) {
//                std::cout << factory.get(piece).name;
//            }
//            std::cout << std::endl;

            auto begin = std::lower_bound(values.begin(), values.end(), sfexp::PiecesValue::lower(pieces));
            auto end = std::upper_bound(values.begin(), values.end(), sfexp::PiecesValue::upper(pieces));

            bool isAllSuccess = true;
            for (auto it = begin; it != end; ++it) {
                auto pieces11 = sfexp::PiecesValue::create(*it);
                auto parse = forwardLookup.parse(pieces11.vector());

                bool success = false;
                for (const auto &p11 : parse) {
                    auto value11 = sfexp::toValue(p11);
                    if (marker.succeed(value11)) {
                        success = true;
                        break;
                    }
                }

                if (!success) {
                    isAllSuccess = false;

                    for (const auto &piece : pieces11.vector()) {
                        std::cout << factory.get(piece).name;
                    }
                    std::cout << std::endl;

                    std::cout << "----" << std::endl;
                }
            }

            if (isAllSuccess) {
                s++;
            } else {
                f++;
            }
        } else {
            s++;
        }
    }

    std::cout << "-------" << std::endl;

    std::cout << s << std::endl;
    std::cout << f << std::endl;
}

int main() {
//    create();
    find<0, 90>(9);
//    find<90, 180>(9);
//    find<180, 270>(9);
//    find<270, 360>(9);
//    find<360, 450>(9);
//    find<450, 540>(9);
//    find<540, 630>(9);
//    find<630, 720>(9);
//    find<720, -1>(9);
//    verify();
//    check();
}

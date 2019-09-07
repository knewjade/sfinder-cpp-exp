#include <list>

#include "core/bits.hpp"
#include "sfinder/marker.hpp"
#include "sfinder/permutations.hpp"
#include "sfinder/lookup.hpp"
#include "sfinder/perfect_clear/full_finder.hpp"
#include "sfinder/perfect_clear/checker.hpp"

#include "core/field.hpp"
#include "core/factory.hpp"
#include "core/pieces.hpp"

#include "create.hpp"
#include "open.hpp"
#include "pieces.hpp"
#include "runner.hpp"

void create() {
    sfexp::createIndexes("../../csv/index.csv", "../../bin/index.bin");
    sfexp::createSolutions("../../csv/indexed_solutions_10x4_SRS7sBAG.csv", "../../bin/solutionsrs.bin");
}

void find() {
    auto indexes = std::vector<sfexp::PieceIndex>{};
    sfexp::openIndexes("../../bin/index.bin", indexes);
    std::cout << indexes.size() << std::endl;

    auto solutions = std::vector<sfexp::Solution>{};
    sfexp::openSolutions("../../bin/solutionsrs.bin", solutions);
    std::cout << solutions.size() << std::endl;

    const int max = 7 * 7 * 7 * 7 * 7 * 7 * 7 * 7 * 7 * 7;
    auto bits = sfinder::Bits::create(max);

    // Runner
    auto factory = core::Factory::create();
    auto moveGenerator = core::srs::MoveGenerator(factory);
    auto currentOperations = std::vector<core::PieceType>{};
    auto field = core::Field{};
    auto runner = sfexp::Runner(factory, field, bits, moveGenerator);

    // run
    auto start = std::chrono::system_clock::now();

    auto point = 10000;
    for (int i = 0, size = solutions.size(); i < size; ++i) {
        auto &solution = solutions[i];

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
    sfexp::createBits("output.bin", bits);
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
//    find();
//    verify();
//    check();
}

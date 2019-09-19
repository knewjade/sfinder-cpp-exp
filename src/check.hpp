#ifndef SFINDER_EXP_CHECK_HPP
#define SFINDER_EXP_CHECK_HPP

#include <map>

#include "sfinder/perfect_clear/types.hpp"

struct Configure {
    const core::Factory &factory;
    const int nextSize;
    const std::map<uint64_t, sfexp::PieceIndex> pieceIndexMap;
    const std::vector<PiecePopPair> pairs10;
    const std::vector<PiecePopPair> pairs11;
    core::srs::MoveGenerator moveGenerator;
    std::vector<std::vector<core::Move>> movePool;
    std::map<std::string, std::vector<sfinder::Bits::FlagType>> caches;
};

struct Target {
    const uint64_t pieces;
    const uint8_t nextPieceBit;
};

struct Result {
    const uint64_t pieces;
    const int failed;
};

void decideRange(
        const Configure &configure, std::vector<core::PieceType> head, uint8_t nextPieceBit,
        std::vector<Target> &targets
) {
    if (head.size() == configure.nextSize + 1) {
        auto pieces = sfexp::PiecesValue::create(head);
        auto target = Target{pieces.value(), nextPieceBit};
        targets.push_back(target);
        return;
    }

    auto next = nextPieceBit;

    do {
        auto bit = next & (next - 1U);
        auto i = next & ~bit;
        auto nextPieceType = core::pieceBitToVal[i];
        assert(0 <= nextPieceType);

        head.emplace_back(nextPieceType);
        auto n = nextPieceBit & ~i;
        decideRange(configure, head, n != 0 ? n : 0b1111111U, targets);
        head.pop_back();

        next = bit;
    } while (next != 0);
}

bool checks(
        Configure &configure, std::vector<core::PieceType> &used, std::vector<int> &selected,
        std::list<core::PieceType> pieces, core::PieceType hold, bool mustNotFirstHold, int visible
) {
    assert(10 <= visible && visible <= 11);

    auto reducedPairs = reduce(visible == 10 ? configure.pairs10 : configure.pairs11, used.cbegin(), used.cend());
    assert(!reducedPairs.empty());

    std::string holdName = hold == core::kEmptyPieceType ? "E" : configure.factory.get(hold).name;

    auto v = std::vector(selected.cbegin(), selected.cend());
    std::sort(v.begin(), v.end());

    std::string name{};
    for (const auto &index : v) {
        name += std::to_string(index) + "_";
    }
    name += "output.bin";

    auto rest = pieces.size() + (hold != core::kEmptyPieceType ? 1 : 0);
    auto file =
            "../../../cy1/bin" + std::to_string(selected.size()) + "-"
            + holdName + "h" + std::to_string(rest) +
            (mustNotFirstHold ? "f" : "") + "/" + name;

    auto vector = configure.caches[name];
    if (vector.empty()) {
        if (!sfexp::fileExists(file)) {
            std::cout << file << std::endl;
            return false;
        }

        int size = static_cast<int>(std::pow(7, rest));
        int length = sfinder::Bits::calculateLength(size);
        sfexp::openBitsFixedSize(file, vector, length);
    }

    auto bits = sfinder::Bits(vector);

    auto permutationVector = createPermutationVector(reducedPairs);
    auto permutations = sfinder::Permutations::create(permutationVector);

    if (hold != core::kEmptyPieceType) {
        pieces.push_front(hold);
    }
    int number = permutations.numberify(pieces.cbegin(), pieces.cend());
    assert(0 <= number);
    if (hold != core::kEmptyPieceType) {
        pieces.pop_front();
    }

    return bits.checks(number);
}

int check(
        Configure &configure, std::list<core::PieceType> pieces, const core::Field &initField, int currentLine,
        std::vector<core::PieceType> &used, std::vector<int> &selected,
        core::PieceType hold, uint8_t nextPieceBit, int visible, bool mustNotFirstHold,
        int allowFailed
);

int move(
        Configure &configure, std::vector<core::Move> &moves,
        std::list<core::PieceType> &pieces, const core::Field &initField, int currentLine,
        std::vector<core::PieceType> &used, std::vector<int> &selected,
        core::PieceType currentPiece, core::PieceType nextHold, uint8_t nextPieceBit, int nextVisible,
        int allowFailedInit
) {
    auto field = core::Field(initField);
    auto deletedKey = field.clearLineReturnKey();

    moves.clear();

    configure.moveGenerator.search(moves, field, currentPiece, currentLine);

    auto allowFailed = allowFailedInit;

    for (const auto &move : moves) {
        auto next = nextPieceBit;
        auto &blocks = configure.factory.get(currentPiece, move.rotateType);

        auto mino = core::Field{};
        mino.put(blocks, move.x, move.y);
        mino.insertWhiteLineWithKey(deletedKey);

        auto &pieceIndex = configure.pieceIndexMap.at(mino.boardLow());

        auto freeze = core::Field(field);
        freeze.merge(mino);

        auto freeze2 = core::Field(freeze);
        int numCleared = freeze2.clearLineReturnNum();

        int nextLeftLine = currentLine - numCleared;

        if (!sfinder::perfect_clear::validate(freeze2, nextLeftLine)) {
            continue;
        }

        freeze.insertBlackLineWithKey(deletedKey);

        used.push_back(currentPiece);
        selected.push_back(pieceIndex.index);

        int failed = 0;

        do {
            auto bit = next & (next - 1U);
            auto i = next & ~bit;
            auto nextPieceType = core::pieceBitToVal[i];
            assert(0 <= nextPieceType);

            auto n = nextPieceBit & ~i;

            pieces.emplace_back(nextPieceType);
            failed += check(
                    configure, pieces, freeze, nextLeftLine, used, selected,
                    nextHold, n != 0 ? n : 0b1111111U, nextVisible, false, allowFailed
            );

            pieces.pop_back();

            if (allowFailed <= failed) {
                break;
            }

            next = bit;
        } while (next != 0);

        selected.pop_back();
        used.pop_back();

        if (failed == 0) {
            return 0;
        }

        if (failed < allowFailed) {
            allowFailed = failed;
        }
    }

    return allowFailed;
}

int check(
        Configure &configure, std::list<core::PieceType> pieces, const core::Field &initField, int currentLine,
        std::vector<core::PieceType> &used, std::vector<int> &selected,
        core::PieceType hold, uint8_t nextPieceBit, int visible, bool mustNotFirstHold,
        int allowFailedInit
) {
    assert(0 < visible && visible <= 11);

    if (10 <= visible) {
        auto result = checks(configure, used, selected, pieces, hold, mustNotFirstHold, visible);
        if (result) {
            return 0;
        }

        if (11 <= visible) {
            return 1;
        }
    }

    assert(visible <= 10);

    int allowFailed = allowFailedInit;

    auto moves = configure.movePool[visible];

    auto headPieceType = *pieces.cbegin();
    pieces.pop_front();

    // hold
    if (!mustNotFirstHold) {
        if (hold == core::kEmptyPieceType) {
            // empty
            auto next = nextPieceBit;

            int failed = 0;

            do {
                auto bit = next & (next - 1U);
                auto i = next & ~bit;
                auto nextPieceType = core::pieceBitToVal[i];
                assert(0 <= nextPieceType);

                auto n = nextPieceBit & ~i;

                pieces.emplace_back(nextPieceType);

                failed += check(
                        configure, pieces, initField, currentLine, used, selected,
                        headPieceType, n, visible + 1, true, allowFailed
                );

                if (allowFailed <= failed) {
                    break;
                }

                pieces.pop_back();

                next = bit;
            } while (next != 0);

            if (failed == 0) {
                return 0;
            }

            if (failed < allowFailed) {
                allowFailed = failed;
            }
        }
    }

    // no hold
    {
        int failed = move(
                configure, moves, pieces, initField, currentLine, used, selected,
                headPieceType, hold, nextPieceBit, visible + 1, allowFailed
        );

        if (failed == 0) {
            return 0;
        }

        if (failed < allowFailed) {
            allowFailed = failed;
        }
    }

    // hold
    if (!mustNotFirstHold) {
        if (hold != core::kEmptyPieceType) {
            // swap
            int failed = move(
                    configure, moves, pieces, initField, currentLine, used, selected,
                    hold, headPieceType, nextPieceBit, visible + 1, allowFailed
            );

            if (failed == 0) {
                return 0;
            }

            if (failed < allowFailed) {
                allowFailed = failed;
            }
        }
    }

    pieces.push_front(headPieceType);

    return allowFailed;
}

void check(
        const int nextSize, const core::PieceType hold, const uint8_t nextPieceBit
) {
    assert(0 <= nextSize && nextSize <= 9);

    auto factory = core::Factory::create();

    // Load
    auto indexes = std::vector<sfexp::PieceIndex>{};
    sfexp::openIndexes("../../bin/index.bin", indexes);
    std::cout << "indexes: " << indexes.size() << std::endl;

    // Create map
    auto pieceIndexMap = std::map<uint64_t, sfexp::PieceIndex>{};
    for (const auto &pieceIndex : indexes) {
        auto mino = core::Field{};
        auto blocks = factory.get(pieceIndex.piece, pieceIndex.rotate);
        mino.put(blocks, pieceIndex.x, pieceIndex.y);
        mino.insertWhiteLineWithKey(pieceIndex.deletedLine);
        pieceIndexMap[mino.boardLow()] = pieceIndex;
    }

    // Create pairs
    auto all = core::PieceCounter::create(core::kAllPieceType.cbegin(), core::kAllPieceType.cend());
    auto pairs10 = std::vector{
//            PiecePopPair{core::PieceCounter::create(Hold), 1},
            PiecePopPair{all, 7},
            PiecePopPair{all, 3},
    };
    auto pairs11 = std::vector{
//            PiecePopPair{core::PieceCounter::create(Hold), 1},
            PiecePopPair{all, 7},
            PiecePopPair{all, 4},
    };

    // Create configure
    auto moveGenerator = core::srs::MoveGenerator(factory);
    auto movePool = std::vector<std::vector<core::Move>>{};
    for (int count = 0; count < 12; ++count) {
        movePool.emplace_back();
    }
    auto caches = std::map<std::string, std::vector<sfinder::Bits::FlagType>>{};
    auto configure = Configure{factory, nextSize, pieceIndexMap, pairs10, pairs11, moveGenerator, movePool, caches};

    // Find targets
    auto targets = std::vector<Target>{};
    auto pieces = std::vector<core::PieceType>{};
    decideRange(configure, pieces, nextPieceBit, targets);
    std::cout << "targets: " << targets.size() << std::endl;

    // Expand
    std::string resultsFileName = "../../bin/test.bin";
    std::map<uint64_t, int> results{};
    std::vector<Result> resultsVector{};

    {
        std::ifstream fin(resultsFileName, std::ios::in | std::ios::binary);
        if (fin) {
            while (true) {
                Result result{};
                fin.read(reinterpret_cast<char *>(&result), sizeof(Result));
                if (fin.eof()) {
                    break;
                }
                results[result.pieces] = result.failed;
                resultsVector.push_back(result);
            }
        }

        fin.close();
    }

    for (auto index = 0; index < targets.size(); index++) {
        auto &target = targets[index];

        if (results.find(target.pieces) != results.cend()) {
            continue;
        }

        auto piecesValue = sfexp::PiecesValue::create(target.pieces);
        auto piecesList = piecesValue.list();
        int visible = piecesList.size() + (hold != core::kEmptyPieceType ? 1 : 0);

        std::string name{};
        for (const auto &piece : piecesList) {
            name += factory.get(piece).name;
        }
        std::cout << name << std::endl;

        auto all = core::bitCount(target.nextPieceBit);

        auto used = std::vector<core::PieceType>{};
        auto selected = std::vector<int>{};
        auto field = core::Field{};
        int failed = check(
                configure, piecesList, field, 4, used, selected, hold, target.nextPieceBit, visible, false, all
        );
        std::cout << (all - failed) << "/" << all << std::endl;

        resultsVector.push_back(Result{target.pieces, failed});

        if (0 < index && index % 2000 == 0) {
            std::cout << "output..." << std::endl;

            // Create binary
            std::ofstream out(resultsFileName, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!out) {
                std::cout << "Failed to open file" << std::endl;
                throw std::runtime_error("Failed to open file: " + resultsFileName);
            }

            out.write(reinterpret_cast<const char *>(&resultsVector[0]), sizeof(uint64_t) * resultsVector.size());

            out.close();
        }
    }
}

void check() {
    check(
            8, core::kEmptyPieceType, 0b1111111ULL
    );
}

#endif //SFINDER_EXP_CHECK_HPP
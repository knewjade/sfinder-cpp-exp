#include <iostream>
#include <list>
#include <fstream>
#include <chrono>

#include "core/types.hpp"
#include "core/field.hpp"
#include "core/pieces.hpp"
#include "core/moves.hpp"

#include "sfinder/perfect_clear/checker.hpp"

#include "cache.hpp"

auto factory = core::Factory::create();
auto moveGenerator = core::srs::MoveGenerator(factory);
auto movePool = std::vector<std::vector<core::Move>>{
        std::vector<core::Move>{},
        std::vector<core::Move>{},
        std::vector<core::Move>{},
        std::vector<core::Move>{},
        std::vector<core::Move>{},

        std::vector<core::Move>{},
        std::vector<core::Move>{},
        std::vector<core::Move>{},
        std::vector<core::Move>{},
        std::vector<core::Move>{},

        std::vector<core::Move>{},
};

inline bool validate(const core::Field &field, int maxLine) {
    int sum = maxLine - field.getBlockOnX(0, maxLine);
    for (int x = 1; x < core::kFieldWidth; x++) {
        int emptyCountInColumn = maxLine - field.getBlockOnX(x, maxLine);
        if (field.isWallBetween(x, maxLine)) {
            if (sum % 4 != 0)
                return false;
            sum = emptyCountInColumn;
        } else {
            sum += emptyCountInColumn;
        }
    }

    return sum % 4 == 0;
}

template<bool UseFirstHold>
bool check(const core::Field field, core::Pieces &pieces, int maxDepth, int maxLine, bool holdEmpty) {
    auto checker = sfinder::perfect_clear::Checker<core::srs::MoveGenerator>(factory, moveGenerator);

    auto vector = pieces.vector();
    return checker.run<UseFirstHold>(field, vector, maxDepth, maxLine, holdEmpty);
}

auto cache11true = Cache{};
auto cache11false = Cache{};

template<bool UseFirstHold>
bool check11(
        core::Field field, std::list<core::PieceType> &pieces, core::PieceType hold, int depth, int line
) {
    auto piecesObj = core::Pieces::create(pieces);

    if constexpr (UseFirstHold) {
        auto cacheResult = cache11true.get(field, hold, piecesObj, line);
        if (0 <= cacheResult) {
            return true;
        }
    }

    {
        auto cacheResult = cache11false.get(field, hold, piecesObj, line);
        if (0 <= cacheResult) {
            return true;
        }
    }

    auto success = check<UseFirstHold>(field, piecesObj, hold == core::emptyPieceType, depth, line);

    if constexpr (UseFirstHold) {
        cache11true.add(field, hold, piecesObj, line, success ? 1 : 0);
    } else {
        cache11false.add(field, hold, piecesObj, line, success ? 1 : 0);
    }

    return success;
}

auto cache10 = Cache{};

int check10(
        const core::Field field, std::list<core::PieceType> &pieces, core::PieceType hold,
        uint64_t initNext, int depth, int line
) {
    auto piecesObj = core::Pieces::create(pieces);
    auto cacheResult = cache10.get(field, hold, piecesObj, line);
    if (0 <= cacheResult) {
        return cacheResult;
    }

    if (initNext == 0) {
        initNext = 0b1111111;
    }

    {
        auto success = check<true>(field, piecesObj, depth, line, hold == core::emptyPieceType);
        auto result = success ? core::bitCount(initNext) : 0;
        cache10.add(field, hold, piecesObj, line, result);
        if (success) {
            return result;
        }
    }

    auto headPieceType = *pieces.cbegin();
    auto moves = movePool[10];

    int max = -1;

    // no hold
    moves.clear();
    moveGenerator.search(moves, field, headPieceType, line);

    pieces.pop_front();

    for (const auto &move : moves) {
        auto next = initNext;
        auto &blocks = factory.get(headPieceType, move.rotateType);

        auto freeze = core::Field(field);
        freeze.put(blocks, move.x, move.y);

        int numCleared = freeze.clearLineReturnNum();

        int nextLeftLine = line - numCleared;

        if (!validate(freeze, nextLeftLine)) {
            continue;
        }

        int score = 0;

        do {
            auto bit = next & (next - 1U);
            auto i = next & ~bit;
            auto nextPieceType = core::pieceBitToVal[i];
            assert(0 <= nextPieceType);

            pieces.emplace_back(nextPieceType);
            if (check11<true>(freeze, pieces, hold, depth - 1, nextLeftLine)) {
                score += 1;
            }
            pieces.pop_back();

            next = bit;
        } while (next != 0);

        if (max < score) {
            max = score;
        }
    }

    pieces.push_front(headPieceType);

    // hold
    if (hold == core::emptyPieceType) {
        // empty
        pieces.pop_front();

        auto head2PieceType = *pieces.cbegin();
        moves.clear();
        moveGenerator.search(moves, field, head2PieceType, line);

        pieces.pop_front();

        for (const auto &move : moves) {
            auto next = initNext;
            auto &blocks = factory.get(head2PieceType, move.rotateType);

            auto freeze = core::Field(field);
            freeze.put(blocks, move.x, move.y);

            int numCleared = freeze.clearLineReturnNum();

            int nextLeftLine = line - numCleared;

            if (!validate(freeze, nextLeftLine)) {
                continue;
            }

            int score = 0;

            do {
                auto bit = next & (next - 1U);
                auto i = next & ~bit;
                auto nextPieceType = core::pieceBitToVal[i];
                assert(0 <= nextPieceType);

                pieces.emplace_back(nextPieceType);
                if (check11<false>(freeze, pieces, headPieceType, depth - 1, nextLeftLine)) {
                    score += 1;
                }
                pieces.pop_back();

                next = bit;
            } while (next != 0);

            if (max < score) {
                max = score;
            }
        }

        pieces.push_front(head2PieceType);
        pieces.push_front(headPieceType);
    } else {
        // swap
        moves.clear();
        moveGenerator.search(moves, field, hold, line);

        pieces.pop_front();

        for (const auto &move : moves) {
            auto next = initNext;
            auto &blocks = factory.get(hold, move.rotateType);

            auto freeze = core::Field(field);
            freeze.put(blocks, move.x, move.y);

            int numCleared = freeze.clearLineReturnNum();

            int nextLeftLine = line - numCleared;

            if (!validate(freeze, nextLeftLine)) {
                continue;
            }

            int score = 0;

            do {
                auto bit = next & (next - 1U);
                auto i = next & ~bit;
                auto nextPieceType = core::pieceBitToVal[i];
                assert(0 <= nextPieceType);

                pieces.emplace_back(nextPieceType);
                if (check11<true>(freeze, pieces, headPieceType, depth - 1, nextLeftLine)) {
                    score += 1;
                }
                pieces.pop_back();

                next = bit;
            } while (next != 0);

            if (max < score) {
                max = score;
            }
        }

        pieces.push_front(headPieceType);
    }

    return max;
}

template<int Next>
int calc(
        core::Field field, std::list<core::PieceType> &pieces, core::PieceType hold,
        uint64_t initNext, int visible, int depth, int line
) {
    if (visible == 10) {
        return check10(field, pieces, hold, initNext, depth, line);
    }

    if (visible == 11) {
        return check11<true>(field, pieces, hold, depth, line);
    }

    if (initNext == 0) {
        initNext = 0b1111111;
    }

    auto headPieceType = *pieces.cbegin();
    auto moves = movePool[visible];

    int max = -1;

    // no hold
    moves.clear();
    moveGenerator.search(moves, field, headPieceType, line);

    pieces.pop_front();

    for (const auto &move : moves) {
        auto next = initNext;
        auto &blocks = factory.get(headPieceType, move.rotateType);

        auto freeze = core::Field(field);
        freeze.put(blocks, move.x, move.y);

        int numCleared = freeze.clearLineReturnNum();

        int nextLeftLine = line - numCleared;

        if (!validate(freeze, nextLeftLine)) {
            continue;
        }

        do {
            auto bit = next & (next - 1U);
            auto i = next & ~bit;
            auto nextPieceType = core::pieceBitToVal[i];
            assert(0 <= nextPieceType);

            pieces.emplace_back(nextPieceType);
            auto score = calc<Next>(freeze, pieces, hold, initNext & ~i, visible + 1, depth, nextLeftLine);
            if (max < score) {
                max = score;
            }
            pieces.pop_back();

            next = bit;
        } while (next != 0);
    }

    pieces.push_front(headPieceType);

    // hold
    if (hold == core::emptyPieceType) {
        // empty
        pieces.pop_front();

        auto head2PieceType = *pieces.cbegin();
        moves.clear();
        moveGenerator.search(moves, field, head2PieceType, line);

        pieces.pop_front();

        for (const auto &move : moves) {
            auto next = initNext;
            auto &blocks = factory.get(head2PieceType, move.rotateType);

            auto freeze = core::Field(field);
            freeze.put(blocks, move.x, move.y);

            int numCleared = freeze.clearLineReturnNum();

            int nextLeftLine = line - numCleared;

            if (!validate(freeze, nextLeftLine)) {
                continue;
            }

            do {
                auto bit = next & (next - 1U);
                auto i = next & ~bit;
                auto nextPieceType = core::pieceBitToVal[i];
                assert(0 <= nextPieceType);

                pieces.emplace_back(nextPieceType);
                auto score = calc<Next>(
                        freeze, pieces, headPieceType, initNext & ~i, visible + 1, depth - 1, nextLeftLine
                );
                if (max < score) {
                    max = score;
                }
                pieces.pop_back();

                next = bit;
            } while (next != 0);
        }

        pieces.push_front(head2PieceType);
        pieces.push_front(headPieceType);
    } else {
        // swap
        moves.clear();
        moveGenerator.search(moves, field, hold, line);

        pieces.pop_front();

        for (const auto &move : moves) {
            auto next = initNext;
            auto &blocks = factory.get(hold, move.rotateType);

            auto freeze = core::Field(field);
            freeze.put(blocks, move.x, move.y);

            int numCleared = freeze.clearLineReturnNum();

            int nextLeftLine = line - numCleared;

            if (!validate(freeze, nextLeftLine)) {
                continue;
            }

            do {
                auto bit = next & (next - 1U);
                auto i = next & ~bit;
                auto nextPieceType = core::pieceBitToVal[i];
                assert(0 <= nextPieceType);

                pieces.emplace_back(nextPieceType);
                auto score = calc<Next>(
                        freeze, pieces, headPieceType, initNext & ~i, visible + 1, depth - 1, nextLeftLine
                );
                if (max < score) {
                    max = score;
                }
                pieces.pop_back();

                next = bit;
            } while (next != 0);
        }

        pieces.push_front(headPieceType);
    }

    return max;
}

std::fstream file("./result.dat", std::ios::binary | std::ios::out);

template<int Next, int Call>
void generate(std::list<core::PieceType> &pieces, uint64_t initNext) {
    static_assert(0 <= Next && Next <= 10);
    static_assert(0 <= Call && Call <= Next + 1);

    if constexpr (Call == Next + 1) {
        auto field = core::Field{};
        auto copiedPieces = std::list<core::PieceType>(pieces.cbegin(), pieces.cend());
        int score = calc<Next>(field, copiedPieces, core::emptyPieceType, initNext, copiedPieces.size(), 10, 4);

        auto piecesObj = core::Pieces::create(copiedPieces);
        uint64_t value = piecesObj.value();
        file.write((char *) &value, sizeof(uint64_t));
        file.write((char *) &score, sizeof(int));
    } else {
        if (initNext == 0) {
            initNext = 0b1111111;
        }

        auto next = initNext;

        auto start = std::chrono::system_clock::now();

        do {
            auto bit = next & (next - 1U);
            auto i = next & ~bit;
            auto nextPieceType = core::pieceBitToVal[i];
            assert(0 <= nextPieceType);

            if constexpr (Call < 2) {
                auto end = std::chrono::system_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(end - start).count();
                std::cout << factory.get(nextPieceType).name << Call << " " << elapsed << "mins" << std::endl;
            }

            pieces.emplace_back(nextPieceType);
            generate<Next, Call + 1>(pieces, initNext & ~i);
            pieces.pop_back();

            next = bit;
        } while (next != 0);
    }
}

template<int Next>
void generate() {
    auto pieces = std::list<core::PieceType>{};
    generate<Next, 0>(pieces, 0);
}

int main() {
    generate<9>();

    std::cout << "cache10 = " << cache10.size() << std::endl;
    std::cout << "cache11true = " << cache11true.size() << std::endl;
    std::cout << "cache11false = " << cache11false.size() << std::endl;

    file.close();

    return 0;
}
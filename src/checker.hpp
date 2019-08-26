#ifndef SFINDER_EXP_CHECKER_HPP
#define SFINDER_EXP_CHECKER_HPP

#include "core/field.hpp"
#include "core/moves.hpp"

#include "sfinder/perfect_clear/checker.hpp"

#include "cache.hpp"

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

class CacheChecker {
public:
    explicit CacheChecker(const core::Factory &factory)
            : factory_(factory), moveGenerator_(core::srs::MoveGenerator(factory)), moves_(std::vector<core::Move>{}) {}

    template<bool UseFirstHold>
    bool check(const core::Field field, core::Pieces &pieces, int maxDepth, int maxLine, bool holdEmpty) {
        auto checker = sfinder::perfect_clear::Checker<core::srs::MoveGenerator>(factory_, moveGenerator_);
        auto vector = pieces.vector();
        return checker.run<UseFirstHold>(field, vector, maxDepth, maxLine, holdEmpty);
    }

    template<bool UseFirstHold>
    bool check11(
            core::Field field, std::list<core::PieceType> &pieces, core::PieceType hold, int depth, int line
    ) {
        auto piecesObj = core::Pieces::create(pieces);

        if constexpr (UseFirstHold) {
            auto cacheResult = cache11true_.get(field, hold, piecesObj, line);
            if (0 <= cacheResult) {
                return true;
            }
        }

        {
            auto cacheResult = cache11false_.get(field, hold, piecesObj, line);
            if (0 <= cacheResult) {
                return true;
            }
        }

        auto success = check<UseFirstHold>(field, piecesObj, hold == core::emptyPieceType, depth, line);

        if constexpr (UseFirstHold) {
            cache11true_.add(field, hold, piecesObj, line, success ? 1 : 0);
        } else {
            cache11false_.add(field, hold, piecesObj, line, success ? 1 : 0);
        }

        return success;
    }

    int check10(
            const core::Field field, std::list<core::PieceType> &pieces, core::PieceType hold,
            int depth, int line, uint64_t initNext
    ) {
        auto piecesObj = core::Pieces::create(pieces);
        auto cacheResult = cache10_.get(field, hold, piecesObj, line);
        if (0 <= cacheResult) {
            return cacheResult;
        }

        if (initNext == 0) {
            initNext = 0b1111111;
        }

        {
            auto success = check<true>(field, piecesObj, depth, line, hold == core::emptyPieceType);
            auto result = success ? core::bitCount(initNext) : 0;
            cache10_.add(field, hold, piecesObj, line, result);
            if (success) {
                return result;
            }
        }

        auto headPieceType = *pieces.cbegin();

        int max = -1;

        // no hold
        {
            moves_.clear();
            moveGenerator_.search(moves_, field, headPieceType, line);

            pieces.pop_front();

            for (const auto &move : moves_) {
                auto next = initNext;
                auto &blocks = factory_.get(headPieceType, move.rotateType);

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
        }

        // hold
        if (hold == core::emptyPieceType) {
            // empty
            pieces.pop_front();

            auto head2PieceType = *pieces.cbegin();
            moves_.clear();
            moveGenerator_.search(moves_, field, head2PieceType, line);

            pieces.pop_front();

            for (const auto &move : moves_) {
                auto next = initNext;
                auto &blocks = factory_.get(head2PieceType, move.rotateType);

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
            moves_.clear();
            moveGenerator_.search(moves_, field, hold, line);

            pieces.pop_front();

            for (const auto &move : moves_) {
                auto next = initNext;
                auto &blocks = factory_.get(hold, move.rotateType);

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

private:
    const core::Factory factory_;
    core::srs::MoveGenerator moveGenerator_;
    std::vector<core::Move> moves_;

    Cache cache10_ = Cache{};
    Cache cache11true_ = Cache{};
    Cache cache11false_ = Cache{};
};

#endif //SFINDER_EXP_CHECKER_HPP

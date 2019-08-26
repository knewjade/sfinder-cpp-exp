#ifndef SFINDER_EXP_CALCULATER_HPP
#define SFINDER_EXP_CALCULATER_HPP

#include <list>

#include "core/field.hpp"

#include "checker.hpp"

template<int Next>
class Calculator {
public:
    explicit Calculator(const core::Factory &factory, CacheChecker &checker)
            : factory_(factory), moveGenerator_(core::srs::MoveGenerator(factory)), checker_(checker),
              movePool_(std::vector<std::vector<core::Move>>{}) {
        for (int count = 0; count < 11; ++count) {
            movePool_.push_back(std::vector<core::Move>{});
        }
    }

    int calculate(
            core::Field field, std::list<core::PieceType> &pieces, core::PieceType hold,
            uint64_t initNext, int visible, int depth, int line
    ) {
        if (visible == 10) {
            return checker_.check10(field, pieces, hold, depth, line, initNext);
        }

        if (visible == 11) {
            return checker_.check11<true>(field, pieces, hold, depth, line);
        }

        if (initNext == 0) {
            initNext = 0b1111111;
        }

        auto headPieceType = *pieces.cbegin();
        auto moves = movePool_[visible];

        int max = -1;

        // no hold
        moves.clear();
        moveGenerator_.search(moves, field, headPieceType, line);

        pieces.pop_front();

        for (const auto &move : moves) {
            auto next = initNext;
            auto &blocks = factory_.get(headPieceType, move.rotateType);

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
                auto score = calculate(
                        freeze, pieces, hold, initNext & ~i, visible + 1, depth, nextLeftLine
                );
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
            moveGenerator_.search(moves, field, head2PieceType, line);

            pieces.pop_front();

            for (const auto &move : moves) {
                auto next = initNext;
                auto &blocks = factory_.get(head2PieceType, move.rotateType);

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
                    auto score = calculate(
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
            moveGenerator_.search(moves, field, hold, line);

            pieces.pop_front();

            for (const auto &move : moves) {
                auto next = initNext;
                auto &blocks = factory_.get(hold, move.rotateType);

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
                    auto score = calculate(
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

private:
    const core::Factory factory_;
    core::srs::MoveGenerator moveGenerator_;
    CacheChecker checker_;
    std::vector<std::vector<core::Move>> movePool_;
};


#endif //SFINDER_EXP_CALCULATER_HPP

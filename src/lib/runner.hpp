#ifndef SFINDER_EXP_LIB_RUNNER_HPP
#define SFINDER_EXP_LIB_RUNNER_HPP

#include <array>
#include <vector>

#include "core/factory.hpp"
#include "core/moves.hpp"
#include "sfinder/marker.hpp"

#include "pieces.hpp"

namespace sfexp {
    constexpr std::array<int, 513> createIndexMapping() noexcept {
        auto array = std::array<int, 513>{};
        for (unsigned int i = 0; i < 10; ++i) {
            array[1U << i] = i;
        }
        return array;
    }

    class Runner {
    public:
        static constexpr auto kIndexMapping = createIndexMapping();

        Runner(
                const core::Factory &factory, const core::Field field,
                sfinder::Bits &bits, core::srs::MoveGenerator &moveGenerator
        ) : factory_(factory), field_(field),
            bits_(bits), moveGenerator_(moveGenerator), currentOperations_(std::vector<core::PieceType>{}) {
        }

        void find(std::vector<sfexp::PieceIndex> &operationWithKeys) {
            auto field = core::Field{field_};
            currentOperations_.clear();
            find(field, operationWithKeys, (1U << operationWithKeys.size()) - 1U);
        }

    private:
        const core::Factory &factory_;
        const core::Field field_;

        sfinder::Bits &bits_;
        core::srs::MoveGenerator &moveGenerator_;
        std::vector<core::PieceType> currentOperations_;

        const int height_ = 4;

        void find(core::Field &field, std::vector<sfexp::PieceIndex> &operationWithKeys, uint16_t initRemainder) {
            assert(initRemainder != 0);

            auto deletedKey = field.clearLineReturnKey();

            auto remainder = initRemainder;
            do {
                auto next = remainder & (remainder - 1U);
                auto indexBit = remainder & ~next;

                auto index = kIndexMapping[indexBit];
                assert(0 <= index && index < operationWithKeys.size());
                auto operation = operationWithKeys[index];

                currentOperations_.push_back(operation.piece);

                auto needDeletedKey = operation.deletedLine;
                if ((deletedKey & needDeletedKey) == needDeletedKey) {
                    auto originalY = operation.y;
                    auto deletedLines = core::bitCount(core::getUsingKeyBelowY(originalY) & deletedKey);

                    auto &blocks = factory_.get(operation.piece, operation.rotate);

                    auto x = operation.x;
                    auto y = originalY - deletedLines;

                    if (
                            field.isOnGround(blocks, x, y) &&
                            field.canPut(blocks, x, y) &&
                            moveGenerator_.canReach(field, blocks.pieceType, blocks.rotateType, x, y, height_)
                            ) {

                        auto nextReminder = initRemainder & ~indexBit;
                        if (nextReminder == 0) {
                            // Found
                            auto value = toValue(currentOperations_);
                            bits_.set(value, true);
                        } else {
                            auto nextField = core::Field{field};
                            nextField.put(blocks, x, y);
                            nextField.insertBlackLineWithKey(deletedKey);

                            find(nextField, operationWithKeys, nextReminder);
                        }
                    }
                }

                currentOperations_.pop_back();

                remainder = next;
            } while (remainder != 0);

            field.insertBlackLineWithKey(deletedKey);
        }
    };
}

#endif //SFINDER_EXP_LIB_RUNNER_HPP

#ifndef EXP_PIECES_HPP
#define EXP_PIECES_HPP

#include <vector>
#include <array>

#include "core/types.hpp"

namespace sfexp {
    constexpr std::array<unsigned, 16> createSlideIndex() noexcept {
        auto arr = std::array<unsigned, 16>{};
        for (int index = 0; index < arr.size(); index++) {
            arr[index] = 4 * (15 - index);
        }
        return arr;
    }

    constexpr std::array<unsigned, 16> kSlideIndex = createSlideIndex();

    class PiecesValue {
    public:
        static uint64_t lower(const std::vector<core::PieceType> &pieces) {
            auto size = pieces.size();
            assert(size <= 16);

            uint64_t value = 0ULL;
            for (int index = 0; index < size; ++index) {
                auto slide = kSlideIndex[index];
                value += static_cast<uint64_t>(pieces[index]) << slide;
            }

            return value;
        }

        static uint64_t upper(const std::vector<core::PieceType> &pieces) {
            auto size = pieces.size();
            assert(size <= 16);

            uint64_t value = (0b10000ULL << kSlideIndex[size]) - 1ULL;
            for (int index = 0; index < size; ++index) {
                auto slide = kSlideIndex[index];
                value += static_cast<uint64_t>(pieces[index]) << slide;
            }

            return value;
        }

        static PiecesValue create(const std::vector<core::PieceType> &pieces) {
            return PiecesValue(upper(pieces), pieces.size());
        }

        static PiecesValue create(uint64_t value) {
            int size = 0;
            for (auto slide : kSlideIndex) {
                auto piece = (value >> slide) & 0b1111ULL;
                if (0b1000ULL <= piece) {
                    break;
                }
                size += 1;
            }
            return PiecesValue(value, size);
        }

        PiecesValue(uint64_t value, int size) : value_(value), size_(size) {
        }

        std::vector<core::PieceType> vector() const {
            auto vector = std::vector<core::PieceType>(size_);
            for (int index = 0; index < size_; ++index) {
                uint64_t piece = (value_ >> kSlideIndex[index]) & 0b1111ULL;
                vector[index] = static_cast<core::PieceType>(piece);
            }
            return vector;
        }

        uint64_t value() const {
            return value_;
        }

        uint64_t size() const {
            return size_;
        }

    private:
        const uint64_t value_;
        const int size_;
    };
}

#endif //EXP_PIECES_HPP

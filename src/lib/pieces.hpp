#ifndef SFINDER_EXP_LIB_PIECES_HPP
#define SFINDER_EXP_LIB_PIECES_HPP

#include <vector>
#include <array>
#include <list>

#include "core/types.hpp"

namespace sfexp {
    template<class InputIterator>
    int toValue(
            InputIterator begin,
            typename core::is_input_iterator<InputIterator, core::PieceType>::type end
    ) {
        int value = 0;
        auto it = begin;

        value += *it;
        ++it;

        for (; it != end; ++it) {
            value *= 7;
            value += *it;
        }

        return value;
    }

    template<class Container>
    int toValue(const Container &pieces) {
        return toValue(pieces.cbegin(), pieces.cend());
    }

    constexpr std::array<unsigned, 16> createSlideIndex() noexcept {
        auto arr = std::array<unsigned, 16>{};
        for (int index = 0, size = arr.size(); index < size; index++) {
            arr[index] = 4 * (15 - index);
        }
        return arr;
    }

    constexpr std::array<unsigned, 16> kSlideIndex = createSlideIndex();

    class PiecesValue {
    public:
        template<typename Container>
        static uint64_t lower(const Container &pieces) {
            return lower(pieces.begin(), pieces.end());
        }

        template<class InputIterator>
        static uint64_t lower(
                InputIterator begin,
                typename core::is_input_iterator<InputIterator, core::PieceType>::type end
        ) {
            int index = 0;
            uint64_t value = 0ULL;
            for (auto it = begin; it != end; ++it) {
                auto piece = *it;
                assert(0 <= piece);

                auto slide = kSlideIndex[index];
                value += static_cast<uint64_t>(piece) << slide;
                index += 1;
            }

            assert(index <= 16);

            return value;
        }

        template<typename Container>
        static uint64_t upper(const Container &pieces) {
            return upper(pieces.begin(), pieces.end());
        }

        template<class InputIterator>
        static uint64_t upper(
                InputIterator begin,
                typename core::is_input_iterator<InputIterator, core::PieceType>::type end
        ) {
            int index = 0;
            uint64_t value = 0ULL;
            for (auto it = begin; it != end; ++it) {
                auto piece = *it;
                assert(0 <= piece);

                auto slide = kSlideIndex[index];
                value += static_cast<uint64_t>(piece) << slide;
                index += 1;
            }

            assert(index <= 16);

            value += (0b10000ULL << kSlideIndex[index]) - 1ULL;

            return value;
        }

        template<typename Container>
        static PiecesValue create(const Container &pieces) {
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
            auto vector = std::vector<core::PieceType>{};
            vector.reserve(size_);
            push(vector);
            return vector;
        }

        std::list<core::PieceType> list() const {
            auto list = std::list<core::PieceType>{};
            push(list);
            return list;
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

        template<class Container>
        void push(Container &container) const {
            for (int index = 0; index < size_; ++index) {
                uint64_t piece = (value_ >> kSlideIndex[index]) & 0b1111ULL;
                container.push_back(static_cast<core::PieceType>(piece));
            }
        }
    };
}

#endif //SFINDER_EXP_LIB_PIECES_HPP

#ifndef SFINDER_EXP_LIB_PARSE_HPP
#define SFINDER_EXP_LIB_PARSE_HPP

#include <cstring>

#include "core/types.hpp"
#include "core/bits.hpp"

namespace sfexp {
    core::PieceType parseToPiece(char ch) {
        switch (ch) {
            case 'T':
                return core::PieceType::T;
            case 'I':
                return core::PieceType::I;
            case 'L':
                return core::PieceType::L;
            case 'J':
                return core::PieceType::J;
            case 'S':
                return core::PieceType::S;
            case 'Z':
                return core::PieceType::Z;
            case 'O':
                return core::PieceType::O;
            default:
                assert(false);
                return static_cast<core::PieceType>(-1);
        }
    }

    core::RotateType parseToRotate(char ch) {
        switch (ch) {
            case '0':
                return core::RotateType::Spawn;
            case 'L':
                return core::RotateType::Left;
            case 'R':
                return core::RotateType::Right;
            case '2':
                return core::RotateType::Reverse;
            default:
                assert(false);
                return static_cast<core::RotateType>(-1);
        }
    }

    uint64_t parseToMask(std::string str) {
        uint64_t mask = 0ULL;
        for (int y = 3; 0 <= y; --y) {
            auto &ch = str[3 - y];
            if (ch == '1') {
                mask |= core::getBitKey(y);
            }
        }
        return mask;
    }
}

#endif //SFINDER_EXP_LIB_PARSE_HPP

#ifndef SFINDER_EXP_TYPES_HPP
#define SFINDER_EXP_TYPES_HPP

namespace sfexp {
    struct PieceIndex {
        int index;
        core::PieceType piece;
        core::RotateType rotate;
        int x;
        int y;
        uint64_t deletedLine;
    };

    using Solution = std::array<uint16_t, 10>;
    using SolutionVector = std::vector<uint16_t>;
}

#endif //SFINDER_EXP_TYPES_HPP

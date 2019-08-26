#include <iostream>

#include "calculater.hpp"

template<int Next, int Call>
void generate(std::list<core::PieceType> &pieces, uint64_t initNext, Calculator<Next> &calculator) {
    static_assert(0 <= Next && Next <= 10);
    static_assert(0 <= Call && Call <= Next + 1);

    if constexpr (Call == Next + 1) {
        auto field = core::Field{};
        auto copiedPieces = std::list<core::PieceType>(pieces.cbegin(), pieces.cend());
        int score = calculator.calculate(
                field, copiedPieces, core::emptyPieceType, initNext, copiedPieces.size(), 10, 4
        );
    } else {
        if (initNext == 0) {
            initNext = 0b1111111;
        }

        auto next = initNext;

        do {
            auto bit = next & (next - 1U);
            auto i = next & ~bit;
            auto nextPieceType = core::pieceBitToVal[i];
            assert(0 <= nextPieceType);

            pieces.emplace_back(nextPieceType);
            generate<Next, Call + 1>(pieces, initNext & ~i, calculator);
            pieces.pop_back();

            next = bit;
        } while (next != 0);
    }
}

template<int Next>
void generate(Calculator<Next> &calculator) {
    auto pieces = std::list<core::PieceType>{};
    generate<Next, 0>(pieces, 0, calculator);
}

int main() {
    const int Next = 10;
    auto factory = core::Factory::create();
    auto checker = CacheChecker(factory);
    auto calculator = Calculator<Next>(factory, checker);

    generate(calculator);

    return 0;
}

/**
 *
 *
        auto piecesObj = core::Pieces::create(copiedPieces);
        uint64_t value = piecesObj.value();
        file.write((char *) &value, sizeof(uint64_t));
        file.write((char *) &score, sizeof(int));


            if constexpr (Call < 2) {
                auto end = std::chrono::system_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(end - start).count();
                std::cout << factory.get(nextPieceType).name << Call << " " << elapsed << "mins" << std::endl;
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
    // auto checker = CacheChecker{};
    generate<9>();

    std::cout << "cache10 = " << cache10.size() << std::endl;
    std::cout << "cache11true = " << cache11true.size() << std::endl;
    std::cout << "cache11false = " << cache11false.size() << std::endl;

    file.close();

    return 0;
}
 */
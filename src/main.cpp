#include <list>

#include "core/bits.hpp"
#include "sfinder/marker.hpp"
#include "sfinder/permutations.hpp"
#include "sfinder/perfect_clear/full_finder.hpp"
#include "sfinder/perfect_clear/checker.hpp"

#include "core/field.hpp"
#include "core/factory.hpp"
#include "core/pieces.hpp"

#include "create.hpp"
#include "open.hpp"

int toValue(const std::vector<core::PieceType> &pieces) {
    int value = 0;
    auto it = pieces.cbegin();

    value += *it;
    ++it;

    for (; it != pieces.cend(); ++it) {
        value *= 7;
        value += *it;
    }

    return value;
}

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
            sfinder::Marker &marker, core::srs::MoveGenerator &moveGenerator
    )
            : factory_(factory), field_(field),
              marker_(marker), moveGenerator_(moveGenerator), currentOperations_(std::vector<core::PieceType>{}) {
    }

    void find(std::vector<sfexp::PieceIndex> &operationWithKeys) {
        auto field = core::Field{field_};
        currentOperations_.clear();
        find(field, operationWithKeys, (1U << operationWithKeys.size()) - 1U);
    }

private:
    const core::Factory &factory_;
    const core::Field field_;

    sfinder::Marker &marker_;
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
                        assert(currentOperations_.size() == 10);

                        auto value = toValue(currentOperations_);
                        assert(0 <= value && value < 7 * 7 * 7 * 7 * 7 * 7 * 7 * 7 * 7 * 7);

                        marker_.set(value, true);
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

void create() {
    sfexp::createIndexes("../../csv/index.csv", "../../bin/index.bin");
    sfexp::createSolutions("../../csv/indexed_solutions_10x4_SRS7BAG.csv", "../../bin/solutionsrs.bin");
}

void find() {
    auto indexes = std::vector<sfexp::PieceIndex>{};
    sfexp::openIndexes("../../bin/index.bin", indexes);
    std::cout << indexes.size() << std::endl;

    auto solutions = std::vector<sfexp::Solution>{};
    sfexp::openSolutions("../../bin/solutionsrs.bin", solutions);
    std::cout << solutions.size() << std::endl;

    const int max = 7 * 7 * 7 * 7 * 7 * 7 * 7 * 7 * 7 * 7;
    auto marker = sfinder::Marker::create(max);

    const std::vector<core::PieceType> sampling = std::vector{
            core::PieceType::T, core::PieceType::L, core::PieceType::J, core::PieceType::Z, core::PieceType::I,
            core::PieceType::O, core::PieceType::S, core::PieceType::I, core::PieceType::L, core::PieceType::T,
    };
    auto PIECE_COUNTER = core::PieceCounter::create(sampling);

    // Runner
    auto factory = core::Factory::create();
    auto moveGenerator = core::srs::MoveGenerator(factory);
    auto currentOperations = std::vector<core::PieceType>{};
    auto field = core::Field{};
    auto runner = Runner(factory, field, marker, moveGenerator);

    // run
    auto start = std::chrono::system_clock::now();

    auto point = 10;
    for (int i = 0; i < solutions.size(); ++i) {
        auto &solution = solutions[i];

        auto vector = std::vector<sfexp::PieceIndex>{};
        auto pieces = std::vector<core::PieceType>{};
        for (const auto &index : solution) {
            auto pieceIndex = indexes[index];
            vector.push_back(pieceIndex);
            pieces.push_back(pieceIndex.piece);
        }

        auto pieceCounter = core::PieceCounter::create(pieces);

        if (pieceCounter.value() == PIECE_COUNTER.value()) {
            runner.find(vector);

            if (i % point == point - 1) {
                auto s = i + 1;
                auto elapsed = std::chrono::system_clock::now() - start;
                auto count = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
                auto average = count / s;
                std::cout << "index=" << s << " remind=" << average * (solutions.size() - s) << "secs" << std::endl;
            }
        }
    }

    int value = toValue(sampling);
    std::cout << value << std::endl;
    std::cout << std::boolalpha << marker.succeed(value) << std::endl;


    /*


    for (int i = 0; i < solutions.size(); ++i) {
        auto &solution = solutions[i];

        auto vector = std::vector<sfexp::PieceIndex>{};
        for (const auto &index : solution) {
            auto pieceIndex = indexes[index];
            vector.push_back(pieceIndex);
        }

        runner.find(vector);

        if (i % point == point - 1) {
            auto s = i + 1;
            auto elapsed = std::chrono::system_clock::now() - start;
            auto count = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
            auto average = count / s;
            std::cout << "index=" << s << " remind=" << average * (solutions.size() - s) << "secs" << std::endl;
        }
    }

    // Output
    auto fout = std::ofstream("output.bin", std::ios::out | std::ios::binary | std::ios::trunc);
    if (!fout) {
        std::cout << "Cannot create output file";
        return;
    }

    auto &flags = marker.flags();
    std::copy(flags.cbegin(), flags.cend(), std::ostreambuf_iterator<char>(fout));

    fout.close();
    */
}

void load() {
    auto fin = std::ifstream("output.bin", std::ios::in | std::ifstream::binary);
    if (!fin) {
        std::cout << "Cannot create input file";
        return;
    }

    auto iter = std::istreambuf_iterator(fin);
    auto end = std::istreambuf_iterator<char>();

    std::vector<sfinder::Marker::FlagType> newVector{};
    std::copy(iter, end, std::back_inserter(newVector));

    auto factory = core::Factory::create();
    auto marker = sfinder::Marker(newVector);
    auto moveGenerator = core::srs::MoveGenerator(factory);
    auto finder = sfinder::perfect_clear::Checker<core::srs::MoveGenerator>(factory, moveGenerator);

    // Create permutations
    auto permutation = std::vector<sfinder::Permutation>{
            sfinder::Permutation::create<7>(core::kAllPieceType, 7),
            sfinder::Permutation::create<7>(core::kAllPieceType, 3),
    };
    auto permutations = sfinder::Permutations::create(permutation);

    int s = 0;
    int f = 0;
    for (int index = 0; index < permutations.size(); ++index) {
        auto pieces = permutations.pieces(index);
        auto value = toValue(pieces);
        if (!marker.succeed(value)) {
            for (const auto &piece : pieces) {
                std::cout << factory.get(piece).name;
            }

            auto success = finder.run<false, true>(core::Field{}, pieces, 10, 4, true);
            std::cout << " " << std::boolalpha << success << std::endl;

            f++;
        } else {
            s++;
        }
    }

    std::cout << s << std::endl;
    std::cout << f << std::endl;

}

int main() {
//    create();
    find();
//    load();
}

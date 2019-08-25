#ifndef SFINDER_CPP_EXP_CACHE_HPP
#define SFINDER_CPP_EXP_CACHE_HPP

#include <map>

#include "core/types.hpp"
#include "core/pieces.hpp"
#include "core/field.hpp"

struct CacheKey {
    core::Bitboard field;
    core::PieceType hold;
    uint64_t pieces;
    int line;
};

bool operator<(const CacheKey &lhs, const CacheKey &rhs) {
    if (lhs.field != rhs.field) {
        return lhs.field < rhs.field;
    }

    if (lhs.hold != rhs.hold) {
        return lhs.hold < rhs.hold;
    }

    if (lhs.pieces < rhs.pieces) {
        return lhs.pieces < rhs.pieces;
    }

    return lhs.line < rhs.line;
}

class Cache {
public:
    void add(core::Field field, core::PieceType hold, core::Pieces pieces, int line, int result) {
        caches_.insert(std::make_pair(CacheKey{field.boardLow(), hold, pieces.value(), line}, result));
    }

    int get(core::Field field, core::PieceType hold, core::Pieces pieces, int line) {
        auto result = caches_.find(CacheKey{field.boardLow(), hold, pieces.value(), line});
        if (result != caches_.end()) {
            return result->second;
        }
        return -1;
    }

    int size() const {
        return caches_.size();
    }

private:
    std::map<CacheKey, int> caches_{};
};

#endif //SFINDER_CPP_EXP_CACHE_HPP

#pragma once
#include <vector>
#include <cstdint>
#include "bitboards.h"

struct GroundND {
    int dimensions;
    int size;
    int total_cells;
    int blocks;
    int revealed_cells = 0;

    std::vector<uint64_t> bombs;

    StateBoard state;
    DynamicBitBoard counts;

    void init(int dim, int s);
    void buildCache();

    void reveal(int startIndex);
    void clear();

    inline void setBomb(int index) {
        bombs[index >> 6] |= (1ULL << (index & 63));
    }

    inline bool getBomb(int index) const {
        return (bombs[index >> 6] >> (index & 63)) & 1ULL;
    }
};

void generateBombsExact(GroundND& ground, int numMines);
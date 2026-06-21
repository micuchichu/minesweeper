#pragma once
#include <vector>
#include <cstdint>
#include "bitboards.h"

#include <random>
#include <bit>

struct GroundND {
    uint8_t dimensions;
    uint64_t size;
    uint64_t total_cells;
    uint64_t blocks;
    uint64_t revealed_cells = 0;

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

inline void generateBombsExact(GroundND& ground, size_t numMines) {
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<unsigned long long> dist(0, ground.total_cells - 1);

    size_t placed = 0;
    while (placed < numMines) {
        size_t index = (size_t)dist(rng);
        if (!ground.getBomb(index)) {
            ground.setBomb(index);
            placed++;
        }
    }
}
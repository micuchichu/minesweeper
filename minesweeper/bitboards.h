#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <random>
#include <bit>
#include <algorithm>

struct StateBoard {
    std::vector<uint64_t> data;

    void init(size_t total_cells) {
        data.assign(((total_cells + 31) / 32) + 1, 0ULL);
    }

    inline void set(size_t index, uint8_t state) {
        size_t block = index / 32;
        if (block >= data.size()) return;
        size_t offset = (index % 32) * 2;

        data[block] &= ~(3ULL << offset);
        data[block] |= ((uint64_t)state << offset);
    }

    inline uint8_t get(size_t index) const {
        size_t block = index / 32;
        if (block >= data.size()) return 0;
        size_t offset = (index % 32) * 2;

        return (data[block] >> offset) & 3ULL;
    }

    inline size_t countRevealed() const {
        size_t count = 0;
        for (uint64_t block : data) {
            uint64_t lower = block & 0x5555555555555555ULL;
            uint64_t upper = (block >> 1) & 0x5555555555555555ULL;

            uint64_t revealed = lower & ~upper;

            count += std::popcount(revealed);
        }
        return count;
    }
};

struct DynamicBitBoard {
    std::vector<uint64_t> data;
    size_t bpc;
    uint64_t mask;

    void init(size_t total_cells, size_t bits_per_cell) {
        bpc = bits_per_cell;
        mask = (1ULL << bpc) - 1;
        data.assign((((total_cells * bpc) + 63) / 64) + 2, 0ULL);
    }

    inline void increment(size_t index) {
        uint64_t current = get(index);
        set(index, current + 1);
    }

    inline void set(size_t index, uint64_t val) {
        size_t bit_pos = index * bpc;
        size_t block = bit_pos / 64;
        if (block >= data.size()) return;

        size_t offset = bit_pos % 64;

        if (offset + bpc <= 64) {
            data[block] &= ~(mask << offset);
            data[block] |= (val & mask) << offset;
        }
        else {
            if (block + 1 >= data.size()) return;

            size_t bits_first = 64 - offset;
            size_t bits_second = bpc - bits_first;

            uint64_t mask_first = (1ULL << bits_first) - 1;
            data[block] &= ~(mask_first << offset);
            data[block] |= (val & mask_first) << offset;

            uint64_t mask_second = (1ULL << bits_second) - 1;
            data[block + 1] &= ~mask_second;
            data[block + 1] |= (val >> bits_first);
        }
    }

    inline uint64_t get(size_t index) const {
        size_t bit_pos = index * bpc;
        size_t block = bit_pos / 64;
        if (block >= data.size()) return 0;

        size_t offset = bit_pos % 64;

        if (offset + bpc <= 64) {
            return (data[block] >> offset) & mask;
        }
        else {
            if (block + 1 >= data.size()) return 0;

            size_t bits_first = 64 - offset;
            uint64_t val_first = (data[block] >> offset) & ((1ULL << bits_first) - 1);
            uint64_t val_second = data[block + 1] & ((1ULL << (bpc - bits_first)) - 1);

            return val_first | (val_second << bits_first);
        }
    }
};
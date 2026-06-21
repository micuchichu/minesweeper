#include "engine.h"

#include <algorithm>
#include <iostream>

void GroundND::init(int dim, int s) {
    dimensions = dim;
    size = s;
    total_cells = 1;
    for (int i = 0; i < dim; i++) total_cells *= size;

    blocks = (total_cells + 63) / 64;
    bombs.assign(blocks, 0ULL);

    state.init(total_cells);

    revealed_cells = 0;

    int bits_per_cell = 4;
    if (dim == 3) bits_per_cell = 5;
    if (dim == 4) bits_per_cell = 7;
    if (dim == 5) bits_per_cell = 8;
    if (dim >= 6) bits_per_cell = 10;

    counts.init(total_cells, bits_per_cell);
}

void GroundND::clear() {
    std::fill(bombs.begin(), bombs.end(), 0ULL);
    std::fill(state.data.begin(), state.data.end(), 0ULL);
    std::fill(counts.data.begin(), counts.data.end(), 0ULL);
}

void GroundND::reveal(int startIndex) {
    if (state.get(startIndex) != 0) return;

    std::vector<size_t> stack;
    stack.push_back(startIndex);
    state.set(startIndex, 1);

    size_t S1 = size;
    size_t S2 = size * size;
    size_t S3 = size * size * size;

    while (!stack.empty()) {
        size_t index = stack.back();
        stack.pop_back();

        if (counts.get(index) > 0 || getBomb(index)) continue;

        size_t x = index % S1;
        size_t y = (index / S1) % S1;
        size_t z = dimensions >= 3 ? (index / S2) % S1 : 0;
        size_t w = dimensions == 4 ? (index / S3) : 0;

        size_t minW = (dimensions == 4 && w > 0) ? w - 1 : w;
        size_t maxW = (dimensions == 4 && w < S1 - 1) ? w + 1 : w;
        size_t minZ = (dimensions >= 3 && z > 0) ? z - 1 : z;
        size_t maxZ = (dimensions >= 3 && z < S1 - 1) ? z + 1 : z;
        size_t minY = (y > 0) ? y - 1 : y;
        size_t maxY = (y < S1 - 1) ? y + 1 : y;
        size_t minX = (x > 0) ? x - 1 : x;
        size_t maxX = (x < S1 - 1) ? x + 1 : x;

        for (size_t nw = minW; nw <= maxW; nw++) {
            size_t wBase = nw * S3;
            for (size_t nz = minZ; nz <= maxZ; nz++) {
                size_t zBase = wBase + nz * S2;
                for (size_t ny = minY; ny <= maxY; ny++) {
                    size_t yBase = zBase + ny * S1;
                    for (size_t nx = minX; nx <= maxX; nx++) {
                        size_t nIndex = yBase + nx;

                        if (state.get(nIndex) == 0) {
                            state.set(nIndex, 1);

                            if (counts.get(nIndex) == 0 && !getBomb(nIndex)) {
                                stack.push_back(nIndex);
                            }
                        }
                    }
                }
            }
        }
    }
    revealed_cells = state.countRevealed();
}

void GroundND::buildCache() {
    size_t S1 = size;
    size_t S2 = size * size;
    size_t S3 = size * size * size;

    for (size_t b = 0; b < blocks; b++) {
        uint64_t blockBombs = bombs[b];

        while (blockBombs != 0) {
            int bitOffset = std::countr_zero(blockBombs);
            size_t index = (b * 64) + bitOffset;

            size_t x = index % S1;
            size_t y = (index / S1) % S1;
            size_t z = dimensions >= 3 ? (index / S2) % S1 : 0;
            size_t w = dimensions == 4 ? (index / S3) : 0;

            size_t minW = (dimensions == 4 && w > 0) ? w - 1 : w;
            size_t maxW = (dimensions == 4 && w < S1 - 1) ? w + 1 : w;
            size_t minZ = (dimensions >= 3 && z > 0) ? z - 1 : z;
            size_t maxZ = (dimensions >= 3 && z < S1 - 1) ? z + 1 : z;
            size_t minY = (y > 0) ? y - 1 : y;
            size_t maxY = (y < S1 - 1) ? y + 1 : y;
            size_t minX = (x > 0) ? x - 1 : x;
            size_t maxX = (x < S1 - 1) ? x + 1 : x;

            for (size_t nw = minW; nw <= maxW; nw++) {
                size_t wBase = nw * S3;
                for (size_t nz = minZ; nz <= maxZ; nz++) {
                    size_t zBase = wBase + nz * S2;
                    for (size_t ny = minY; ny <= maxY; ny++) {
                        size_t yBase = zBase + ny * S1;
                        for (size_t nx = minX; nx <= maxX; nx++) {
                            size_t nIndex = yBase + nx;
                            if (nIndex == index) continue;

                            counts.increment(nIndex);
                        }
                    }
                }
            }
            blockBombs &= (blockBombs - 1);
        }
    }
}


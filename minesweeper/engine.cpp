#include "engine.h"
#include <random>
#include <bit>
#include <algorithm>

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
     
    std::vector<int> stack;
    stack.push_back(startIndex);
    state.set(startIndex, 1);

    int S1 = size, S2 = size * size, S3 = size * size * size;

    while (!stack.empty()) {
        int index = stack.back();
        stack.pop_back();

        if (counts.get(index) > 0 || getBomb(index)) continue;

        int x = index % S1;
        int y = (index / S1) % S1;
        int z = dimensions >= 3 ? (index / S2) % S1 : 0;
        int w = dimensions == 4 ? (index / S3) : 0;

        int minW = (dimensions == 4 && w > 0) ? w - 1 : w;
        int maxW = (dimensions == 4 && w < S1 - 1) ? w + 1 : w;
        int minZ = (dimensions >= 3 && z > 0) ? z - 1 : z;
        int maxZ = (dimensions >= 3 && z < S1 - 1) ? z + 1 : z;
        int minY = (y > 0) ? y - 1 : y;
        int maxY = (y < S1 - 1) ? y + 1 : y;
        int minX = (x > 0) ? x - 1 : x;
        int maxX = (x < S1 - 1) ? x + 1 : x;

        for (int nw = minW; nw <= maxW; nw++) {
            int wBase = nw * S3;
            for (int nz = minZ; nz <= maxZ; nz++) {
                int zBase = wBase + nz * S2;
                for (int ny = minY; ny <= maxY; ny++) {
                    int yBase = zBase + ny * S1;
                    for (int nx = minX; nx <= maxX; nx++) {

                        int nIndex = yBase + nx;

                        if (state.get(nIndex) == 0) {
                            state.set(nIndex, 1);
                            revealed_cells++;

                            if (counts.get(nIndex) == 0 && !getBomb(nIndex)) {
                                stack.push_back(nIndex);
                            }
                        }
                    }
                }
            }
        }
    }
}

void GroundND::buildCache() {
    int S1 = size, S2 = size * size, S3 = size * size * size;

    for (int b = 0; b < blocks; b++) {
        uint64_t blockBombs = bombs[b];

        while (blockBombs != 0) {
            int bitOffset = std::countr_zero(blockBombs);
            int index = (b * 64) + bitOffset;

            int x = index % S1;
            int y = (index / S1) % S1;
            int z = dimensions >= 3 ? (index / S2) % S1 : 0;
            int w = dimensions == 4 ? (index / S3) : 0;

            int minW = (dimensions == 4 && w > 0) ? w - 1 : w;
            int maxW = (dimensions == 4 && w < S1 - 1) ? w + 1 : w;
            int minZ = (dimensions >= 3 && z > 0) ? z - 1 : z;
            int maxZ = (dimensions >= 3 && z < S1 - 1) ? z + 1 : z;
            int minY = (y > 0) ? y - 1 : y;
            int maxY = (y < S1 - 1) ? y + 1 : y;
            int minX = (x > 0) ? x - 1 : x;
            int maxX = (x < S1 - 1) ? x + 1 : x;

            for (int nw = minW; nw <= maxW; nw++) {
                int wBase = nw * S3;
                for (int nz = minZ; nz <= maxZ; nz++) {
                    int zBase = wBase + nz * S2;
                    for (int ny = minY; ny <= maxY; ny++) {
                        int yBase = zBase + ny * S1;
                        for (int nx = minX; nx <= maxX; nx++) {

                            int nIndex = yBase + nx;
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

void generateBombsExact(GroundND& ground, int numMines) {
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, ground.total_cells - 1);

    int placed = 0;
    while (placed < numMines) {
        int index = dist(rng);
        if (!ground.getBomb(index)) {
            ground.setBomb(index);
            placed++;
        }
    }
}
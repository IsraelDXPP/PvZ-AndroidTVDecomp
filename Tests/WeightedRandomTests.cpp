#include "MTRand.h"
#include "ReconHelpers.h"
#include "ReconFreeFunctions.h"

#include <array>
#include <cstdio>
#include <random>

bool RunWeightedRandomTests()
{
    Sexy::MTRand random(4357UL);
    std::mt19937 reference(4357U);
    for (int draw = 0; draw < 2000; ++draw)
    {
        if (random.Next() != (reference() & 2147483647U))
        {
            fprintf(stderr, "FAIL original 31-bit MT sequence at draw %d\n", draw);
            return false;
        }
    }
    std::array<TodWeightedGridArray, 30> cells{};
    std::array<int, 30> visits{};
    for (int index = 0; index < 30; ++index)
        cells[index] = {3 + index / 5, index % 5, 100000};
    for (int draw = 0; draw < 10000; ++draw)
    {
        const TodWeightedGridArray* picked = TodPickFromWeightedGridArray(cells.data(), 30);
        ++visits[(picked->mX - 3) * 5 + picked->mY];
    }
    for (int count : visits)
    {
        if (count < 200 || count > 500)
        {
            fprintf(stderr, "FAIL grave candidate coverage: %d\n", count);
            return false;
        }
    }
    for (int placed = 0; placed < 9; ++placed)
    {
        TodWeightedGridArray* picked = TodPickFromWeightedGridArray(cells.data(), 30);
        if (picked->mWeight == 0)
            return false;
        fprintf(stderr, "GRAVE %d,%d\n", picked->mX, picked->mY);
        picked->mWeight = 0;
    }
    fprintf(stderr, "PASS original MT sequence and large-weight grave selection\n");
    return true;
}

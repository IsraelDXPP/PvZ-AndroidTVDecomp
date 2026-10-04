#include "PottedPlant.h"

#include <cstring>

void PottedPlant::InitializePottedPlant(SeedType theSeedType)
{
    std::memset(this, 0, sizeof(PottedPlant));
    mSeedType = theSeedType;
    mDrawVariation = 0;
    mFacing = (FacingDirection)RandRangeInt((int)FACING_RIGHT, (int)FACING_LEFT);
    mPlantAge = (PottedPlantAge)0;
    mTimesFed = 0;
    mWhichZenGarden = (GardenType)0;
    mFeedingsPerGrow = RandRangeInt(3, 5);
    mPlantNeed = (PottedPlantNeed)0;
    mLastWateredTime = 0;
    mLastNeedFulfilledTime = 0;
    mLastFertilizedTime = 0;
    mLastChocolateTime = 0;
}

#include "Level.h"

Sexy::Level::Level()
    : mGameMode(-1),
      mLevelId(-1),
      mScene(-1),
      mInitialSun(0),
      mSeedBankLimit(-1),
      mFogColumns(0),
      mFlagWaveCount(0),
      mZombiesVisible(true),
      mWinType(0)
{
}

int Sexy::Level::InitFromXML(int theMode, int theLevelId, char const* thePath)
{
    (void)theMode;
    (void)theLevelId;
    (void)thePath;
    return 1;
}

int Sexy::Level::hasPlantOnPosition(int theX, int theY)
{
    for (size_t aIndex = 0; aIndex < mPlants.size(); ++aIndex)
    {
        const LevelPlantInfo& aPlant = mPlants[aIndex];
        if (aPlant.mX == theX && aPlant.mY == theY)
            return 1;
    }
    return 0;
}

int Sexy::Level::getPlantIdOnPosition(int theX, int theY)
{
    for (size_t aIndex = 0; aIndex < mPlants.size(); ++aIndex)
    {
        const LevelPlantInfo& aPlant = mPlants[aIndex];
        if (aPlant.mX == theX && aPlant.mY == theY)
            return aPlant.mPlantId;
    }
    return -1;
}

int Sexy::Level::isSeedForbidden(int theSeed)
{
    for (size_t aIndex = 0; aIndex < mForbiddenSeeds.size(); ++aIndex)
    {
        if (mForbiddenSeeds[aIndex] == theSeed)
            return 1;
    }
    return 0;
}

int Sexy::Level::getFlagWaveCount()
{
    return mFlagWaveCount;
}

int Sexy::Level::getWaveCount()
{
    return (int)mWaves.size();
}

int Sexy::Level::isCardNotAllowedToPick(int theSeed)
{
    return isSeedForbidden(theSeed);
}

int Sexy::Level::reset()
{
    mAllowedZombies.clear();
    mPlants.clear();
    mForbiddenSeeds.clear();
    mSeedConfig.clear();
    mTombs.clear();
    mWinConditions.clear();
    mWaves.clear();

    mGameMode = -1;
    mLevelId = -1;
    mScene = -1;
    mSeedBankLimit = -1;
    mInitialSun = 0;
    mFogColumns = 0;
    mFlagWaveCount = 0;
    mWinType = 0;
    mZombiesVisible = true;
    return 0;
}

int Sexy::Level::getWavePoint(int theWave)
{
    std::map<int, LevelWave>::const_iterator aFound = mWaves.find(theWave);
    if (aFound == mWaves.end())
        return 0;
    return aFound->second.mPoints;
}

int Sexy::Level::isFlagWave(int theWave)
{
    std::map<int, LevelWave>::const_iterator aFound = mWaves.find(theWave);
    if (aFound == mWaves.end())
        return 0;
    return aFound->second.mFlag ? 1 : 0;
}

bool Sexy::Level::hasMustHaveZombieInWave(int theWave)
{
    std::map<int, LevelWave>::const_iterator aFound = mWaves.find(theWave);
    if (aFound == mWaves.end())
        return false;
    return !aFound->second.mMustHave.empty();
}

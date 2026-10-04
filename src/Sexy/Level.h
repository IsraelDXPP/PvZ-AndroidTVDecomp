#pragma once

#include "ReconTypes.h"

#include <map>
#include <vector>

// One plant the level file places before the intro. The attribute is
// initPlantSetting, written as "[column row seed]".
struct LevelPlantInfo
{
    int mX;
    int mY;
    int mPlantId;
};

// One wave in the level file. The map key is the wave index.
struct LevelWave
{
    int mPoints;
    bool mFlag;
    std::vector<int> mMustHave;

    LevelWave()
        : mPoints(0), mFlag(false)
    {
    }
};

namespace Sexy {

// The stage record loaded from properties/levels.xml. Callers ask it which
// plants are already on the lawn, which seeds the player may not pick, and
// what each wave spends.
class Level
{
public:
    int mGameMode;
    int mLevelId;
    int mScene;
    std::vector<int> mAllowedZombies;
    int mInitialSun;
    std::vector<LevelPlantInfo> mPlants;
    std::vector<int> mForbiddenSeeds;
    std::vector<TombConfig> mTombs;
    int mSeedBankLimit;
    std::vector<int> mSeedConfig;
    int mFogColumns;
    int mFlagWaveCount;
    bool mZombiesVisible;
    int mWinType;
    std::vector<int> mWinConditions;
    std::map<int, LevelWave> mWaves;

    Level();
    int InitFromXML(int theMode, int theLevelId, char const* thePath);
    int hasPlantOnPosition(int theX, int theY);
    int getPlantIdOnPosition(int theX, int theY);
    int isSeedForbidden(int theSeed);
    int getFlagWaveCount();
    int getWaveCount();
    int isCardNotAllowedToPick(int theSeed);
    int reset();
    int getWavePoint(int theWave);
    int isFlagWave(int theWave);
    bool hasMustHaveZombieInWave(int theWave);
};

} // namespace Sexy

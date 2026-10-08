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
    bool mHasPoints;
    bool mHasFlag;
    int mNextWaveDelay;
    std::vector<ZombieType> mMustHave;
    std::vector<ZombieType> mAfterNativeZombies;

    LevelWave()
        : mPoints(0), mFlag(false), mHasPoints(false), mHasFlag(false), mNextWaveDelay(-1)
    {
    }
};

namespace Sexy {

enum class LevelSpawnRules { Legacy, Native };
enum class LevelFlagRules { Legacy, Native, Xml };

struct LevelLoadContext
{
    GameMode mGameMode = GAMEMODE_ADVENTURE;
    int mLevelId = 0;
    int mSurvivalStage = 0;
    bool mAdventureComplete = false;
};

// The stage record loaded from properties/levels.xml. Callers ask it which
// plants are already on the lawn, which seeds the player may not pick, and
// what each wave spends.
class Level
{
public:
    GameMode mGameMode;
    int mLevelId;
    BackgroundType mScene;
    std::vector<ZombieType> mAllowedZombies;
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

    bool mLoaded;
    bool mHasScene;
    bool mHasInitialSun;
    bool mHasAllowedZombies;
    bool mHasWaveCount;
    int mWaveCount;
    int mSurvivalStage;
    bool mAdventureComplete;
    LevelSpawnRules mSpawnRules;
    LevelFlagRules mFlagRules;
    bool mSkipShopSeedTutorial;
    bool mApplyNativeFlagWave;
    bool mApplyNativePointMultiplier;
    bool mApplyNativeIntroductions;
    bool mApplyNativeLastWave;
    int mFirstWaveDelay;
    String mConfigurationError;

    Level();
    bool InitFromXML(GameMode theMode, int theLevelId, char const* thePath);
    bool InitFromXML(const LevelLoadContext& theContext, const String& thePath);
    bool MatchesContext(const LevelLoadContext& theContext) const;
    const LevelWave* FindWave(int theWave) const;
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

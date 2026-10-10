#ifndef LAWNPROJECTTV_SEXY_INCLUDE_LEVEL_H
#define LAWNPROJECTTV_SEXY_INCLUDE_LEVEL_H

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

enum class LevelWaveActionType
{
    Scale,
    Spawn,
    Missing,
    Fill
};

struct LevelWaveAction
{
    LevelWaveActionType mType = LevelWaveActionType::Fill;
    std::vector<ZombieType> mZombies;
    int mCount = 1;
    int mBudgetCountLimit = -1;
    int mNumerator = 1;
    int mDenominator = 1;
    bool mFlagOnly = false;
    bool mLastOnly = false;
    bool mYetiUnkilledOnly = false;
};

struct LevelExtraSpawns
{
    bool mGraves = false;
    bool mGraveCountdownChance = false;
    bool mGravesOnFlag = false;
    int mGraveMinimumAge = 100;
    int mPoolCount = 0;
    int mPoolPoints = 0;
    int mSkyCount = 0;
    int mSkyPoints = 0;
    bool mSkyEveryWave = false;
    ZombieType mCarrier = ZOMBIE_INVALID;
    std::vector<ZombieType> mTypes;
    std::vector<ZombieType> mBobsledFallback;
};

struct LevelWhackStage
{
    int mTwoCountWeight = 0;
    int mThreeCountWeight = 0;
    std::vector<ZombieType> mTypes;
    std::vector<int> mWeights;
    std::vector<ZombieType> mThreeTypes;
    std::vector<int> mThreeWeights;
};

struct LevelWhackSpawns
{
    std::vector<LevelWhackStage> mStages;
    int mRollRange = 30;
    int mWaveDelay = 2000;
    int mInitialGraves = 9;
    int mGraveTarget = 5;
    int mFinalCount = 20;
    int mFirstMinimumDelay = 100;
    int mFirstMaximumDelay = 200;
    int mLastMinimumDelay = 30;
    int mLastMaximumDelay = 60;
    int mDelayEndWave = 12;
    std::vector<ZombieType> mFinalTypes;
};

struct LevelBossSpawn
{
    int mMaximumAge = 1000000;
    int mRow = -1;
    std::vector<ZombieType> mTypes;
};

struct LevelSummons
{
    ZombieType mBackupDancer = ZOMBIE_INVALID;
    ZombieType mImp = ZOMBIE_INVALID;
    ZombieType mBossBungee = ZOMBIE_INVALID;
    int mBossBungeeCount = 0;
    ZombieType mAquarium = ZOMBIE_INVALID;
    ZombieType mSquirrel = ZOMBIE_INVALID;
    ZombieType mBobsled = ZOMBIE_INVALID;
};

struct LevelVaseContents
{
    ScaryPotType mKind = SCARYPOT_ZOMBIE;
    ZombieType mZombie = ZOMBIE_INVALID;
    SeedType mPlant = SEED_NONE;
    int mCount = 0;
    int mStageDelta = 0;
    int mStageDivisor = 1;
    int mMaximumCount = 45;
};

struct LevelVaseLayout
{
    int mMinimumStage = 0;
    int mMaximumStage = 1000000;
    int mLeafCount = 0;
    std::vector<int> mExcludedColumns;
    std::vector<LevelVaseContents> mContents;
};

struct LevelMoundSpawn
{
    int mMinimumLevel = 0;
    int mMaximumLevel = 1000000;
    std::vector<ZombieType> mTypes;
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
    std::vector<LevelWaveAction> mActions;
    bool mHasActions = false;

    LevelWave() :
        mPoints(0),
        mFlag(false),
        mHasPoints(false),
        mHasFlag(false),
        mNextWaveDelay(-1)
    {
    }
};

namespace Sexy
{

enum class LevelSpawnRules
{
    Legacy,
    Native,
    Xml
};
enum class LevelFlagRules
{
    Legacy,
    Native,
    Xml
};

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
    // An explicit value overrides the profile for this level only:
    // true skips the shop tutorial, false enables it; omission uses the profile.
    bool mHasSkipShopSeedTutorial;
    bool mSkipShopSeedTutorial;
    std::vector<LevelWaveAction> mActions;
    LevelExtraSpawns mExtraSpawns;
    LevelWhackSpawns mWhackSpawns;
    std::vector<LevelBossSpawn> mBossSpawns;
    LevelSummons mSummons;
    std::vector<LevelVaseLayout> mVaseLayouts;
    std::vector<LevelMoundSpawn> mMoundSpawns;
    ZombieType mBossType = ZOMBIE_INVALID;
    bool mAllowYeti = false;
    bool mSurvivalAllowed = false;
    int mPointNumerator = 1;
    int mPointDenominator = 3;
    int mPointOffset = 1;
    int mPointIndexOffset = 0;
    int mPointStageStride = 0;
    int mFirstWaveDelay;
    String mConfigurationError;

    Level();
    bool InitFromXML(GameMode theMode, int theLevelId, char const *thePath);
    bool InitFromXML(const LevelLoadContext &theContext, const String &thePath);
    bool MatchesContext(const LevelLoadContext &theContext) const;
    const LevelWave *FindWave(int theWave) const;
    int hasPlantOnPosition(int theX, int theY);
    int getPlantIdOnPosition(int theX, int theY);
    int isSeedForbidden(int theSeed);
    int getFlagWaveCount();
    int getWaveCount() const;
    int isCardNotAllowedToPick(int theSeed);
    int reset();
    int getWavePoint(int theWave);
    int isFlagWave(int theWave);
    bool hasMustHaveZombieInWave(int theWave);
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_LEVEL_H

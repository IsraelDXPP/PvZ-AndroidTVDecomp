#include "Level.h"
#include "XMLParser.h"
#include "XMLElement.h"

#include <algorithm>
#include <charconv>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
constexpr int kMaximumWaves = 100;
constexpr int kMaximumZombiesPerWave = 50;
constexpr int kMaximumDelay = 600000;

int ParseInteger(const Sexy::String& theText, int theMinimum, int theMaximum)
{
    std::string text = theText.c_str();
    const size_t first = text.find_first_not_of(" \t\r\n");
    const size_t last = text.find_last_not_of(" \t\r\n");
    if (first == std::string::npos)
        throw std::runtime_error("expected an integer");
    text = text.substr(first, last - first + 1);
    int value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size()
        || value < theMinimum || value > theMaximum)
        throw std::runtime_error("integer outside the supported range: " + text);
    return value;
}

void ReadInteger(const Sexy::XMLElement& theElement, const char* theName,
                 int& theValue, int theMinimum, int theMaximum)
{
    if (theElement.HasAttribute(theName))
        theValue = ParseInteger(theElement.GetAttribute(theName), theMinimum, theMaximum);
}

bool ParseBoolean(const Sexy::String& theText)
{
    if (theText == "1" || theText == "true")
        return true;
    if (theText == "0" || theText == "false")
        return false;
    throw std::runtime_error("expected true, false, 1 or 0");
}

void ReadBoolean(const Sexy::XMLElement& theElement, const char* theName, bool& theValue)
{
    if (theElement.HasAttribute(theName))
        theValue = ParseBoolean(theElement.GetAttribute(theName));
}

std::vector<int> ReadIntegerList(const Sexy::String& theText, int theMinimum, int theMaximum)
{
    std::istringstream stream(theText.c_str());
    std::vector<int> values;
    std::string token;
    while (stream >> token)
        values.push_back(ParseInteger(token.c_str(), theMinimum, theMaximum));
    return values;
}

std::vector<ZombieType> ReadZombieList(const Sexy::String& theText)
{
    std::vector<ZombieType> zombies;
    for (int type : ReadIntegerList(theText, 0, NUM_ZOMBIE_TYPES - 1))
        zombies.push_back(static_cast<ZombieType>(type));
    return zombies;
}

bool MatchesNumber(const Sexy::String& theSelector, int theValue, int theMaximum, int& theScore)
{
    if (theSelector == "*")
        return true;
    const std::string selector = theSelector.c_str();
    const size_t separator = selector.find('-');
    if (separator == std::string::npos)
    {
        theScore += 4;
        return ParseInteger(theSelector, 0, theMaximum) == theValue;
    }
    const int first = ParseInteger(selector.substr(0, separator).c_str(), 0, theMaximum);
    const int last = ParseInteger(selector.substr(separator + 1).c_str(), first, theMaximum);
    theScore += 2;
    return theValue >= first && theValue <= last;
}

bool MatchesLevel(const Sexy::XMLElement& theElement, const Sexy::LevelLoadContext& theContext,
                  int& theScore)
{
    const Sexy::String mode = theElement.HasAttribute("mode") ? theElement.GetAttribute("mode") : Sexy::String("0");
    const Sexy::String level = theElement.HasAttribute("id") ? theElement.GetAttribute("id") : Sexy::String("0");
    if (!MatchesNumber(mode, theContext.mGameMode, NUM_GAME_MODES - 1, theScore)
        || !MatchesNumber(level, theContext.mLevelId, 10000, theScore))
        return false;
    if (theElement.HasAttribute("adventureCycle"))
    {
        const Sexy::String cycle = theElement.GetAttribute("adventureCycle");
        if (cycle != "any" && cycle != "first" && cycle != "repeat")
            throw std::runtime_error("unknown adventureCycle");
        if (cycle != "any")
        {
            theScore += 4;
            if ((cycle == "repeat") != theContext.mAdventureComplete)
                return false;
        }
    }
    if (theElement.HasAttribute("survivalStage")
        && !MatchesNumber(theElement.GetAttribute("survivalStage"), theContext.mSurvivalStage, 1000000, theScore))
        return false;
    int priority = 0;
    ReadInteger(theElement, "priority", priority, -10000, 10000);
    theScore += priority * 32;
    return true;
}

void ReadPlants(const Sexy::String& theText, std::vector<LevelPlantInfo>& thePlants)
{
    thePlants.clear();
    std::istringstream stream(theText.c_str());
    char open = 0;
    while (stream >> open)
    {
        LevelPlantInfo plant{};
        char close = 0;
        if (open != '[' || !(stream >> plant.mX >> plant.mY >> plant.mPlantId >> close)
            || close != ']' || plant.mX < 0 || plant.mX >= 9 || plant.mY < 0 || plant.mY >= 6
            || plant.mPlantId < 0 || plant.mPlantId >= NUM_SEED_TYPES)
            throw std::runtime_error("invalid initPlantSetting group; expected [column row seed]");
        thePlants.push_back(plant);
    }
}

void ReadTombs(const Sexy::String& theText, std::vector<Sexy::TombConfig>& theTombs)
{
    theTombs.clear();
    std::istringstream stream(theText.c_str());
    char open = 0;
    while (stream >> open)
    {
        Sexy::TombConfig tomb{};
        char close = 0;
        if (open != '[' || !(stream >> tomb.mGridX >> tomb.mGridY >> close)
            || close != ']' || tomb.mGridX < 0 || tomb.mGridX >= 9 || tomb.mGridY < 0 || tomb.mGridY >= 6)
            throw std::runtime_error("invalid initTomb group; expected [column row]");
        theTombs.push_back(tomb);
    }
}

void ReadLevelAttributes(const Sexy::XMLElement& theElement, Sexy::Level& theLevel)
{
    for (const char* obsolete : {"nativeFlagWave", "nativePointMultiplier", "nativeIntroductions", "nativeLastWave"})
        if (theElement.HasAttribute(obsolete))
            throw std::runtime_error(std::string(obsolete) + " was replaced by XML wave actions");
    if (theElement.HasAttribute("spawnRules"))
    {
        const Sexy::String rules = theElement.GetAttribute("spawnRules");
        if (rules != "native" && rules != "legacy" && rules != "xml")
            throw std::runtime_error("unknown spawnRules");
        theLevel.mSpawnRules = rules == "xml" ? Sexy::LevelSpawnRules::Xml
            : rules == "native" ? Sexy::LevelSpawnRules::Native : Sexy::LevelSpawnRules::Legacy;
        if (theLevel.mSpawnRules == Sexy::LevelSpawnRules::Native && theLevel.mFlagRules == Sexy::LevelFlagRules::Legacy)
            theLevel.mFlagRules = Sexy::LevelFlagRules::Native;
    }
    if (theElement.HasAttribute("flagRules"))
    {
        const Sexy::String rules = theElement.GetAttribute("flagRules");
        if (rules == "native") theLevel.mFlagRules = Sexy::LevelFlagRules::Native;
        else if (rules == "xml") theLevel.mFlagRules = Sexy::LevelFlagRules::Xml;
        else if (rules == "legacy") theLevel.mFlagRules = Sexy::LevelFlagRules::Legacy;
        else throw std::runtime_error("unknown flagRules");
    }
    if (theElement.HasAttribute("waveCount"))
    {
        const Sexy::String count = theElement.GetAttribute("waveCount");
        theLevel.mHasWaveCount = count != "native";
        theLevel.mWaveCount = theLevel.mHasWaveCount ? ParseInteger(count, 0, kMaximumWaves) : -1;
    }
    if (theElement.HasAttribute("allowedZombies"))
    {
        const Sexy::String allowed = theElement.GetAttribute("allowedZombies");
        theLevel.mSurvivalAllowed = allowed == "survival";
        theLevel.mHasAllowedZombies = allowed != "native" && !theLevel.mSurvivalAllowed;
        theLevel.mAllowedZombies = theLevel.mHasAllowedZombies ? ReadZombieList(allowed) : std::vector<ZombieType>();
        std::sort(theLevel.mAllowedZombies.begin(), theLevel.mAllowedZombies.end());
        theLevel.mAllowedZombies.erase(std::unique(theLevel.mAllowedZombies.begin(), theLevel.mAllowedZombies.end()), theLevel.mAllowedZombies.end());
    }
    if (theElement.HasAttribute("scene"))
    {
        theLevel.mHasScene = true;
        theLevel.mScene = static_cast<BackgroundType>(ParseInteger(theElement.GetAttribute("scene"), BACKGROUND_1_DAY, BACKGROUND_TREEOFWISDOM));
    }
    if (theElement.HasAttribute("initialSunMoney"))
    {
        theLevel.mHasInitialSun = true;
        ReadInteger(theElement, "initialSunMoney", theLevel.mInitialSun, 0, 1000000);
    }
    if (theElement.HasAttribute("initPlantSetting")) ReadPlants(theElement.GetAttribute("initPlantSetting"), theLevel.mPlants);
    if (theElement.HasAttribute("initTomb")) ReadTombs(theElement.GetAttribute("initTomb"), theLevel.mTombs);
    if (theElement.HasAttribute("initSeeds"))
    {
        theLevel.mSeedConfig = ReadIntegerList(theElement.GetAttribute("initSeeds"), 0, NUM_ZOMBIE_SEED_IN_CHOOSER_VISIBLE - 1);
        if (theLevel.mSeedConfig.size() > 10) throw std::runtime_error("initSeeds exceeds the seed bank capacity");
    }
    if (theElement.HasAttribute("forbiddenPlants")) theLevel.mForbiddenSeeds = ReadIntegerList(theElement.GetAttribute("forbiddenPlants"), 0, NUM_ZOMBIE_SEED_IN_CHOOSER_VISIBLE - 1);
    if (theElement.HasAttribute("winCodeCondition")) theLevel.mWinConditions = ReadIntegerList(theElement.GetAttribute("winCodeCondition"), 0, 1000000);
    ReadInteger(theElement, "initSlotLimit", theLevel.mSeedBankLimit, -1, 10);
    ReadInteger(theElement, "initFogColumns", theLevel.mFogColumns, 0, 9);
    ReadInteger(theElement, "winCodeType", theLevel.mWinType, 0, 100);
    ReadInteger(theElement, "firstWaveDelay", theLevel.mFirstWaveDelay, 1, kMaximumDelay);
    ReadBoolean(theElement, "zombieVisible", theLevel.mZombiesVisible);
    if (theElement.HasAttribute("skipShopSeedTutorial"))
    {
        theLevel.mHasSkipShopSeedTutorial = true;
        ReadBoolean(theElement, "skipShopSeedTutorial", theLevel.mSkipShopSeedTutorial);
    }
    ReadBoolean(theElement, "allowYeti", theLevel.mAllowYeti);
    ReadInteger(theElement, "pointNumerator", theLevel.mPointNumerator, 0, 1000);
    ReadInteger(theElement, "pointDenominator", theLevel.mPointDenominator, 1, 1000);
    ReadInteger(theElement, "pointOffset", theLevel.mPointOffset, 0, 1000000);
    ReadInteger(theElement, "pointIndexOffset", theLevel.mPointIndexOffset, 0, 1000);
    ReadInteger(theElement, "pointStageStride", theLevel.mPointStageStride, 0, 100);
}

bool IsAction(const Sexy::String& theName)
{
    return theName == "Scale" || theName == "Spawn" || theName == "Missing" || theName == "Fill";
}

LevelWaveAction ReadAction(const Sexy::XMLElement& theElement)
{
    LevelWaveAction action;
    const Sexy::String& name = theElement.mInstruction;
    if (name == "Scale") action.mType = LevelWaveActionType::Scale;
    else if (name == "Spawn") action.mType = LevelWaveActionType::Spawn;
    else if (name == "Missing") action.mType = LevelWaveActionType::Missing;
    else action.mType = LevelWaveActionType::Fill;
    action.mZombies = ReadZombieList(theElement.GetAttribute("types"));
    if (action.mZombies.size() > kMaximumZombiesPerWave)
        throw std::runtime_error("action type list exceeds wave capacity");
    if ((action.mType == LevelWaveActionType::Spawn || action.mType == LevelWaveActionType::Missing)
        && action.mZombies.empty())
        throw std::runtime_error("Spawn and Missing require types");
    ReadInteger(theElement, "count", action.mCount, 0, kMaximumZombiesPerWave);
    ReadInteger(theElement, "budgetCountLimit", action.mBudgetCountLimit, 0, kMaximumZombiesPerWave);
    ReadInteger(theElement, "numerator", action.mNumerator, 0, 1000);
    ReadInteger(theElement, "denominator", action.mDenominator, 1, 1000);
    ReadBoolean(theElement, "flagOnly", action.mFlagOnly);
    ReadBoolean(theElement, "lastOnly", action.mLastOnly);
    ReadBoolean(theElement, "yetiUnkilledOnly", action.mYetiUnkilledOnly);
    return action;
}

void ReadSpawnPolicy(const Sexy::XMLElement& theElement, Sexy::Level& theLevel)
{
    if (theElement.mInstruction == "Summons")
    {
        auto readType = [&](const char* name, ZombieType& type) {
            if (theElement.HasAttribute(name))
                type = static_cast<ZombieType>(ParseInteger(theElement.GetAttribute(name), -1, NUM_ZOMBIE_TYPES - 1));
        };
        readType("backupDancer", theLevel.mSummons.mBackupDancer);
        readType("imp", theLevel.mSummons.mImp);
        readType("bossBungee", theLevel.mSummons.mBossBungee);
        readType("aquarium", theLevel.mSummons.mAquarium);
        readType("squirrel", theLevel.mSummons.mSquirrel);
        readType("bobsled", theLevel.mSummons.mBobsled);
        ReadInteger(theElement, "bossBungeeCount", theLevel.mSummons.mBossBungeeCount, 0, 3);
    }
    else if (theElement.mInstruction == "ExtraSpawns")
    {
        LevelExtraSpawns& extra = theLevel.mExtraSpawns;
        ReadBoolean(theElement, "graves", extra.mGraves);
        ReadBoolean(theElement, "graveCountdownChance", extra.mGraveCountdownChance);
        ReadBoolean(theElement, "gravesOnFlag", extra.mGravesOnFlag);
        ReadBoolean(theElement, "skyEveryWave", extra.mSkyEveryWave);
        ReadInteger(theElement, "graveMinimumAge", extra.mGraveMinimumAge, 0, kMaximumDelay);
        ReadInteger(theElement, "poolCount", extra.mPoolCount, 0, 8);
        ReadInteger(theElement, "poolPoints", extra.mPoolPoints, 0, 1000000);
        ReadInteger(theElement, "skyCount", extra.mSkyCount, 0, 50);
        ReadInteger(theElement, "skyPoints", extra.mSkyPoints, 0, 1000000);
        if (theElement.HasAttribute("types")) extra.mTypes = ReadZombieList(theElement.GetAttribute("types"));
        if (theElement.HasAttribute("bobsledFallback")) extra.mBobsledFallback = ReadZombieList(theElement.GetAttribute("bobsledFallback"));
        if (extra.mBobsledFallback.size() > kMaximumZombiesPerWave)
            throw std::runtime_error("bobsled fallback exceeds wave capacity");
        if (theElement.HasAttribute("carrier")) extra.mCarrier = static_cast<ZombieType>(ParseInteger(theElement.GetAttribute("carrier"), 0, NUM_ZOMBIE_TYPES - 1));
    }
    else if (theElement.mInstruction == "WhackSpawns")
    {
        LevelWhackSpawns& whack = theLevel.mWhackSpawns;
        whack.mStages.clear();
        whack.mFinalTypes = ReadZombieList(theElement.GetAttribute("finalTypes"));
        ReadInteger(theElement, "rollRange", whack.mRollRange, 1, 1000);
        ReadInteger(theElement, "waveDelay", whack.mWaveDelay, 1, kMaximumDelay);
        ReadInteger(theElement, "initialGraves", whack.mInitialGraves, 0, 54);
        ReadInteger(theElement, "graveTarget", whack.mGraveTarget, 0, 54);
        ReadInteger(theElement, "finalCount", whack.mFinalCount, 0, 54);
        ReadInteger(theElement, "firstMinimumDelay", whack.mFirstMinimumDelay, 1, kMaximumDelay);
        ReadInteger(theElement, "firstMaximumDelay", whack.mFirstMaximumDelay, 1, kMaximumDelay);
        ReadInteger(theElement, "lastMinimumDelay", whack.mLastMinimumDelay, 1, kMaximumDelay);
        ReadInteger(theElement, "lastMaximumDelay", whack.mLastMaximumDelay, 1, kMaximumDelay);
        ReadInteger(theElement, "delayEndWave", whack.mDelayEndWave, 2, 100);
        if (whack.mFirstMaximumDelay < whack.mFirstMinimumDelay || whack.mLastMaximumDelay < whack.mLastMinimumDelay)
            throw std::runtime_error("WhackSpawns maximum delay is smaller than minimum");
    }
    else if (theElement.mInstruction == "BossSpawns")
    {
        theLevel.mBossSpawns.clear();
        theLevel.mBossType = static_cast<ZombieType>(ParseInteger(theElement.GetAttribute("type"), 0, NUM_ZOMBIE_TYPES - 1));
    }
    else if (theElement.mInstruction == "Vases")
    {
        LevelVaseLayout layout;
        ReadInteger(theElement, "minimumStage", layout.mMinimumStage, 0, 1000000);
        ReadInteger(theElement, "maximumStage", layout.mMaximumStage, layout.mMinimumStage, 1000000);
        ReadInteger(theElement, "leafCount", layout.mLeafCount, 0, 45);
        layout.mExcludedColumns = ReadIntegerList(theElement.GetAttribute("excludedColumns"), 0, 8);
        theLevel.mVaseLayouts.push_back(std::move(layout));
    }
    else if (theElement.mInstruction == "Mound")
    {
        LevelMoundSpawn spawn;
        ReadInteger(theElement, "minimumLevel", spawn.mMinimumLevel, 0, 1000000);
        ReadInteger(theElement, "maximumLevel", spawn.mMaximumLevel, spawn.mMinimumLevel, 1000000);
        spawn.mTypes = ReadZombieList(theElement.GetAttribute("types"));
        if (spawn.mTypes.empty()) throw std::runtime_error("Mound requires types");
        theLevel.mMoundSpawns.push_back(std::move(spawn));
    }
}

void ReadWave(const Sexy::XMLElement& theElement, Sexy::Level& theLevel, int& theNextIndex)
{
    if (theElement.HasAttribute("afterNativeZombies"))
        throw std::runtime_error("afterNativeZombies was replaced by a Spawn action");
    int index = theNextIndex;
    ReadInteger(theElement, "index", index, 0, kMaximumWaves - 1);
    if (theLevel.mWaves.find(index) != theLevel.mWaves.end())
        throw std::runtime_error("duplicate Wave index");
    LevelWave wave;
    wave.mHasPoints = theLevel.mSpawnRules == Sexy::LevelSpawnRules::Legacy;
    if (theElement.HasAttribute("points"))
    {
        const Sexy::String points = theElement.GetAttribute("points");
        wave.mHasPoints = points != "native";
        if (wave.mHasPoints) wave.mPoints = ParseInteger(points, 0, 1000000);
    }
    wave.mHasFlag = theLevel.mFlagRules == Sexy::LevelFlagRules::Xml;
    if (theElement.HasAttribute("flag"))
    {
        const Sexy::String flag = theElement.GetAttribute("flag");
        wave.mHasFlag = flag != "native";
        if (wave.mHasFlag) wave.mFlag = ParseBoolean(flag);
    }
    wave.mMustHave = ReadZombieList(theElement.GetAttribute("mustHaveZombies"));
    if (wave.mMustHave.size() > kMaximumZombiesPerWave)
        throw std::runtime_error("forced zombie list exceeds the wave capacity");
    ReadBoolean(theElement, "replaceActions", wave.mHasActions);
    ReadInteger(theElement, "nextWaveDelay", wave.mNextWaveDelay, 1, kMaximumDelay);
    theLevel.mWaves.emplace(index, std::move(wave));
    theNextIndex = index + 1;
}

void FinishLevel(Sexy::Level& theLevel, const Sexy::LevelLoadContext& theContext)
{
    if (theLevel.mSpawnRules == Sexy::LevelSpawnRules::Xml && !theLevel.mHasWaveCount)
        throw std::runtime_error("XML spawn rules require an explicit waveCount");
    if (theLevel.mSpawnRules == Sexy::LevelSpawnRules::Xml && !theLevel.mHasAllowedZombies && !theLevel.mSurvivalAllowed)
        throw std::runtime_error("XML spawn rules require allowedZombies or a survival pool");
    if (theLevel.mSpawnRules == Sexy::LevelSpawnRules::Legacy)
    {
        theLevel.mHasAllowedZombies = true;
        if (!theLevel.mHasWaveCount)
            theLevel.mWaveCount = theLevel.mWaves.empty() ? 0 : theLevel.mWaves.rbegin()->first + 1;
        theLevel.mHasWaveCount = true;
    }
    if (theLevel.mSpawnRules != Sexy::LevelSpawnRules::Xml && theLevel.mActions.empty())
        theLevel.mActions.push_back(LevelWaveAction());
    if (theLevel.mHasWaveCount && !theLevel.mWaves.empty()
        && theLevel.mWaves.rbegin()->first >= theLevel.mWaveCount)
        throw std::runtime_error("Wave index is outside waveCount");
    theLevel.mFlagWaveCount = 0;
    for (const auto& entry : theLevel.mWaves)
        if (entry.second.mFlag) ++theLevel.mFlagWaveCount;
    theLevel.mGameMode = theContext.mGameMode;
    theLevel.mLevelId = theContext.mLevelId;
    theLevel.mSurvivalStage = theContext.mSurvivalStage;
    theLevel.mAdventureComplete = theContext.mAdventureComplete;
    theLevel.mLoaded = true;
}
}

Sexy::Level::Level()
{
    reset();
}

bool Sexy::Level::InitFromXML(GameMode theMode, int theLevelId, char const* thePath)
{
    LevelLoadContext context;
    context.mGameMode = theMode;
    context.mLevelId = theLevelId;
    return InitFromXML(context, String(thePath));
}

bool Sexy::Level::InitFromXML(const LevelLoadContext& theContext, const String& thePath)
{
    reset();
    try
    {
        XMLParser parser;
        if (!parser.OpenFile(thePath))
            return false;
        if (parser.HasFailed())
            throw std::runtime_error(parser.GetErrorText().c_str());
        Level defaults;
        Level candidate;
        Level selected;
        bool rootSeen = false;
        bool levelsSeen = false;
        bool insideLevel = false;
        bool matching = false;
        int candidateScore = 0;
        int bestScore = std::numeric_limits<int>::min();
        bool ambiguous = false;
        int nextIndex = 0;
        int currentWave = -1;
        bool levelActionsSeen = false;
        XMLElement element;
        while (parser.NextElement(&element))
        {
            if (element.mType == 1 && element.mInstruction == "PVZ" && element.mValue.empty())
                rootSeen = true;
            if (element.mType == 1 && element.mValue == "PVZ" && element.mInstruction == "Defaults")
            {
                if (levelsSeen) throw std::runtime_error("Defaults must precede Level records");
                ReadLevelAttributes(element, defaults);
            }
            else if (element.mType == 1 && element.mValue == "PVZ" && element.mInstruction == "Level")
            {
                levelsSeen = true;
                insideLevel = true;
                candidateScore = 0;
                matching = MatchesLevel(element, theContext, candidateScore);
                nextIndex = 0;
                currentWave = -1;
                levelActionsSeen = false;
                if (matching)
                {
                    candidate = defaults;
                    ReadLevelAttributes(element, candidate);
                }
            }
            else if (insideLevel && matching && element.mType == 1
                && element.mValue == "PVZ/Level" && element.mInstruction == "Wave")
            {
                ReadWave(element, candidate, nextIndex);
                currentWave = nextIndex - 1;
            }
            else if (insideLevel && matching && element.mType == 1
                && element.mValue == "PVZ/Level/Wave" && IsAction(element.mInstruction))
            {
                if (currentWave < 0) throw std::runtime_error("action outside Wave");
                LevelWave& wave = candidate.mWaves.at(currentWave);
                wave.mHasActions = true;
                wave.mActions.push_back(ReadAction(element));
            }
            else if (element.mType == 1 && (element.mValue == "PVZ/Defaults"
                || (insideLevel && matching && element.mValue == "PVZ/Level")))
            {
                Level& target = element.mValue == "PVZ/Defaults" ? defaults : candidate;
                if (IsAction(element.mInstruction))
                {
                    if (&target == &candidate && !levelActionsSeen)
                    {
                        target.mActions.clear();
                        levelActionsSeen = true;
                    }
                    target.mActions.push_back(ReadAction(element));
                }
                else ReadSpawnPolicy(element, target);
            }
            else if (insideLevel && matching && element.mType == 1
                && element.mValue == "PVZ/Level/WhackSpawns" && element.mInstruction == "Stage")
            {
                LevelWhackStage stage;
                ReadInteger(element, "twoWeight", stage.mTwoCountWeight, 0, candidate.mWhackSpawns.mRollRange);
                ReadInteger(element, "threeWeight", stage.mThreeCountWeight, 0, candidate.mWhackSpawns.mRollRange);
                stage.mTypes = ReadZombieList(element.GetAttribute("types"));
                stage.mWeights = ReadIntegerList(element.GetAttribute("weights"), 0, 1000000);
                stage.mThreeTypes = element.HasAttribute("threeTypes") ? ReadZombieList(element.GetAttribute("threeTypes")) : stage.mTypes;
                stage.mThreeWeights = element.HasAttribute("threeWeights") ? ReadIntegerList(element.GetAttribute("threeWeights"), 0, 1000000) : stage.mWeights;
                if (stage.mTypes.empty() || stage.mTypes.size() != stage.mWeights.size()
                    || stage.mTwoCountWeight + stage.mThreeCountWeight > candidate.mWhackSpawns.mRollRange
                    || stage.mThreeTypes.empty() || stage.mThreeTypes.size() != stage.mThreeWeights.size()
                    || std::all_of(stage.mWeights.begin(), stage.mWeights.end(), [](int weight) { return weight == 0; })
                    || std::all_of(stage.mThreeWeights.begin(), stage.mThreeWeights.end(), [](int weight) { return weight == 0; }))
                    throw std::runtime_error("invalid whack Stage weights");
                candidate.mWhackSpawns.mStages.push_back(std::move(stage));
            }
            else if (insideLevel && matching && element.mType == 1
                && element.mValue == "PVZ/Level/BossSpawns" && element.mInstruction == "Spawn")
            {
                LevelBossSpawn spawn;
                ReadInteger(element, "maximumAge", spawn.mMaximumAge, 0, 1000000);
                ReadInteger(element, "row", spawn.mRow, -1, 5);
                spawn.mTypes = ReadZombieList(element.GetAttribute("types"));
                if (spawn.mTypes.empty()) throw std::runtime_error("boss Spawn requires types");
                candidate.mBossSpawns.push_back(std::move(spawn));
            }
            else if (insideLevel && matching && element.mType == 1
                && element.mValue == "PVZ/Level/Vases" && element.mInstruction == "Pot")
            {
                if (candidate.mVaseLayouts.empty()) throw std::runtime_error("Pot outside Vases");
                LevelVaseContents pot;
                const Sexy::String kind = element.GetAttribute("kind");
                if (kind == "plant") pot.mKind = SCARYPOT_SEED;
                else if (kind == "zombie") pot.mKind = SCARYPOT_ZOMBIE;
                else if (kind == "sun") pot.mKind = SCARYPOT_SUN;
                else throw std::runtime_error("unknown Pot kind");
                if (element.HasAttribute("zombie")) pot.mZombie = static_cast<ZombieType>(ParseInteger(element.GetAttribute("zombie"), 0, NUM_ZOMBIE_TYPES - 1));
                if (element.HasAttribute("plant")) pot.mPlant = static_cast<SeedType>(ParseInteger(element.GetAttribute("plant"), 0, NUM_SEED_TYPES - 1));
                if ((pot.mKind == SCARYPOT_ZOMBIE && pot.mZombie == ZOMBIE_INVALID)
                    || (pot.mKind == SCARYPOT_SEED && pot.mPlant == SEED_NONE)) throw std::runtime_error("Pot requires content type");
                ReadInteger(element, "count", pot.mCount, 0, 45);
                ReadInteger(element, "stageDelta", pot.mStageDelta, -45, 45);
                ReadInteger(element, "stageDivisor", pot.mStageDivisor, 1, 1000000);
                ReadInteger(element, "maximumCount", pot.mMaximumCount, 0, 45);
                candidate.mVaseLayouts.back().mContents.push_back(pot);
            }
            else if (insideLevel && element.mType == 2 && element.mInstruction == "Level"
                && element.mValue == "PVZ/Level")
            {
                if (matching)
                {
                    FinishLevel(candidate, theContext);
                    if (candidateScore == bestScore)
                        ambiguous = true;
                    if (candidateScore > bestScore)
                    {
                        selected = std::move(candidate);
                        bestScore = candidateScore;
                        ambiguous = false;
                    }
                }
                insideLevel = false;
            }
        }
        if (!rootSeen || insideLevel || parser.HasFailed())
            throw std::runtime_error("invalid PVZ level document");
        if (ambiguous)
            throw std::runtime_error("ambiguous Level records; use distinct selectors or priority");
        if (!selected.mLoaded)
            return false;
        *this = std::move(selected);
        return true;
    }
    catch (const std::exception& error)
    {
        reset();
        mConfigurationError = error.what();
        return false;
    }
}

bool Sexy::Level::MatchesContext(const LevelLoadContext& theContext) const
{
    return mLoaded && mGameMode == theContext.mGameMode && mLevelId == theContext.mLevelId
        && mSurvivalStage == theContext.mSurvivalStage && mAdventureComplete == theContext.mAdventureComplete;
}

const LevelWave* Sexy::Level::FindWave(int theWave) const
{
    const auto found = mWaves.find(theWave);
    return found == mWaves.end() ? nullptr : &found->second;
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

int Sexy::Level::getWaveCount() const
{
    return mHasWaveCount ? mWaveCount : static_cast<int>(mWaves.size());
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

    mGameMode = NUM_GAME_MODES;
    mLevelId = -1;
    mScene = BACKGROUND_1_DAY;
    mSeedBankLimit = -1;
    mInitialSun = 0;
    mFogColumns = 0;
    mFlagWaveCount = 0;
    mWinType = 0;
    mZombiesVisible = true;
    mLoaded = false;
    mHasScene = false;
    mHasInitialSun = false;
    mHasAllowedZombies = false;
    mHasWaveCount = false;
    mWaveCount = -1;
    mSurvivalStage = 0;
    mAdventureComplete = false;
    mSpawnRules = LevelSpawnRules::Legacy;
    mFlagRules = LevelFlagRules::Legacy;
    mHasSkipShopSeedTutorial = false;
    mSkipShopSeedTutorial = false;
    mActions.clear();
    mExtraSpawns = LevelExtraSpawns();
    mWhackSpawns = LevelWhackSpawns();
    mBossSpawns.clear();
    mSummons = LevelSummons();
    mVaseLayouts.clear();
    mMoundSpawns.clear();
    mBossType = ZOMBIE_INVALID;
    mAllowYeti = false;
    mSurvivalAllowed = false;
    mPointNumerator = 1;
    mPointDenominator = 3;
    mPointOffset = 1;
    mPointIndexOffset = 0;
    mPointStageStride = 0;
    mFirstWaveDelay = -1;
    mConfigurationError.clear();
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

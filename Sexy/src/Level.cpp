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
    if (theElement.HasAttribute("spawnRules"))
    {
        const Sexy::String rules = theElement.GetAttribute("spawnRules");
        if (rules != "native" && rules != "legacy")
            throw std::runtime_error("unknown spawnRules");
        theLevel.mSpawnRules = rules == "native" ? Sexy::LevelSpawnRules::Native : Sexy::LevelSpawnRules::Legacy;
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
        theLevel.mHasAllowedZombies = allowed != "native";
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
    ReadBoolean(theElement, "skipShopSeedTutorial", theLevel.mSkipShopSeedTutorial);
    ReadBoolean(theElement, "nativeFlagWave", theLevel.mApplyNativeFlagWave);
    ReadBoolean(theElement, "nativePointMultiplier", theLevel.mApplyNativePointMultiplier);
    ReadBoolean(theElement, "nativeIntroductions", theLevel.mApplyNativeIntroductions);
    ReadBoolean(theElement, "nativeLastWave", theLevel.mApplyNativeLastWave);
}

void ReadWave(const Sexy::XMLElement& theElement, Sexy::Level& theLevel, int& theNextIndex)
{
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
    wave.mAfterNativeZombies = ReadZombieList(theElement.GetAttribute("afterNativeZombies"));
    if (wave.mMustHave.size() + wave.mAfterNativeZombies.size() > kMaximumZombiesPerWave)
        throw std::runtime_error("forced zombie list exceeds the wave capacity");
    ReadInteger(theElement, "nextWaveDelay", wave.mNextWaveDelay, 1, kMaximumDelay);
    theLevel.mWaves.emplace(index, std::move(wave));
    theNextIndex = index + 1;
}

void FinishLevel(Sexy::Level& theLevel, const Sexy::LevelLoadContext& theContext)
{
    if (theLevel.mSpawnRules == Sexy::LevelSpawnRules::Legacy)
    {
        theLevel.mHasAllowedZombies = true;
        if (!theLevel.mHasWaveCount)
            theLevel.mWaveCount = theLevel.mWaves.empty() ? 0 : theLevel.mWaves.rbegin()->first + 1;
        theLevel.mHasWaveCount = true;
    }
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
                if (matching)
                {
                    candidate = defaults;
                    ReadLevelAttributes(element, candidate);
                }
            }
            else if (insideLevel && matching && element.mType == 1
                && element.mValue == "PVZ/Level" && element.mInstruction == "Wave")
                ReadWave(element, candidate, nextIndex);
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

int Sexy::Level::getWaveCount()
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
    mSkipShopSeedTutorial = false;
    mApplyNativeFlagWave = true;
    mApplyNativePointMultiplier = true;
    mApplyNativeIntroductions = true;
    mApplyNativeLastWave = true;
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

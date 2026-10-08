// Included by the headless regression runner after normal app loading.
#include "Board.h"
#include "Challenge.h"
#include "Level.h"
#include "Zombie.h"
#include "DataArrayMethods.h"
#include "ReconFreeFunctions.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
int WaveSize(Board* theBoard, int theWave)
{
    int count = 0;
    while (count < MAX_ZOMBIES_IN_WAVE && theBoard->mZombiesInWave[theWave][count] != ZOMBIE_INVALID)
        ++count;
    return count;
}
}

bool RunLevelConfigurationTests(LawnApp* theApp)
{
    int failures = 0;
    auto check = [&failures](const char* theName, bool thePassed) {
        fprintf(stderr, "%s %s\n", thePassed ? "PASS" : "FAIL", theName);
        if (!thePassed) ++failures;
    };
    Sexy::Level level;
    Sexy::LevelLoadContext context;
    const Sexy::String manifest("properties/levels.xml");
    bool allContexts = true;
    int contexts = 0;
    for (int modeIndex = 0; modeIndex <= 93; ++modeIndex)
    {
        context.mGameMode = static_cast<GameMode>(modeIndex);
        for (bool repeat : {false, true})
        {
            context.mAdventureComplete = repeat;
            for (int stage : {0, 1, 5})
            {
                context.mSurvivalStage = stage;
                const int firstLevel = modeIndex <= 1 ? 1 : 0;
                const int lastLevel = modeIndex <= 1 ? 50 : 0;
                for (int levelId = firstLevel; levelId <= lastLevel; ++levelId)
                {
                    context.mLevelId = levelId;
                    const bool loaded = level.InitFromXML(context, manifest);
                    if (!loaded)
                        fprintf(stderr, "CONTEXT mode=%d level=%d stage=%d repeat=%d: %s\n",
                            modeIndex, levelId, stage, repeat, level.mConfigurationError.c_str());
                    allContexts = allContexts && loaded && level.MatchesContext(context);
                    ++contexts;
                }
            }
        }
    }
    check("all 94 defined modes, both cycles and stage contexts load", allContexts);
    fprintf(stderr, "CONTEXTS %d\n", contexts);
    context = {};
    context.mLevelId = 1;
    check("first 1-1 is four native-budget waves with tutorial skipped",
        level.InitFromXML(context, manifest) && level.getWaveCount() == 4
        && level.getWavePoint(0) == 1 && level.getWavePoint(1) == 1
        && level.getWavePoint(2) == 1 && level.getWavePoint(3) == 2
        && level.getFlagWaveCount() == 0 && level.mSkipShopSeedTutorial
        && !level.mHasAllowedZombies);
    context.mAdventureComplete = true;
    check("repeat 1-1 selected automatically", level.InitFromXML(context, manifest)
        && level.getWaveCount() == 20 && level.getWavePoint(3) == 2 && level.isFlagWave(9));
    context.mGameMode = GAMEMODE_CHALLENGE_HEAVY_WEAPON;
    context.mLevelId = 0;
    bool heavy = level.InitFromXML(context, manifest) && level.getWaveCount() == 25;
    for (int wave = 0; wave < 25; ++wave)
        heavy = heavy && level.getWavePoint(wave) == 3 * wave / 4;
    check("heavy weapon has 25 waves including zero budget", heavy);
    context.mGameMode = GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
    context.mSurvivalStage = 5;
    check("later survival stages inherit dynamic count, points and permission list",
        level.InitFromXML(context, manifest) && !level.mHasWaveCount
        && !level.mHasAllowedZombies && level.mWaves.empty());

    const auto fixtureDirectory = std::filesystem::absolute("build/levels-xml-regression/fixtures");
    std::filesystem::create_directories(fixtureDirectory);
    const auto fixturePath = fixtureDirectory / "levels-test.xml";
    auto loadText = [&](const std::string& theXml) {
        { std::ofstream stream(fixturePath, std::ios::binary); stream << theXml; }
        return level.InitFromXML(context, fixturePath.string().c_str());
    };
    context = {};
    context.mLevelId = 1;
    check("legacy sequential Wave and existing plant/tomb/seed attributes",
        loadText("<PVZ><Level id='1' mode='0' allowedZombies='0 2' scene='1' "
                 "initPlantSetting='[1 2 8]' initTomb='[6 1]' initSeeds='55 62 79' "
                 "forbiddenPlants='3' initSlotLimit='3'><Wave points='1'/>"
                 "<Wave points='2' flag='1' mustHaveZombies='2'/></Level></PVZ>")
        && level.getWaveCount() == 2 && level.mHasAllowedZombies
        && level.mScene == BACKGROUND_2_NIGHT && level.mPlants.size() == 1
        && level.mTombs.size() == 1 && level.mSeedConfig.size() == 3
        && level.isSeedForbidden(3) && level.getFlagWaveCount() == 1);
    check("native sparse overlay preserves unspecified waves",
        loadText("<PVZ><Defaults spawnRules='native'/><Level id='1'>"
                 "<Wave index='3' points='0' flag='0' nextWaveDelay='123'/></Level></PVZ>")
        && !level.mHasWaveCount && !level.mHasAllowedZombies && level.FindWave(0) == nullptr
        && level.FindWave(3)->mHasPoints && level.FindWave(3)->mNextWaveDelay == 123);
    check("specific best record clears ambiguity of less specific records",
        loadText("<PVZ><Defaults spawnRules='native'/><Level mode='*' id='*'/>"
                 "<Level mode='*' id='*'/><Level mode='0' id='1' waveCount='7'/></PVZ>")
        && level.getWaveCount() == 7);
    check("priority outranks specificity without replacing specificity score",
        loadText("<PVZ><Defaults spawnRules='native'/><Level mode='0' id='1' waveCount='7'/>"
                 "<Level mode='*' id='*' priority='1' waveCount='8'/></PVZ>")
        && level.getWaveCount() == 8);
    check("tied best records rejected and stale config cleared",
        !loadText("<PVZ><Level id='1'/><Level id='1'/></PVZ>")
        && !level.mLoaded && !level.mConfigurationError.empty() && level.mWaves.empty());
    check("no match clears previous selection",
        loadText("<PVZ><Level id='1'/></PVZ>")
        && !loadText("<PVZ><Level id='2'/></PVZ>") && !level.mLoaded);
    check("missing file clears previous selection",
        loadText("<PVZ><Level id='1'/></PVZ>")
        && !level.InitFromXML(context, (fixtureDirectory / "missing.xml").string().c_str()) && !level.mLoaded);
    for (const char* malformed : {
        "<PVZ><Level id='1'><Wave points='-1'/></Level></PVZ>",
        "<PVZ><Level id='1' waveCount='101'/></PVZ>",
        "<PVZ><Level id='1' allowedZombies='34'/></PVZ>",
        "<PVZ><Level id='1' waveCount='1'><Wave index='1'/></Level></PVZ>",
        "<PVZ><Level id='1'><Wave index='0'/><Wave index='0'/></Level></PVZ>",
        "<PVZ><Level id='1' firstWaveDelay='0'/></PVZ>",
        "<PVZ><Level id='1' adventureCycle='invalid'/></PVZ>",
        "<PVZ><Level id='1'/><Defaults spawnRules='native'/></PVZ>",
        "<PVZ><Level id='1' initPlantSetting='[9 0 0]'/></PVZ>",
        "<PVZ><Level id='1'><Wave/></PVZ>"})
    {
        check("invalid configuration rejected atomically", !loadText(malformed)
            && !level.mLoaded && !level.mConfigurationError.empty());
    }
    std::string oversized = "<PVZ><Level id='1'><Wave mustHaveZombies='";
    for (int count = 0; count < 51; ++count) oversized += "0 ";
    oversized += "'/></Level></PVZ>";
    check("oversized forced list rejected", !loadText(oversized) && !level.mLoaded);

    theApp->KillGameSelector();
    LawnPlayerInfo* originalPlayer = theApp->mPlayerInfo;
    const bool originalSkipSaving = theApp->mSkipProfileSaving;
    LawnPlayerInfo player(nullptr);
    player.Reset();
    player.mId = 0;
    player.mProfileId = 1234567;
    player.mName = "Level XML regression";
    theApp->mPlayerInfo = &player;
    theApp->mSkipProfileSaving = true;
    auto newBoard = [&](GameMode theMode, int theLevelId) {
        theApp->mGameMode = theMode;
        player.mLevel = theLevelId;
        theApp->NewGame();
        return theApp->mBoard;
    };
    auto killBoard = [&]() { theApp->KillBoard(); theApp->ProcessSafeDeleteList(); };
    Board* board = newBoard(GAMEMODE_ADVENTURE, 1);
    int total = 0;
    for (int wave = 0; wave < board->mNumWaves; ++wave) total += WaveSize(board, wave);
    check("runtime first 1-1 is exactly five normal zombies", board->mNumWaves == 4 && total == 5
        && player.mPassedShopSeedTutorial && !board->IsFlagWave(3));
    killBoard();
    player.SetFlag(1, 1);
    board = newBoard(GAMEMODE_ADVENTURE_TWO_PLAYER, 1);
    check("two-player repeat adventure selects the repeat record", board->mNumWaves == 20
        && board->IsFlagWave(9) && theApp->mLevel.mAdventureComplete);
    killBoard();
    player.SetFlag(1, 0);
    board = newBoard(GAMEMODE_CHALLENGE_HEAVY_WEAPON, 0);
    check("runtime heavy weapon retains native permissions and zero-budget first wave",
        board->mNumWaves == 25 && WaveSize(board, 0) == 0 && board->mZombieAllowed[ZOMBIE_POGO]
        && board->mZombieAllowed[ZOMBIE_GARGANTUAR] && !board->mZombieAllowed[ZOMBIE_TRAFFIC_CONE]);
    board->SpawnZombieWave();
    check("heavy weapon zero-budget wave still invokes native sky spawning", board->mZombies.mSize > 0
        && board->mCurrentWave == 1);
    killBoard();
    board = newBoard(GAMEMODE_SURVIVAL_NORMAL_STAGE_1, 0);
    check("runtime normal survival begins with ten waves", board->mNumWaves == 10);
    board->mChallenge->mSurvivalStage = 5;
    board->InitZombieWaves();
    check("survival transition reloads stage context and preserves dynamic generation",
        board->IsLevelDataLoaded() && theApp->mLevel.mSurvivalStage == 5
        && !theApp->mLevel.mHasWaveCount && !theApp->mLevel.mHasAllowedZombies
        && board->mNumWaves == 10 && WaveSize(board, 0) > 1 && board->mZombieCountDown == 600);
    killBoard();
    board = newBoard(GAMEMODE_CHALLENGE_LAST_STAND, 0);
    board->mChallenge->mSurvivalStage = 2;
    board->InitZombieWaves();
    check("last stand transition selects its new context", board->IsLevelDataLoaded()
        && theApp->mLevel.mSurvivalStage == 2 && board->mNumWaves == 10);
    killBoard();
    board = newBoard(GAMEMODE_ADVENTURE, 45);
    bool bungees = WaveSize(board, 9) == 5;
    for (int index = 0; index < 5; ++index)
        bungees = bungees && board->mZombiesInWave[9][index] == ZOMBIE_BUNGEE;
    check("5-5 flag wave remains exactly five bungees", bungees);
    killBoard();
    for (int levelId : {15, 35, 50})
    {
        board = newBoard(GAMEMODE_ADVENTURE, levelId);
        check("special adventure controller retained", board->IsLevelDataLoaded()
            && (levelId == 15 ? theApp->IsWhackAZombieLevel()
                : levelId == 35 ? theApp->IsScaryPotterLevel() : theApp->IsFinalBossLevel()));
        if (levelId == 15)
        {
            const int initialWave = board->mCurrentWave;
            board->mZombieCountDown = 2;
            board->mChallenge->UpdateZombieSpawning();
            check("whack controller handles its own countdown", board->mZombieCountDown == 1
                && board->mCurrentWave == initialWave);
        }
        killBoard();
    }
    board = newBoard(GAMEMODE_CHALLENGE_SLOT_MACHINE, 0);
    context.mGameMode = GAMEMODE_CHALLENGE_SLOT_MACHINE;
    context.mLevelId = 0;
    const bool overridden = loadText("<PVZ><Defaults spawnRules='native' flagRules='xml'/><Level mode='19' id='0' "
        "waveCount='2' firstWaveDelay='321' nativeIntroductions='0' nativeLastWave='0' nativeFlagWave='0' "
        "nativePointMultiplier='0'><Wave points='0' flag='1' mustHaveZombies='0' afterNativeZombies='2' "
        "nextWaveDelay='123'/><Wave points='0'/></Level></PVZ>");
    check("non-adventure flags and both forced phases parsed", overridden);
    theApp->mLevel = level;
    board->InitZombieWaves();
    check("custom phases preserve forced order, disable native additions and use first delay",
        board->mNumWaves == 2 && board->IsFlagWave(0) && !board->IsFlagWave(1)
        && board->mZombiesInWave[0][0] == ZOMBIE_NORMAL && board->mZombiesInWave[0][1] == ZOMBIE_TRAFFIC_CONE
        && WaveSize(board, 0) == 2 && board->mZombieCountDown == 321);
    board->mTutorialState = TUTORIAL_OFF;
    board->mZombieCountDown = 1;
    board->UpdateZombieSpawning();
    check("custom next wave delay is applied", board->mCurrentWave == 1 && board->mZombieCountDown == 123);
    killBoard();
    theApp->mPlayerInfo = originalPlayer;
    theApp->mSkipProfileSaving = originalSkipSaving;
    fprintf(stderr, "LEVEL XML failures=%d\n", failures);
    return failures == 0;
}

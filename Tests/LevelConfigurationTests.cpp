// Included by the headless regression runner after normal app loading.
#include "game/Board.h"
#include "Challenge.h"
#include "Level.h"
#include "Zombie.h"
#include "GridItem.h"
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

bool RunSpecialSpawnTests(LawnApp* theApp)
{
    bool success = true;
    auto check = [&](const char* name, bool passed) {
        fprintf(stderr, "%s %s\n", passed ? "PASS" : "FAIL", name);
        success = passed && success;
    };
    LawnPlayerInfo* originalPlayer = theApp->mPlayerInfo;
    const bool originalSkipSaving = theApp->mSkipProfileSaving;
    LawnPlayerInfo player(nullptr);
    player.Reset();
    player.mId = 0;
    player.mProfileId = 1234567;
    player.mName = "Special spawn regression";
    theApp->mPlayerInfo = &player;
    theApp->mSkipProfileSaving = true;
    auto create = [&](int levelId) {
        theApp->mGameMode = GAMEMODE_ADVENTURE;
        player.mLevel = levelId;
        theApp->NewGame();
        return theApp->mBoard;
    };
    auto destroy = [&]() { theApp->KillBoard(); theApp->ProcessSafeDeleteList(); };
    Board* board = create(35);
    for (int stage = 0; stage < 3; ++stage)
    {
        GridItem* item = nullptr;
        while (board->IterateGridItems(item))
            if (item->mGridItemType == GRIDITEM_SCARY_POT) item->GridItemDie();
        board->mChallenge->mSurvivalStage = stage;
        const int pots = board->mChallenge->ScaryPotterPopulate();
        int zombiePots = 0;
        item = nullptr;
        while (board->IterateGridItems(item))
            if (item->mGridItemType == GRIDITEM_SCARY_POT && item->mScaryPotType == SCARYPOT_ZOMBIE) ++zombiePots;
        check("TV adventure vase stages retain declared contents",
            pots == (stage == 0 ? 15 : stage == 1 ? 20 : 25)
            && zombiePots == (stage == 0 ? 5 : stage == 1 ? 7 : 10));
    }
    GridItem* pot = nullptr;
    while (board->IterateGridItems(pot))
        if (pot->mGridItemType == GRIDITEM_SCARY_POT) pot->GridItemDie();
    theApp->mLevel.mVaseLayouts.clear();
    check("empty vase rules do not fall back to hardcoded zombies", board->mChallenge->ScaryPotterPopulate() == 0);
    destroy();
    board = create(50);
    Zombie* boss = board->mChallenge->PlayBossEnter();
    check("boss entry comes from XML", boss && boss->mZombieType == ZOMBIE_BOSS);
    if (boss)
    {
        boss->mZombieAge = 100;
        boss->mTargetRow = 2;
        Zombie* summoned = boss->BossSpawnContact();
        check("boss early summon follows original XML age rule", summoned && summoned->mZombieType == ZOMBIE_NORMAL);
        LevelBossSpawn custom;
        custom.mTypes = {ZOMBIE_POGO};
        theApp->mLevel.mBossSpawns = {custom};
        summoned = boss->BossSpawnContact();
        check("boss pool can be replaced by XML", summoned && summoned->mZombieType == ZOMBIE_POGO);
        theApp->mLevel.mBossSpawns.clear();
        check("empty boss pool disables summon", boss->BossSpawnContact() == nullptr);
    }
    destroy();
    board = create(15);
    LevelWhackStage customStage;
    customStage.mThreeCountWeight = 30;
    customStage.mTypes = {ZOMBIE_PAIL};
    customStage.mWeights = {30};
    theApp->mLevel.mWhackSpawns.mStages = {customStage};
    board->mCurrentWave = 1;
    board->mZombieCountDown = 500;
    board->mChallenge->mChallengeStateCounter = 1;
    const int before = board->mZombies.mSize;
    board->mChallenge->WhackAZombieSpawning();
    bool pails = true;
    Zombie* zombie = nullptr;
    while (board->IterateZombies(zombie)) pails = pails && zombie->mZombieType == ZOMBIE_PAIL;
    check("whack XML controls both count and type", board->mZombies.mSize - before == 3 && pails);
    theApp->mLevel.mWhackSpawns.mStages.clear();
    board->mChallenge->WhackAZombieSpawning();
    check("empty whack rules disable automatic spawn", board->mZombies.mSize - before == 3);
    destroy();
    theApp->mPlayerInfo = originalPlayer;
    theApp->mSkipProfileSaving = originalSkipSaving;
    return success;
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
        for (bool repeat : modeIndex <= 1 ? std::vector<bool>{false, true} : std::vector<bool>{false})
        {
            context.mAdventureComplete = repeat;
            for (int stage : (modeIndex >= 2 && modeIndex <= 16) || modeIndex == 32 ? std::vector<int>{0, 1, 5} : std::vector<int>{0})
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
    check("all 94 defined modes, adventure cycles and survival stages load", allContexts);
    fprintf(stderr, "CONTEXTS %d\n", contexts);
    context = {};
    context.mLevelId = 1;
    check("TV 1-1 preserves four original XML budgets",
        level.InitFromXML(context, manifest) && level.getWaveCount() == 4
        && level.getWavePoint(0) == 1 && level.getWavePoint(1) == 1
        && level.getWavePoint(2) == 2 && level.getWavePoint(3) == 2
        && level.getFlagWaveCount() == 0 && level.mSkipShopSeedTutorial
        && level.mHasAllowedZombies);
    context.mAdventureComplete = true;
    check("repeat 1-1 selected automatically", level.InitFromXML(context, manifest)
        && level.getWaveCount() == 4 && level.getWavePoint(2) == 2 && !level.isFlagWave(3));
    context.mGameMode = GAMEMODE_CHALLENGE_HEAVY_WEAPON;
    context.mLevelId = 0;
    bool heavy = level.InitFromXML(context, manifest) && level.getWaveCount() == 25;
    for (int wave = 0; wave < 25; ++wave)
        heavy = heavy && level.getWavePoint(wave) == 3 * wave / 4;
    check("heavy weapon has 25 waves including zero budget", heavy);
    context.mGameMode = GAMEMODE_SURVIVAL_NORMAL_STAGE_1;
    context.mSurvivalStage = 5;
    check("later survival stages declare points formula and dynamic pool",
        level.InitFromXML(context, manifest) && level.getWaveCount() == 10
        && level.mSurvivalAllowed && level.mPointStageStride == 10 && level.mPointNumerator == 2);

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
    check("legacy sparse overlay parses unspecified fields",
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
        "<PVZ><Level id='1' nativeIntroductions='true'/></PVZ>",
        "<PVZ><Level id='1'><Wave afterNativeZombies='2'/></Level></PVZ>",
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
    check("runtime first 1-1 is exactly six normal zombies", board->mNumWaves == 4 && total == 6
        && player.mPassedShopSeedTutorial && !board->IsFlagWave(3));
    killBoard();
    player.SetFlag(1, 1);
    board = newBoard(GAMEMODE_ADVENTURE_TWO_PLAYER, 1);
    check("two-player repeat adventure selects the repeat record", board->mNumWaves == 4
        && !board->IsFlagWave(3) && theApp->mLevel.mAdventureComplete);
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
        && theApp->mLevel.mHasWaveCount && theApp->mLevel.mSurvivalAllowed
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
    const bool overridden = loadText("<PVZ><Defaults spawnRules='xml' flagRules='xml'/><Level mode='19' id='0' "
        "allowedZombies='0' waveCount='2' firstWaveDelay='321'>"
        "<Wave points='0' flag='1' mustHaveZombies='0' nextWaveDelay='123'>"
        "<Spawn types='2'/></Wave><Wave points='0' replaceActions='true'/></Level></PVZ>");
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
    check("zero-action wave has no hidden additions", WaveSize(board, 1) == 0);
    board->mCurrentWave = 0;
    theApp->mLevel.mExtraSpawns = LevelExtraSpawns();
    const int zombieCount = board->mZombies.mSize;
    board->SpawnZombiesFromSky();
    board->SpawnZombiesFromPool();
    board->SpawnZombiesFromGraves();
    check("empty extra-spawn policy disables all extra sources", board->mZombies.mSize == zombieCount);
    killBoard();
    theApp->mPlayerInfo = originalPlayer;
    theApp->mSkipProfileSaving = originalSkipSaving;
    fprintf(stderr, "LEVEL XML failures=%d\n", failures);
    return failures == 0;
}

bool RunNormalLevelConfigurationTests(LawnApp* theApp, const Sexy::String& thePcPath,
                                     const Sexy::String& theTvPath)
{
    bool success = true;
    auto check = [&](const char* name, bool passed) {
        fprintf(stderr, "%s %s\n", passed ? "PASS" : "FAIL", name);
        success = passed && success;
    };
    Sexy::Level level;
    Sexy::LevelLoadContext context;
    int contexts = 0;
    for (int modeIndex = 0; modeIndex <= 93; ++modeIndex)
    {
        context.mGameMode = static_cast<GameMode>(modeIndex);
        const auto cycles = modeIndex <= 1 ? std::vector<bool>{false, true} : std::vector<bool>{false};
        const auto stages = (modeIndex >= 2 && modeIndex <= 16) || modeIndex == 32
            ? std::vector<int>{0, 1, 5} : std::vector<int>{0};
        for (bool repeat : cycles)
            for (int stage : stages)
            {
                context.mAdventureComplete = repeat;
                context.mSurvivalStage = stage;
                for (int levelId = modeIndex <= 1 ? 1 : 0; levelId <= (modeIndex <= 1 ? 50 : 0); ++levelId)
                {
                    context.mLevelId = levelId;
                    const bool loaded = level.InitFromXML(context, thePcPath);
                    if (!loaded)
                        fprintf(stderr, "PC CONTEXT mode=%d level=%d stage=%d repeat=%d: %s\n",
                            modeIndex, levelId, stage, repeat, level.mConfigurationError.c_str());
                    success = loaded && level.MatchesContext(context) && success;
                    ++contexts;
                }
            }
    }
    check("NormalLevel standalone XML loads all modes, cycles and stages", success);
    fprintf(stderr, "PC CONTEXTS %d\n", contexts);
    fflush(stderr);
    context = {};
    context.mLevelId = 1;
    check("PC first 1-1 retains native 1 1 1 2 budgets",
        level.InitFromXML(context, thePcPath) && level.getWaveCount() == 4
        && level.getWavePoint(2) == 1 && level.getFlagWaveCount() == 0 && level.mSkipShopSeedTutorial);
    check("TV first 1-1 retains supplied 1 1 2 2 budgets",
        level.InitFromXML(context, theTvPath) && level.getWaveCount() == 4 && level.getWavePoint(2) == 2);
    context.mAdventureComplete = true;
    check("PC repeat 1-1 uses twenty waves and two flags",
        level.InitFromXML(context, thePcPath) && level.getWaveCount() == 20
        && level.getWavePoint(10) == 5 && level.isFlagWave(9) && level.isFlagWave(19));
    check("TV repeat retains its own four-wave XML",
        level.InitFromXML(context, theTvPath) && level.getWaveCount() == 4 && !level.isFlagWave(3));
    context.mAdventureComplete = false;
    context.mLevelId = 15;
    check("PC whack has six waves", level.InitFromXML(context, thePcPath) && level.getWaveCount() == 6);
    check("TV whack has seven waves", level.InitFromXML(context, theTvPath) && level.getWaveCount() == 7);

    LawnPlayerInfo* originalPlayer = theApp->mPlayerInfo;
    const bool originalSkipSaving = theApp->mSkipProfileSaving;
    LawnPlayerInfo player(nullptr);
    player.Reset();
    player.mId = 0;
    player.mProfileId = 1234567;
    player.mName = "NormalLevel regression";
    theApp->mPlayerInfo = &player;
    theApp->mSkipProfileSaving = true;
    auto createPcBoard = [&](GameMode mode, int levelId, bool repeat) {
        player.mLevel = levelId;
        player.SetFlag(1, repeat);
        theApp->mGameMode = mode;
        theApp->NewGame();
        Sexy::LevelLoadContext boardContext;
        boardContext.mGameMode = mode;
        boardContext.mLevelId = theApp->mBoard->mLevel;
        boardContext.mAdventureComplete = repeat;
        check("PC runtime configuration selected", theApp->mLevel.InitFromXML(boardContext, thePcPath));
        theApp->mBoard->InitZombieWaves();
        return theApp->mBoard;
    };
    auto destroy = [&]() { theApp->KillBoard(); theApp->ProcessSafeDeleteList(); };
    Board* board = createPcBoard(GAMEMODE_ADVENTURE, 1, false);
    int total = 0;
    bool normalOnly = true;
    for (int wave = 0; wave < board->mNumWaves; ++wave)
        for (int slot = 0; slot < WaveSize(board, wave); ++slot)
        {
            ++total;
            normalOnly = normalOnly && board->mZombiesInWave[wave][slot] == ZOMBIE_NORMAL;
        }
    check("PC first 1-1 is exactly five normal zombies", total == 5 && normalOnly);
    destroy();
    board = createPcBoard(GAMEMODE_ADVENTURE_TWO_PLAYER, 1, true);
    check("PC repeat runtime uses cycle selector and declared flag additions",
        board->mNumWaves == 20 && board->IsFlagWave(9) && board->IsFlagWave(19)
        && board->mZombiesInWave[9][4] == ZOMBIE_FLAG);
    destroy();
    board = createPcBoard(GAMEMODE_ADVENTURE, 5, false);
    check("PC first bowling retains eight waves and final flag", board->mNumWaves == 8 && board->IsFlagWave(7));
    destroy();
    board = createPcBoard(GAMEMODE_CHALLENGE_HEAVY_WEAPON, 0, false);
    board->SpawnZombieWave();
    check("PC package also retains TV Heavy Weapon XML sky spawns",
        board->mNumWaves == 25 && board->mCurrentWave == 1 && board->mZombies.mSize == 6);
    destroy();
    theApp->mPlayerInfo = originalPlayer;
    theApp->mSkipProfileSaving = originalSkipSaving;
    return success;
}

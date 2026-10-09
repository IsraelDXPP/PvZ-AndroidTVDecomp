// Copyright (C) 2023-2026 PvZ TV Touch Team. GPL-3.0-or-later.
// Adapted to native C++ objects and local profiles on 2026-10-09.
#include "AchievementSystem.h"
#include "LawnApp.h"
#include "game/Board.h"
#include "Challenge.h"
#include "SeedBank.h"
#include <algorithm>

namespace {
const AchievementDefinition kAchievements[NUM_ACHIEVEMENT_TYPES] =
{
    {"ACHIEVEMENT_HOME_SECURITY", "achievement_homeLawnsecurity", u8"家庭安保", u8"完成冒险模式。", REANIM_ACHIEVEMENT_HOME_LAWN_SECURITY},
    {"ACHIEVEMENT_MORTICULTURALIST", "achievement_morticulturalist", u8"植物学家", u8"收集全部 49 种植物，包括模仿者。", REANIM_ACHIEVEMENT_MOM},
    {"ACHIEVEMENT_IMMORTAL", "achievement_immortal", u8"生生不息", u8"在生存无尽模式中完成 40 面旗帜。", REANIM_ACHIEVEMENT_ALIVE_AND_PLANTING},
    {"ACHIEVEMENT_SOILPLANTS", "achievement_soilplants", u8"僵尸入侵", u8"在一个普通关卡中种植 10 株豌豆射手。", REANIM_ACHIEVEMENT_SOIL_YOUR_PLANTS},
    {"ACHIEVEMENT_CLOSESHAVE", "achievement_closeshave", u8"千钧一发", u8"在所有草坪除草机都被使用后赢得关卡。", REANIM_ACHIEVEMENT_CLOSE_SHAVE},
    {"ACHIEVEMENT_CHOMP", "achievement_chomp", u8"咔嚓咔嚓", u8"只使用向日葵、坚果墙和大嘴花赢得普通关卡。", REANIM_ACHIEVEMENT_NOM_NOM_NOM},
    {"ACHIEVEMENT_VERSUS", "achievement_versusz", u8"对战高手", u8"在对战模式中连续赢得 5 场比赛。", REANIM_ACHIEVEMENT_VERSUS},
    {"ACHIEVEMENT_GARG", "achievement_garg", u8"泰坦陨落", u8"击败一个巨人僵尸。", REANIM_ACHIEVEMENT_CRASH_OF_THE_TITAN},
    {"ACHIEVEMENT_COOP", "achievement_coop", u8"友谊长存", u8"完成双人合作坚果保龄球。", REANIM_ACHIEVEMENT_ZFFS_4_EVR},
    {"ACHIEVEMENT_SHOP", "achievement_shop", u8"购物愉快", u8"在疯狂戴夫的商店消费至少 25000 金币。已消费：{coin}", REANIM_ACHIEVEMENT_CRAZY},
    {"ACHIEVEMENT_EXPLODONATOR", "achievement_explodonator", u8"爆破专家", u8"用一枚樱桃炸弹炸死 10 个僵尸。", REANIM_ACHIEVEMENT_EXPLODONATOR},
    {"ACHIEVEMENT_TREE", "achievement_tree", u8"智慧树", u8"将智慧树培养到 100 英尺。", REANIM_ACHIEVEMENT_SMARTY_BRANCHES}
};

void GrantSharedBoardAchievement(LawnApp* theApp, AchievementType theAchievement)
{
    theApp->GrantAchievement(theAchievement);
    if (theApp->mSecondPlayerGamepadIndex != -1 && theApp->mPlayer2Info != nullptr)
        theApp->GrantAchievement(theAchievement, theApp->mPlayer2Info);
}
}

const AchievementDefinition& GetAchievementDefinition(AchievementType theAchievement)
{
    return kAchievements[theAchievement];
}

Sexy::String GetAchievementTitle(AchievementType theAchievement)
{
    const AchievementDefinition& definition = GetAchievementDefinition(theAchievement);
    return gLawnApp->GetString(definition.mStringKey, definition.mDefaultTitle);
}

Sexy::String GetAchievementDescription(AchievementType theAchievement)
{
    const AchievementDefinition& definition = GetAchievementDefinition(theAchievement);
    Sexy::String key(definition.mStringKey);
    key += "_TEXT";
    Sexy::String description = gLawnApp->GetString(key, definition.mDefaultDescription);
    if (theAchievement == ACHIEVEMENT_SHOP && gLawnApp->mPlayerInfo != nullptr)
    {
        Sexy::String result;
        TodReplaceNumberString(&result, &description, "{coin}", gLawnApp->mPlayerInfo->mUsedCoins * 10);
        return result;
    }
    return description;
}

Sexy::Image* GetAchievementIcon(AchievementType theAchievement)
{
    return gLawnApp->GetConsoleImage(GetAchievementDefinition(theAchievement).mIconName);
}

Sexy::Image* LawnApp::GetConsoleImage(const char* theName)
{
    auto found = mConsoleImages.find(theName);
    if (found != mConsoleImages.end())
        return found->second;
    Sexy::String path("images/");
    path += theName;
    Sexy::SharedImageRef image = GetSharedImage(path, "", nullptr, true, false);
    mConsoleImages.emplace(theName, image);
    return image;
}

bool LawnApp::GrantAchievement(AchievementType theAchievement, LawnPlayerInfo* thePlayer, bool theShow)
{
    if (thePlayer == nullptr)
        thePlayer = mPlayerInfo;
    if (thePlayer == nullptr || theAchievement < ACHIEVEMENT_HOME_SECURITY
        || theAchievement >= NUM_ACHIEVEMENT_TYPES || thePlayer->mAchievements[theAchievement])
        return false;
    thePlayer->mAchievements[theAchievement] = true;
    if (theShow)
    {
        PlaySample(SOUND_BLEEP, true);
        if (mBoard != nullptr && mGameScene == SCENE_PLAYING)
        {
            Sexy::String message = GetString("ACHIEVEMENT_GRANTED", u8"获得成就：{achievement}");
            Sexy::String formatted;
            TodReplaceString(&formatted, message, "{achievement}", GetAchievementTitle(theAchievement));
            message = formatted;
            mBoard->DisplayAdviceAgain(message, MESSAGE_STYLE_ACHIEVEMENT, ADVICE_NONE);
        }
    }
    if (!mSkipProfileSaving)
        thePlayer->SaveDetails();
    return true;
}

void LawnApp::RefreshProfileAchievements()
{
    if (mPlayerInfo == nullptr)
        return;
    if (HasFinishedAdventure())
        GrantAchievement(ACHIEVEMENT_HOME_SECURITY, mPlayerInfo, false);
    if (HasAllAchievementPlants())
        GrantAchievement(ACHIEVEMENT_MORTICULTURALIST, mPlayerInfo, false);
    if (mPlayerInfo->mUsedCoins >= 2500)
        GrantAchievement(ACHIEVEMENT_SHOP, mPlayerInfo, false);
    if (mPlayerInfo->mChallengeRecords[GAMEMODE_TREE_OF_WISDOM - GAMEMODE_SURVIVAL_NORMAL_STAGE_1] >= 100)
        GrantAchievement(ACHIEVEMENT_TREE, mPlayerInfo, false);
    if (mPlayerInfo->mEndlessHighScore
        || mPlayerInfo->mChallengeRecords[GAMEMODE_SURVIVAL_ENDLESS_STAGE_3 - GAMEMODE_SURVIVAL_NORMAL_STAGE_1] >= 40)
        GrantAchievement(ACHIEVEMENT_IMMORTAL, mPlayerInfo, false);
    if ((mPlayerInfo->mProgress[0] & 0xFF00) != 0)
        GrantAchievement(ACHIEVEMENT_GARG, mPlayerInfo, false);
    if (mPlayerInfo->mWinStreak >= 5)
        GrantAchievement(ACHIEVEMENT_VERSUS, mPlayerInfo, false);
}

bool LawnApp::HasAllAchievementPlants()
{
    if (mPlayerInfo == nullptr)
        return false;
    // The forty adventure seeds and all nine shop seeds, including Imitater.
    for (int seed = SEED_PEASHOOTER; seed <= SEED_IMITATER; ++seed)
        if (!HasSeedType(SeedType(seed), false))
            return false;
    return true;
}

void LawnApp::SetHouseReanim(Reanimation* theHouse)
{
    if (theHouse == nullptr || mPlayerInfo == nullptr)
        return;
    static const char* const stages[] = {"anim_house_1_1", "anim_house_1_1", "anim_house_1_2",
                                         "anim_house_1_3", "anim_house_1_4", "anim_house_1_5"};
    static const char* const prefixes[] = {"house_1", "house_2", "house_3", "house_4", "house_5"};
    HouseType type = mPlayerInfo->mGameStats.mHouseType;
    if (type < BLUEPRINT_BLING || type > BLUEPRINT_CLOWN)
        type = BLUEPRINT_BLING;
    int level = mPlayerInfo->mPurchases[STORE_ITEM_BLUEPRINT_BLING + int(type)];
    if (type == BLUEPRINT_BLING)
        level += CanShowStore() ? 3 : (CanShowAlmanac() ? 2 : 1);
    theHouse->PlayReanim(stages[std::clamp(level, 0, 5)], REANIM_LOOP, 0, 12.0f);
    for (int house = BLUEPRINT_BLING; house <= BLUEPRINT_CLOWN; ++house)
        theHouse->AssignRenderGroupToPrefix(prefixes[house], house == type ? 0 : -1);
    theHouse->AssignRenderGroupToPrefix("achievement", -1);
}

void Board::DoPlantingAchievementCheck(SeedType theSeedType)
{
    // Exclude garden/setup plants and challenges without ordinary planting.
    if (mApp->mGameScene != SCENE_PLAYING || !mApp->IsSeedPlantingLevel())
        return;
    if (theSeedType != SEED_CHOMPER && theSeedType != SEED_SUNFLOWER && theSeedType != SEED_WALLNUT)
        mNewWallNutAndSunFlowerAndChomperOnly = false;
    if (theSeedType == SEED_PEASHOOTER && !HasConveyorBeltSeedBank(false))
    {
        if (mNewPeaShooterCount < 10)
            ++mNewPeaShooterCount;
        if (mNewPeaShooterCount >= 10)
            GrantSharedBoardAchievement(mApp, ACHIEVEMENT_SOILPLANTS);
    }
}

void Board::CheckLevelAchievements()
{
    if (mApp->IsAdventureMode() && mLevel == 50)
        GrantSharedBoardAchievement(mApp, ACHIEVEMENT_HOME_SECURITY);
    // Native SurvivalSaveScore @0x15AF08 requires solo pool endless and forty flags.
    if (mApp->mGameMode == GAMEMODE_SURVIVAL_ENDLESS_STAGE_3 && !mApp->IsCoopMode()
        && GetNumWavesPerSurvivalStage() * (mChallenge->mSurvivalStage + 1) / GetNumWavesPerFlag() >= 40)
    {
        mApp->mPlayerInfo->mEndlessHighScore = true;
        mApp->GrantAchievement(ACHIEVEMENT_IMMORTAL);
    }
    if (mApp->mGameMode == GAMEMODE_TWO_PLAYER_COOP_BOWLING)
        GrantSharedBoardAchievement(mApp, ACHIEVEMENT_COOP);
    if (!mApp->IsSurvivalMode() && mApp->mGameMode != GAMEMODE_MP_VS)
    {
        int lawnRows = 0;
        for (PlantRowType row : mPlantRow)
            lawnRows += row != PLANTROW_DIRT ? 1 : 0;
        if (lawnRows > 0 && mTriggeredLawnMowers == lawnRows)
            GrantSharedBoardAchievement(mApp, ACHIEVEMENT_CLOSESHAVE);
        if (mNewWallNutAndSunFlowerAndChomperOnly && mApp->IsSeedPlantingLevel()
            && !HasConveyorBeltSeedBank(false))
        {
            for (int packet = 0; packet < mSeedBank[0]->mNumPackets; ++packet)
            {
                const SeedType type = mSeedBank[0]->mSeedPackets[packet].mPacketType;
                if (type == SEED_CHOMPER || type == SEED_SUNFLOWER || type == SEED_WALLNUT)
                {
                    GrantSharedBoardAchievement(mApp, ACHIEVEMENT_CHOMP);
                    break;
                }
            }
        }
    }
}

// Copyright (C) 2023-2026 PvZ TV Touch Team. GPL-3.0-or-later.
// Native adaptation, 2026-10-09. The TV ShowHouseScreen @0x2B344C is a stub;
// house scene coordinates and layer order come from the supplied console port.
#include "LeaderboardsWidget.h"
#include "AchievementSystem.h"
#include "GameButton.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "WidgetManager.h"
#include "game/Board.h"

namespace
{
const AchievementType kDecorationAchievements[] = {ACHIEVEMENT_HOME_SECURITY,
    ACHIEVEMENT_MORTICULTURALIST,
    ACHIEVEMENT_TREE,
    ACHIEVEMENT_GARG,
    ACHIEVEMENT_COOP,
    ACHIEVEMENT_IMMORTAL,
    ACHIEVEMENT_VERSUS,
    ACHIEVEMENT_SOILPLANTS,
    ACHIEVEMENT_EXPLODONATOR,
    ACHIEVEMENT_CLOSESHAVE,
    ACHIEVEMENT_SHOP,
    ACHIEVEMENT_CHOMP};
const int kDecorationDrawOrder[] = {9, 10, 8, 5, 2, 11, 7, 6, 4, 3, 1, 0};
const Sexy::FPoint kDecorationPositions[] = {{198, 496},
    {210, 492},
    {222, 306},
    {405, 501},
    {368, 500},
    {690, 478},
    {615, 487},
    {524, 326},
    {756, 373},
    {678, 390},
    {791, 278},
    {430, 362}};
const Sexy::Rect kDecorationRects[12][2] = {{{253, 594, 485, 62}, {833, 528, 198, 68}},
    {{209, 488, 91, 106}, {0, 0, 0, 0}},
    {{269, 330, 185, 122}, {298, 453, 91, 91}},
    {{452, 550, 151, 76}, {446, 512, 68, 37}},
    {{373, 504, 83, 34}, {389, 533, 63, 37}},
    {{744, 516, 86, 97}, {0, 0, 0, 0}},
    {{632, 490, 52, 40}, {636, 530, 60, 33}},
    {{561, 340, 77, 196}, {525, 404, 145, 56}},
    {{880, 384, 78, 82}, {891, 467, 26, 68}},
    {{715, 416, 104, 81}, {765, 384, 54, 35}},
    {{817, 332, 113, 171}, {850, 298, 46, 31}},
    {{456, 362, 43, 122}, {461, 484, 71, 30}}};
}

LeaderboardsWidget::LeaderboardsWidget(LawnApp *theApp, bool theFromGame) :
    mApp(theApp),
    mFromGame(theFromGame)
{
    Resize(LawnApp::FULLSCREEN_RECT);
    mClip = false;
    mApp->RefreshProfileAchievements();
    mApp->mPlayerInfo->UpdateHouseData();
    const ReanimationType backgrounds[] = {
        REANIM_HOUSE, REANIM_HOUSE_SCENE, REANIM_HOUSE_BG_TREES, REANIM_HOUSE_BG_HOUSES, REANIM_HOUSE_CLOUDS};
    for (int layer = 0; layer < 5; ++layer)
    {
        mBackgrounds[layer] = std::make_unique<Reanimation>();
        Reanimation &animation = *mBackgrounds[layer];
        animation.ReanimationInitializeType(0.0f, 0.0f, backgrounds[layer]);
        if (layer == 0)
        {
            mApp->SetHouseReanim(&animation);
            animation.SetPosition(456.9f, 129.3f);
        }
        else
            animation.PlayReanim(layer == 4 ? "anim_float" : "anim_idle", REANIM_LOOP, 0, layer == 4 ? 2.0f : 12.0f);
        animation.Update();
    }
    Reanimation &scene = *mBackgrounds[1];
    scene.AssignRenderGroupToTrack("survival button 1", 1);
    Sexy::Image *survival = mApp->GetConsoleImage("survival_button");
    if (survival != nullptr)
        scene.SetImageOverride("survival button 1", survival);
    scene.AssignRenderGroupToTrack("house 1", -1);
    scene.AssignRenderGroupToTrack("house achievements 1", -1);

    mPlantTrash = std::make_unique<TrashBin>(
        TrashBin::PLANT_PILE, float(mApp->mPlayerInfo->mGameStats.GetMiscStat(STAT_PLANTS_KILLED)) / 125.0f);
    mZombieTrash = std::make_unique<TrashBin>(
        TrashBin::ZOMBIE_PILE, float(mApp->mPlayerInfo->mGameStats.GetMiscStat(STAT_ZOMBIES_KILLED)) / 125.0f);
    const auto placeTrash = [this](TrashBin &trash, const char *track)
    {
        const int index = mBackgrounds[0]->FindTrackIndex(track);
        if (index < 0)
            return;
        SexyTransform2D matrix;
        mBackgrounds[0]->GetTrackMatrix(index, matrix);
        trash.Move(int(matrix.m02), int(matrix.m12));
    };
    placeTrash(*mPlantTrash, "plant_trash");
    placeTrash(*mZombieTrash, "zombie_trash");

    for (int decoration = 0; decoration < 12; ++decoration)
    {
        mDecorations[decoration] = std::make_unique<Reanimation>();
        Reanimation &animation = *mDecorations[decoration];
        animation.ReanimationInitializeType(kDecorationPositions[decoration].mX,
            kDecorationPositions[decoration].mY,
            GetAchievementDefinition(kDecorationAchievements[decoration]).mReanimationType);
        animation.mLoopType = REANIM_LOOP;
        animation.Update();
    }
    mBackButton = MakeButton(1000, this, this, "[CLOSE]");
    mBackButton->mMousePreservesFocus = true;
    // Leave room for the controller's BACK prompt below the mouse button.
    mBackButton->Resize(1040, 540, 120, 50);
    mBackButton->SetViewportRightAnchor(120);
}

LeaderboardsWidget::~LeaderboardsWidget()
{
    delete mBackButton;
}

void LeaderboardsWidget::AddedToManager(Sexy::WidgetManager *theManager)
{
    Sexy::Widget::AddedToManager(theManager);
    AddWidget(mBackButton);
}

void LeaderboardsWidget::RemovedFromManager(Sexy::WidgetManager *theManager)
{
    RemoveWidget(mBackButton);
    Sexy::Widget::RemovedFromManager(theManager);
}

void LeaderboardsWidget::GotFocus()
{
    Sexy::Widget::GotFocus();
    mApp->SetHouseReanim(mBackgrounds[0].get());
    mApp->ForceHideHelpBarWidget();
    HelpBarWidget &help = *mApp->mHelpBarWidget;
    help.AddButton(GAMEPAD_BUTTON_X, "[LOCAL_RECORDS]", HelpBarWidget::HELP_ALIGN_LEFT);
    help.AddButton(GAMEPAD_BUTTON_B, "[BACK]", HelpBarWidget::HELP_ALIGN_RIGHT);
    mApp->ShowHelpBarWidget();
}

void LeaderboardsWidget::Update()
{
    Sexy::Widget::Update();
    for (const auto &background : mBackgrounds)
        background->Update();
    for (int decoration = 0; decoration < 12; ++decoration)
        if (mApp->mPlayerInfo->mAchievements[kDecorationAchievements[decoration]])
            mDecorations[decoration]->Update();
    MarkDirty();
}

void LeaderboardsWidget::Draw(Sexy::Graphics *theGraphics)
{
    for (int layer = 4; layer >= 0; --layer)
        mBackgrounds[layer]->DrawRenderGroup(theGraphics, 0);
    mPlantTrash->Draw(theGraphics);
    mZombieTrash->Draw(theGraphics);
    for (int decoration : kDecorationDrawOrder)
    {
        if (!mApp->mPlayerInfo->mAchievements[kDecorationAchievements[decoration]])
            continue;
        mDecorations[decoration]->Draw(theGraphics);
        if (!mHighlightDecoration || decoration != mFocusedDecoration)
            continue;
        const int iconX = int(kDecorationPositions[decoration].mX) + 20;
        const int iconY = int(kDecorationPositions[decoration].mY) - 200;
        Sexy::Image *icon = GetAchievementIcon(kDecorationAchievements[decoration]);
        if (icon != nullptr)
            theGraphics->DrawImage(icon, iconX, iconY);
        TodDrawStringWrapped(theGraphics,
            GetAchievementTitle(kDecorationAchievements[decoration]),
            Sexy::Rect(iconX - 42, iconY + 125, 200, 200),
            FONT_HOUSEOFTERROR28,
            Sexy::Color(0, 255, 0),
            2,
            false);
    }
    if (mApp->HasFinishedAdventure())
    {
        mBackgrounds[1]->DrawRenderGroup(theGraphics, 1);
        Sexy::String record;
        StrFormat(&record, "%d", mApp->mPlayerInfo->mGameStats.GetMiscStat(STAT_ENDLESS_FLAGS));
        TodDrawStringWrapped(theGraphics,
            record,
            Sexy::Rect(317, 658, 120, 50),
            FONT_CONTINUUMBOLD14,
            Sexy::Color(255, 255, 0),
            2,
            false);
    }
    Sexy::String title = mApp->GetString("PLAYERS_HOUSE", u8"{PLAYER}的房子");
    Sexy::String namedTitle;
    TodReplaceString(&namedTitle, title, "{PLAYER}", mApp->mPlayerInfo->GetName());
    title = namedTitle;
    TodDrawStringWrapped(
        theGraphics, title, Sexy::Rect(240, 70, 800, 70), FONT_HOUSEOFTERROR28, Sexy::Color::White, 2, false);
}

void LeaderboardsWidget::ShowRecords()
{
    GameStats &stats = mApp->mPlayerInfo->mGameStats;
    Sexy::String text;
    const Sexy::String format = TodStringTranslate("[LOCAL_RECORDS_BODY]");
    StrFormat(&text,
        format.c_str(),
        stats.GetMiscStat(STAT_ZOMBIES_KILLED),
        stats.GetMiscStat(STAT_PLANTS_KILLED),
        mApp->mPlayerInfo->mCoins * 10,
        mApp->mPlayerInfo->mUsedCoins * 10,
        stats.GetMiscStat(STAT_TREE_HEIGHT),
        stats.GetMiscStat(STAT_ENDLESS_FLAGS),
        mApp->mPlayerInfo->mWinStreak);
    mApp->LawnMessageBox(51, "[LOCAL_RECORDS]", text.c_str(), "[DIALOG_BUTTON_OK]", "", 3);
    mApp->mWidgetManager->SetFocus(this);
}

void LeaderboardsWidget::MouseDown(int x, int y, int theClickCount)
{
    if (theClickCount <= 0 || theClickCount == 3)
        return;
    for (int order = 11; order >= 0; --order)
    {
        const int decoration = kDecorationDrawOrder[order];
        if (mApp->mPlayerInfo->mAchievements[kDecorationAchievements[decoration]] &&
            (kDecorationRects[decoration][0].Contains(x, y) || kDecorationRects[decoration][1].Contains(x, y)))
        {
            mHighlightDecoration = !mHighlightDecoration || mFocusedDecoration != decoration;
            mFocusedDecoration = decoration;
            return;
        }
    }
    for (TrashBin *trash : {mPlantTrash.get(), mZombieTrash.get()})
    {
        const bool plantPile = trash->mTrashPileType == TrashBin::PLANT_PILE;
        Sexy::Image *can = mApp->GetConsoleImage(plantPile ? "plant_can" : "zombie_can");
        const int height = int(trash->mTrashHeight);
        if (can != nullptr &&
            Sexy::Rect(trash->mX - (plantPile ? 22 : 0), trash->mY - height, can->mWidth, height + can->mHeight + 15)
                .Contains(x, y))
        {
            ShowRecords();
            return;
        }
    }
    Sexy::Image *survival = mApp->GetConsoleImage("survival_button");
    if (!mFromGame && mApp->HasFinishedAdventure() && survival != nullptr &&
        Sexy::Rect(270, 579, survival->mWidth, survival->mHeight).Contains(x, y))
    {
        LawnApp *app = mApp;
        app->KillLeaderboardScreen();
        app->PreNewGame(GAMEMODE_SURVIVAL_ENDLESS_STAGE_3, true);
    }
}

int LeaderboardsWidget::KeyDown(Sexy::KeyCode theKey)
{
    int button = GAMEPAD_BUTTON_NONE;
    int player = 0;
    unsigned int flags = 0;
    if (mEvent != nullptr && mApp->MapToButtonEvent(mEvent, button, player, flags))
    {
        GameButtonDown(Sexy::GamepadButton(button), player, flags);
        return 1;
    }
    switch (theKey)
    {
        case Sexy::KEYCODE_ESCAPE:
        case Sexy::KEYCODE_GAMEPAD_B:
            GameButtonDown(GAMEPAD_BUTTON_B, 0, 0);
            return 1;
        case Sexy::KEYCODE_RETURN:
        case Sexy::KEYCODE_SPACE:
        case Sexy::KEYCODE_GAMEPAD_A:
            GameButtonDown(GAMEPAD_BUTTON_A, 0, 0);
            return 1;
        case Sexy::KEYCODE_GAMEPAD_X:
            GameButtonDown(GAMEPAD_BUTTON_X, 0, 0);
            return 1;
        case Sexy::KEYCODE_GAMEPAD_Y:
            GameButtonDown(GAMEPAD_BUTTON_Y, 0, 0);
            return 1;
        case Sexy::KEYCODE_UP:
        case Sexy::KEYCODE_GAMEPAD_UP:
        case Sexy::KEYCODE_GAMEPAD_DPAD_UP:
            GameButtonDown(GAMEPAD_BUTTON_UP, 0, 0);
            return 1;
        case Sexy::KEYCODE_DOWN:
        case Sexy::KEYCODE_GAMEPAD_DOWN:
        case Sexy::KEYCODE_GAMEPAD_DPAD_DOWN:
            GameButtonDown(GAMEPAD_BUTTON_DOWN, 0, 0);
            return 1;
        case Sexy::KEYCODE_LEFT:
        case Sexy::KEYCODE_GAMEPAD_LEFT:
        case Sexy::KEYCODE_GAMEPAD_DPAD_LEFT:
            GameButtonDown(GAMEPAD_BUTTON_LEFT, 0, 0);
            return 1;
        case Sexy::KEYCODE_RIGHT:
        case Sexy::KEYCODE_GAMEPAD_RIGHT:
        case Sexy::KEYCODE_GAMEPAD_DPAD_RIGHT:
            GameButtonDown(GAMEPAD_BUTTON_RIGHT, 0, 0);
            return 1;
        default:
            return Sexy::Widget::KeyDown(theKey);
    }
}

void LeaderboardsWidget::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
    const bool previous = theButton == GAMEPAD_BUTTON_UP || theButton == GAMEPAD_BUTTON_LEFT ||
        theButton == GAMEPAD_BUTTON_DPAD_UP || theButton == GAMEPAD_BUTTON_DPAD_LEFT;
    const bool next = theButton == GAMEPAD_BUTTON_DOWN || theButton == GAMEPAD_BUTTON_RIGHT ||
        theButton == GAMEPAD_BUTTON_DPAD_DOWN || theButton == GAMEPAD_BUTTON_DPAD_RIGHT;
    if ((theFlags & 1) != 0 && !previous && !next)
        return;
    if (theButton == GAMEPAD_BUTTON_B)
    {
        if (mHighlightDecoration)
            mHighlightDecoration = false;
        else
            Close();
        return;
    }
    if (theButton == GAMEPAD_BUTTON_X || theButton == GAMEPAD_BUTTON_Y)
    {
        ShowRecords();
        return;
    }
    if (previous || next)
    {
        int decoration = mFocusedDecoration;
        for (int count = 0; count < 12; ++count)
        {
            decoration = (decoration + (previous ? 1 : 11)) % 12;
            if (mApp->mPlayerInfo->mAchievements[kDecorationAchievements[decoration]])
            {
                mFocusedDecoration = decoration;
                mHighlightDecoration = true;
                break;
            }
        }
    }
}

void LeaderboardsWidget::ButtonDepress(int theId)
{
    if (theId == 1000)
        Close();
}

void LeaderboardsWidget::Close()
{
    LawnApp *app = mApp;
    const bool fromGame = mFromGame;
    app->KillLeaderboardScreen();
    if (fromGame)
    {
        Sexy::Widget *options = app->GetDialog(2);
        if (options != nullptr)
            app->mWidgetManager->SetFocus(options);
        else if (app->mBoard != nullptr)
        {
            app->mWidgetManager->SetFocus(app->mBoard);
            app->mBoard->RebuildHelpBar();
        }
    }
    else
        app->ShowMainMenuScreen();
}

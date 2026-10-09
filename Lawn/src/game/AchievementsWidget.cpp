// Copyright (C) 2023-2026 PvZ TV Touch Team. GPL-3.0-or-later.
// Adapted on 2026-10-09; layout from the reference AchievementsWidget::Draw.
#include "AchievementsWidget.h"
#include "AchievementSystem.h"
#include "LawnApp.h"
#include "GameButton.h"
#include "Graphics.h"
#include "Image.h"
#include "WidgetManager.h"
#include "game/Board.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int kHoleLength = 136;
const char* HoleImageName(int thePanel)
{
    switch (thePanel)
    {
    case 16: return "hole_worm";
    case 25: return "hole_gems";
    case 38: return "hole_chuzzle";
    case 52: return "hole_bjorn";
    case 69: return "hole_pipe";
    case 85: return "hole_tiki";
    case 101: return "hole_heavyrocks";
    case 114: return "hole_duwei";
    case kHoleLength: return "hole_china";
    default: return "hole";
    }
}
}

AchievementsWidget::AchievementsWidget(LawnApp* theApp, bool theFromGame)
    : mApp(theApp), mFromGame(theFromGame)
{
    mClip = false;
    Resize(LawnApp::FULLSCREEN_RECT);
    mApp->RefreshProfileAchievements();
    mBackButton = MakeButton(1000, this, this, "[CLOSE]");
    // Reference MainMenu's achievements close button, after the 720px slide.
    mBackButton->Resize(1000, 564, 170, 50);
}

AchievementsWidget::~AchievementsWidget()
{
    delete mBackButton;
}

void AchievementsWidget::AddedToManager(Sexy::WidgetManager* theManager)
{
    Sexy::Widget::AddedToManager(theManager);
    AddWidget(mBackButton);
}

void AchievementsWidget::RemovedFromManager(Sexy::WidgetManager* theManager)
{
    RemoveWidget(mBackButton);
    Sexy::Widget::RemovedFromManager(theManager);
}

void AchievementsWidget::GotFocus()
{
    Sexy::Widget::GotFocus();
    mApp->ForceHideHelpBarWidget();
}

void AchievementsWidget::ScrollTo(int theY)
{
    Sexy::Image* hole = mApp->GetConsoleImage("hole");
    if (hole == nullptr)
        return;
    mScrollY = std::clamp(theY, 0, std::max(0, (kHoleLength + 1) * hole->mHeight - mHeight));
    MarkDirty();
}

void AchievementsWidget::Update()
{
    Sexy::Widget::Update();
    if (!mDragging && std::fabs(mScrollVelocity) > 0.5f)
    {
        const int previous = mScrollY;
        ScrollTo(mScrollY + int(mScrollVelocity));
        mScrollVelocity *= 0.92f;
        if (previous == mScrollY)
            mScrollVelocity = 0.0f;
    }
}

void AchievementsWidget::Draw(Sexy::Graphics* theGraphics)
{
    theGraphics->SetColor(Sexy::Color(31, 20, 12));
    theGraphics->FillRect(0, 0, mWidth, mHeight);
    Sexy::Image* hole = mApp->GetConsoleImage("hole");
    if (hole == nullptr || hole->mHeight <= 0)
        return;
    const int panelHeight = hole->mHeight;
    theGraphics->PushState();
    theGraphics->ClipRect(0, 0, mWidth, mHeight);
    theGraphics->Translate(0, -mScrollY);
    const int firstPanel = mScrollY / panelHeight;
    const int lastPanel = std::min(kHoleLength, (mScrollY + mHeight) / panelHeight);
    for (int panel = firstPanel; panel <= lastPanel; ++panel)
    {
        Sexy::Image* image = mApp->GetConsoleImage(HoleImageName(panel));
        if (image != nullptr)
            theGraphics->DrawImage(image, 0, panel * panelHeight);
    }
    if (firstPanel == 0)
    {
        Sexy::Image* top = mApp->GetConsoleImage("hole_top");
        if (top != nullptr)
            theGraphics->DrawImage(top, 0, 0);
    }
    int completed = 0;
    for (int index = 0; index < NUM_ACHIEVEMENT_TYPES; ++index)
    {
        const AchievementType achievement = AchievementType(index);
        const bool unlocked = mApp->mPlayerInfo->mAchievements[index];
        completed += unlocked ? 1 : 0;
        const int achievementY = 300 + index * (panelHeight * 2 / 3);
        if (achievementY + 150 < mScrollY || achievementY - 5 > mScrollY + mHeight)
            continue;
        Sexy::Image* icon = GetAchievementIcon(achievement);
        theGraphics->SetColorizeImages(!unlocked);
        theGraphics->SetColor(Sexy::Color(128, 128, 128));
        if (icon != nullptr)
            theGraphics->DrawImage(icon, 330, achievementY - 5);
        theGraphics->SetColorizeImages(false);
        TodDrawString(theGraphics, GetAchievementTitle(achievement), 460, achievementY + 40,
                      FONT_HOUSEOFTERROR28, Sexy::Color(0, 255, 0), 0);
        TodDrawStringWrapped(theGraphics, GetAchievementDescription(achievement),
                             Sexy::Rect(460, achievementY + 60, 540, 150), FONT_HOUSEOFTERROR20,
                             unlocked ? Sexy::Color::White : Sexy::Color(160, 160, 160), 0, false);
    }
    Sexy::String count;
    StrFormat(&count, "%d/%d", completed, NUM_ACHIEVEMENT_TYPES);
    TodDrawString(theGraphics, count, 1060, 173, FONT_DWARVENTODCRAFT18,
                  Sexy::Color(255, 240, 0), 2);
    theGraphics->PopState();
}

void AchievementsWidget::MouseDown(int x, int y, int theClickCount)
{
    if (theClickCount <= 0 || theClickCount == 3)
        return;
    mDragging = true;
    mDragStartY = y;
    mDragStartScroll = mScrollY;
    mScrollVelocity = 0.0f;
}

void AchievementsWidget::MouseDrag(int x, int y)
{
    if (!mDragging)
        return;
    const int previous = mScrollY;
    ScrollTo(mDragStartScroll + mDragStartY - y);
    mScrollVelocity = mScrollVelocity * 0.75f + float(mScrollY - previous) * 0.25f;
}

void AchievementsWidget::MouseUp(int x, int y, int theClickCount)
{
    mDragging = false;
}

void AchievementsWidget::MouseWheel(int theDelta)
{
    mScrollVelocity = 0.0f;
    ScrollTo(mScrollY - theDelta * 100);
}

int AchievementsWidget::KeyDown(Sexy::KeyCode theKey)
{
    int button = GAMEPAD_BUTTON_NONE;
    int player = 0;
    unsigned int flags = 0;
    if (mEvent != nullptr && mApp->MapToButtonEvent(mEvent, button, player, flags))
    {
        switch (button)
        {
        case GAMEPAD_BUTTON_B: theKey = Sexy::KEYCODE_ESCAPE; break;
        case GAMEPAD_BUTTON_UP:
        case GAMEPAD_BUTTON_DPAD_UP: theKey = Sexy::KEYCODE_UP; break;
        case GAMEPAD_BUTTON_DOWN:
        case GAMEPAD_BUTTON_DPAD_DOWN: theKey = Sexy::KEYCODE_DOWN; break;
        case GAMEPAD_BUTTON_LEFT:
        case GAMEPAD_BUTTON_DPAD_LEFT: theKey = Sexy::KEYCODE_PRIOR; break;
        case GAMEPAD_BUTTON_RIGHT:
        case GAMEPAD_BUTTON_DPAD_RIGHT: theKey = Sexy::KEYCODE_NEXT; break;
        default: break;
        }
    }
    switch (theKey)
    {
    case Sexy::KEYCODE_ESCAPE:
    case Sexy::KEYCODE_GAMEPAD_B: ButtonDepress(1000); return 1;
    case Sexy::KEYCODE_UP: ScrollTo(mScrollY - 100); return 1;
    case Sexy::KEYCODE_DOWN: ScrollTo(mScrollY + 100); return 1;
    case Sexy::KEYCODE_PRIOR: ScrollTo(mScrollY - mHeight); return 1;
    case Sexy::KEYCODE_NEXT: ScrollTo(mScrollY + mHeight); return 1;
    case Sexy::KEYCODE_HOME: ScrollTo(0); return 1;
    case Sexy::KEYCODE_END: ScrollTo(100000); return 1;
    default: return Sexy::Widget::KeyDown(theKey);
    }
}

void AchievementsWidget::ButtonDepress(int theId)
{
    if (theId != 1000)
        return;
    LawnApp* app = mApp;
    const bool fromGame = mFromGame;
    app->KillAchievementsScreen();
    if (fromGame)
    {
        Sexy::Widget* options = app->GetDialog(2);
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

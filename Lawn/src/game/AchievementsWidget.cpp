// Copyright (C) 2023-2026 PvZ TV Touch Team. GPL-3.0-or-later.
// Adapted on 2026-10-09; layout from the reference AchievementsWidget::Draw.
#include "AchievementsWidget.h"
#include "AchievementSystem.h"
#include "LawnApp.h"
#include "MainMenu.h"
#include "GameButton.h"
#include "Graphics.h"
#include "Image.h"
#include "WidgetManager.h"
#include "game/Board.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr int kHoleLength = 136;
constexpr int kMenuSlideFrames = 100;
constexpr int kReturnToTopFrames = 50;
constexpr int kKeyboardScrollFrames = 20;
constexpr int kKeyboardScrollDistance = 192;
constexpr int kMenuSlideDistance = 720;
// Reference MAIN_MENU_HEIGHT: overlap the two backgrounds by two pixels.
constexpr int kMenuJoinOffset = kMenuSlideDistance - 2;
constexpr int kMouseInputFlag = 16;
const char *HoleImageName(int thePanel)
{
    switch (thePanel)
    {
        case 16:
            return "hole_worm";
        case 25:
            return "hole_gems";
        case 38:
            return "hole_chuzzle";
        case 52:
            return "hole_bjorn";
        case 69:
            return "hole_pipe";
        case 85:
            return "hole_tiki";
        case 101:
            return "hole_heavyrocks";
        case 114:
            return "hole_duwei";
        case kHoleLength:
            return "hole_china";
        default:
            return "hole";
    }
}
}

AchievementsWidget::AchievementsWidget(LawnApp *theApp, bool theFromGame) :
    mApp(theApp),
    mFromGame(theFromGame)
{
    mClip = false;
    Resize(LawnApp::FULLSCREEN_RECT);
    mApp->RefreshProfileAchievements();
    mBackButton = MakeButton(1000, this, this, "[CLOSE]");
    // Reference MainMenu's achievements close button, after the 720px slide.
    mBackButton->Resize(1000, 564, 170, 50);
    mBackButton->SetViewportRightAnchor(110);
    mBackButton->mMousePreservesFocus = true;
    if (!mFromGame && mApp->mGameSelector != nullptr)
    {
        mReturnMenu = mApp->mGameSelector;
        mMenuStartY = mReturnMenu->mY;
        mMenuWasDisabled = mReturnMenu->mDisabled;
        mMenuRemovedFlags = mReturnMenu->mWidgetFlagsMod.mRemoveFlags;
        mReturnMenu->mWidgetFlagsMod.mRemoveFlags |= kMouseInputFlag;
        mReturnMenu->SetDisabled(true);
        mTransition = Transition::Entering;
        mTransitionCounter = kMenuSlideFrames;
        // Keep the viewport covered after moving the page two pixels above its edge.
        Resize(mReturnMenu->mX, mMenuStartY + kMenuJoinOffset, mWidth, mHeight + kMenuSlideDistance - kMenuJoinOffset);
        mBackButton->Move(mBackButton->mX, mBackButton->mY + kMenuSlideDistance - kMenuJoinOffset);
        mBackButton->SetVisible(false);
    }
}

AchievementsWidget::~AchievementsWidget()
{
    delete mBackButton;
}

void AchievementsWidget::AddedToManager(Sexy::WidgetManager *theManager)
{
    Sexy::Widget::AddedToManager(theManager);
    AddWidget(mBackButton);
}

void AchievementsWidget::RemovedFromManager(Sexy::WidgetManager *theManager)
{
    RestoreMainMenu();
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
    Sexy::Image *hole = mApp->GetConsoleImage("hole");
    if (hole == nullptr)
        return;
    mScrollY = std::clamp(theY, 0, std::max(0, (kHoleLength + 1) * hole->mHeight - mHeight));
    MarkDirty();
}

void AchievementsWidget::Update()
{
    Sexy::Widget::Update();
    if (mTransition != Transition::None)
    {
        UpdateTransition();
        return;
    }
    if (mKeyScrollCounter > 0)
    {
        --mKeyScrollCounter;
        ScrollTo(TodAnimateCurve(
            kKeyboardScrollFrames, 0, mKeyScrollCounter, mKeyScrollStartY, mKeyScrollTargetY, CURVE_LINEAR));
        return;
    }
    if (!mDragging && std::fabs(mScrollVelocity) > 0.5f)
    {
        const int previous = mScrollY;
        ScrollTo(mScrollY + int(mScrollVelocity));
        mScrollVelocity *= 0.92f;
        if (previous == mScrollY)
            mScrollVelocity = 0.0f;
    }
}

// MainMenu.cpp in the supplied console port slides 720px over 100 frames.
// A scrolled page spends the first 50 return frames moving back to its top.
void AchievementsWidget::UpdateTransition()
{
    --mTransitionCounter;
    if (mTransition == Transition::ReturningToTop)
    {
        ScrollTo(TodAnimateCurve(kReturnToTopFrames, 0, mTransitionCounter, mReturnScrollY, 0, CURVE_EASE_IN_OUT));
        if (mTransitionCounter == 0)
        {
            mTransition = Transition::Leaving;
            mTransitionCounter = kMenuSlideFrames;
        }
        return;
    }
    const bool entering = mTransition == Transition::Entering;
    const int slide = TodAnimateCurve(kMenuSlideFrames,
        0,
        mTransitionCounter,
        entering ? 0 : kMenuSlideDistance,
        entering ? kMenuSlideDistance : 0,
        CURVE_EASE_IN_OUT);
    if (mReturnMenu != nullptr && mApp->mGameSelector == mReturnMenu)
        mReturnMenu->Move(mReturnMenu->mX, mMenuStartY - slide);
    Move(mX, mMenuStartY + kMenuJoinOffset - slide);
    MarkDirty();
    if (mTransitionCounter != 0)
        return;
    if (entering)
    {
        mTransition = Transition::None;
        mBackButton->SetVisible(true);
    }
    else
        FinishClose();
}

void AchievementsWidget::RestoreMainMenu()
{
    if (mReturnMenu == nullptr || mApp->mGameSelector != mReturnMenu)
        return;
    mReturnMenu->Move(mReturnMenu->mX, mMenuStartY);
    mReturnMenu->mWidgetFlagsMod.mRemoveFlags = mMenuRemovedFlags;
    mReturnMenu->SetDisabled(mMenuWasDisabled);
}

void AchievementsWidget::ScrollByKey(int theAmount)
{
    if (mKeyScrollCounter != 0)
        return;
    mScrollVelocity = 0.0f;
    mKeyScrollStartY = mScrollY;
    ScrollTo(mScrollY + theAmount);
    mKeyScrollTargetY = mScrollY;
    mScrollY = mKeyScrollStartY;
    mKeyScrollCounter = kKeyboardScrollFrames;
}

void AchievementsWidget::Draw(Sexy::Graphics *theGraphics)
{
    theGraphics->SetColor(Sexy::Color(31, 20, 12));
    theGraphics->FillRect(0, 0, mWidth, mHeight);
    Sexy::Image *hole = mApp->GetConsoleImage("hole");
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
        Sexy::Image *image = mApp->GetConsoleImage(HoleImageName(panel));
        if (image != nullptr)
            theGraphics->DrawImage(image, 0, panel * panelHeight);
    }
    if (firstPanel == 0)
    {
        Sexy::Image *top = mApp->GetConsoleImage("hole_top");
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
        Sexy::Image *icon = GetAchievementIcon(achievement);
        theGraphics->SetColorizeImages(!unlocked);
        theGraphics->SetColor(Sexy::Color(128, 128, 128));
        if (icon != nullptr)
            theGraphics->DrawImage(icon, 330, achievementY - 5);
        theGraphics->SetColorizeImages(false);
        TodDrawString(theGraphics,
            GetAchievementTitle(achievement),
            460,
            achievementY + 40,
            FONT_HOUSEOFTERROR28,
            Sexy::Color(0, 255, 0),
            0);
        TodDrawStringWrapped(theGraphics,
            GetAchievementDescription(achievement),
            Sexy::Rect(460, achievementY + 60, 540, 150),
            FONT_HOUSEOFTERROR20,
            unlocked ? Sexy::Color::White : Sexy::Color(160, 160, 160),
            0,
            false);
    }
    Sexy::String count;
    StrFormat(&count, "%d/%d", completed, NUM_ACHIEVEMENT_TYPES);
    TodDrawString(theGraphics, count, 1060, 173, FONT_DWARVENTODCRAFT18, Sexy::Color(255, 240, 0), 2);
    theGraphics->PopState();
}

void AchievementsWidget::MouseDown(int x, int y, int theClickCount)
{
    if (mTransition != Transition::None || theClickCount <= 0 || theClickCount == 3)
        return;
    mKeyScrollCounter = 0;
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
    if (mTransition != Transition::None)
        return;
    mKeyScrollCounter = 0;
    mScrollVelocity = 0.0f;
    ScrollTo(mScrollY - theDelta * 100);
}

int AchievementsWidget::KeyDown(Sexy::KeyCode theKey)
{
    if (mTransition != Transition::None)
        return 1;
    int button = GAMEPAD_BUTTON_NONE;
    int player = 0;
    unsigned int flags = 0;
    if (mEvent != nullptr && mApp->MapToButtonEvent(mEvent, button, player, flags))
    {
        switch (button)
        {
            case GAMEPAD_BUTTON_B:
                if ((flags & 1) != 0)
                    return 1;
                theKey = Sexy::KEYCODE_ESCAPE;
                break;
            case GAMEPAD_BUTTON_UP:
            case GAMEPAD_BUTTON_DPAD_UP:
                theKey = Sexy::KEYCODE_UP;
                break;
            case GAMEPAD_BUTTON_DOWN:
            case GAMEPAD_BUTTON_DPAD_DOWN:
                theKey = Sexy::KEYCODE_DOWN;
                break;
            case GAMEPAD_BUTTON_LEFT:
            case GAMEPAD_BUTTON_DPAD_LEFT:
                theKey = Sexy::KEYCODE_PRIOR;
                break;
            case GAMEPAD_BUTTON_RIGHT:
            case GAMEPAD_BUTTON_DPAD_RIGHT:
                theKey = Sexy::KEYCODE_NEXT;
                break;
            default:
                break;
        }
    }
    switch (theKey)
    {
        case Sexy::KEYCODE_ESCAPE:
        case Sexy::KEYCODE_GAMEPAD_B:
            ButtonDepress(1000);
            return 1;
        case Sexy::KEYCODE_UP:
        case Sexy::KEYCODE_GAMEPAD_UP:
        case Sexy::KEYCODE_GAMEPAD_DPAD_UP:
            ScrollByKey(-kKeyboardScrollDistance);
            return 1;
        case Sexy::KEYCODE_DOWN:
        case Sexy::KEYCODE_GAMEPAD_DOWN:
        case Sexy::KEYCODE_GAMEPAD_DPAD_DOWN:
            ScrollByKey(kKeyboardScrollDistance);
            return 1;
        case Sexy::KEYCODE_PRIOR:
        case Sexy::KEYCODE_GAMEPAD_LEFT:
        case Sexy::KEYCODE_GAMEPAD_DPAD_LEFT:
            ScrollByKey(-mHeight);
            return 1;
        case Sexy::KEYCODE_NEXT:
        case Sexy::KEYCODE_GAMEPAD_RIGHT:
        case Sexy::KEYCODE_GAMEPAD_DPAD_RIGHT:
            ScrollByKey(mHeight);
            return 1;
        case Sexy::KEYCODE_HOME:
            mKeyScrollCounter = 0;
            ScrollTo(0);
            return 1;
        case Sexy::KEYCODE_END:
            mKeyScrollCounter = 0;
            ScrollTo(100000);
            return 1;
        default:
            return Sexy::Widget::KeyDown(theKey);
    }
}

void AchievementsWidget::ButtonDepress(int theId)
{
    if (theId != 1000)
        return;
    BeginClose();
}

void AchievementsWidget::BeginClose()
{
    if (mTransition != Transition::None)
        return;
    mDragging = false;
    mScrollVelocity = 0.0f;
    mKeyScrollCounter = 0;
    if (mReturnMenu != nullptr && mApp->mGameSelector == mReturnMenu)
    {
        mBackButton->SetVisible(false);
        mReturnScrollY = mScrollY;
        mTransition = mScrollY > 0 ? Transition::ReturningToTop : Transition::Leaving;
        mTransitionCounter = mScrollY > 0 ? kReturnToTopFrames : kMenuSlideFrames;
        return;
    }
    FinishClose();
}

void AchievementsWidget::FinishClose()
{
    LawnApp *app = mApp;
    const bool fromGame = mFromGame;
    MainMenu *returnMenu = app->mGameSelector == mReturnMenu ? mReturnMenu : nullptr;
    app->KillAchievementsScreen();
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
    else if (returnMenu != nullptr)
    {
        app->mWidgetManager->SetFocus(returnMenu);
        returnMenu->SetFocus(returnMenu->FindWidget(MainMenu::ACHIEVEMENTS_BUTTON), true);
    }
    else
        app->ShowMainMenuScreen();
}

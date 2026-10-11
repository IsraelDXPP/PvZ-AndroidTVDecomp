#include "HelpTextScreen.h"

#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnApp.h"
#include "LawnMenuWidget.h"
#include "MenuWidget.h"
#include "Widget.h"
#include "WidgetManager.h"
#include <algorithm>

namespace
{
// HelpTextScreen @0x189C98 spaces pages by 2560: its children draw their
// menu positions again after the widget tree has translated them.
constexpr float HelpPageSpacing = 2560.0f;
constexpr float HelpPageOffset = 15.0f;
constexpr int HelpPageCount = 5;
constexpr float LastHelpPage = HelpPageOffset + (HelpPageCount - 1) * HelpPageSpacing;
}

void HelpTextScreen::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    // Loading a menu can change focus; the shared bar must already be usable.
    mHelpBarWidget = mApp->mHelpBarWidget;
    MenuWidget::AddedToManager(theWidgetManager);
    for (Sexy::Widget *child : mWidgetVector)
        child->mMouseVisible = false;
    // VS and coop open pages beyond the original two-page hit area.
    Resize(mX, mY, static_cast<int>(LastHelpPage) + mApp->mWidth, mApp->mHeight);
}

void HelpTextScreen::MouseDown(int x, int y, int theClickCount)
{
    if (theClickCount < 0)
    {
        mApp->KillHelpTextScreen();
        return;
    }
    if (theClickCount == 0 || theClickCount == 3)
        return;
    const int screenX = x + GetAbsPos().mX;
    GameButtonDown(screenX < mApp->mWidth / 2 ? GAMEPAD_BUTTON_LEFT : GAMEPAD_BUTTON_RIGHT, 0, 0);
}

void HelpTextScreen::DrawAll(Sexy::ModalFlags *theFlags, Sexy::Graphics *g)
{
    MenuWidget::DrawAll(theFlags, g);
}

HelpTextScreen::~HelpTextScreen() {}

void HelpTextScreen::Update()
{
    if (mScrollTo != mScrollFrom)
    {
        mScrollTimer -= 0.016f;
        if (mScrollTimer <= 0.0f)
        {
            mScrollFrom = mScrollTo;
            mScrollTimer = 0.0f;
        }
    }

    mApp->mWidgetManager->BringToFront(this);
    mApp->mWidgetManager->BringToFront(mApp->mHelpBarWidget);

    mX = (int)(-TodAnimateCurveFloatTime(0.0f, 1.0f, mScrollTimer, mScrollTo, mScrollFrom, CURVE_EASE_IN_OUT));

    MenuWidget::Update();
    MarkDirty();
}

void HelpTextScreen::Draw(Sexy::Graphics *g)
{
    gLawnApp->DrawDialogFade(g);
}

int HelpTextScreen::KeyDown(Sexy::KeyCode theKey)
{
    if (theKey == Sexy::KEYCODE_ESCAPE)
    {
        mApp->KillHelpTextScreen();
        return 1;
    }

    if (theKey == Sexy::KEYCODE_LEFT)
    {
        if (mScrollTo <= HelpPageOffset || mScrollTimer != 0.0f)
            return Widget::KeyDown(theKey);
        mScrollTimer = 1.0f;
        mScrollTo -= HelpPageSpacing;
        return 1;
    }

    if (theKey == Sexy::KEYCODE_RIGHT)
    {
        if (mScrollTo >= LastHelpPage || mScrollTimer != 0.0f)
            return Widget::KeyDown(theKey);
        mScrollTimer = 1.0f;
        mScrollTo += HelpPageSpacing;
        return 1;
    }

    return Widget::KeyDown(theKey);
}

void HelpTextScreen::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    MenuWidget::RemovedFromManager(theWidgetManager);
    RemoveAllWidgets(true, true, NULL);
    mApp->HideHelpBarWidget();
}

void HelpTextScreen::GotFocus()
{
    MenuWidget::GotFocus();
    mHelpBarWidget->ClearButtons(0);

    Sexy::String caption = TodStringTranslate("[CHANGE_PAGES]");
    mHelpBarWidget->AddButton(GAMEPAD_BUTTON_UP, caption, HelpBarWidget::HELP_ALIGN_NONE);

    caption = TodStringTranslate("[CLOSE]");
    mHelpBarWidget->AddButton(GAMEPAD_BUTTON_B, caption, HelpBarWidget::HELP_ALIGN_NONE);

    mHelpBarWidget->mDrawShade = false;
    mApp->ShowHelpBarWidget();
}

HelpTextScreen::HelpTextScreen(LawnApp *theApp, int thePage) :
    LawnMenuWidget(Sexy::String("menu/HelpMenu.menu.txt"))
{
    mBackdropReanimID = REANIMATIONID_NULL;
    mCameraOffsetX = 0.0f;
    mCameraOffsetY = 0.0f;

    mApp = theApp;
    mPage = std::clamp(thePage, 0, HelpPageCount - 1);

    const float initial = mPage * HelpPageSpacing + HelpPageOffset;
    mScrollFrom = initial;
    mScrollTo = initial;
    mScrollTimer = 0.0f;
    mHelpBarWidget = mApp->mHelpBarWidget;

    mHasAlpha = true;
    mClip = false;
    mX = static_cast<int>(-initial);
}

void HelpTextScreen::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theModifiers)
{
    (void)thePlayerIndex;
    (void)theModifiers;
    if (theButton == GAMEPAD_BUTTON_B)
    {
        mApp->KillHelpTextScreen();
        return;
    }

    if (theButton == GAMEPAD_BUTTON_LEFT)
    {
        if (mScrollTo > HelpPageOffset && mScrollTimer == 0.0f)
        {
            mScrollTimer = 1.0f;
            mScrollTo -= HelpPageSpacing;
        }
        return;
    }

    if (theButton == GAMEPAD_BUTTON_RIGHT)
    {
        if (mScrollTo < LastHelpPage && mScrollTimer == 0.0f)
        {
            mScrollTimer = 1.0f;
            mScrollTo += HelpPageSpacing;
        }
    }
}

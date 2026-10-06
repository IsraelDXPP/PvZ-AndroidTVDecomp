#include "HelpTextScreen.h"

#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnApp.h"
#include "LawnMenuWidget.h"
#include "MenuWidget.h"
#include "Widget.h"
#include "WidgetManager.h"

void HelpTextScreen::AddedToManager(Sexy::WidgetManager* theWidgetManager)
{
    // Loading a menu can change focus; the shared bar must already be usable.
    mHelpBarWidget = mApp->mHelpBarWidget;
    MenuWidget::AddedToManager(theWidgetManager);
}

void HelpTextScreen::DrawAll(Sexy::ModalFlags* theFlags, Sexy::Graphics* g)
{
    MenuWidget::DrawAll(theFlags, g);
}

HelpTextScreen::~HelpTextScreen()
{
}

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

void HelpTextScreen::Draw(Sexy::Graphics* g)
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
        if (mScrollTo <= 15.0f || mScrollTimer != 0.0f)
            return Widget::KeyDown(theKey);
        mScrollTimer = 1.0f;
        mScrollTo -= 2560.0f;
        return 1;
    }

    if (theKey == Sexy::KEYCODE_RIGHT)
    {
        if (mScrollTo >= 2560.0f || mScrollTimer != 0.0f)
            return Widget::KeyDown(theKey);
        mScrollTimer = 1.0f;
        mScrollTo += 2560.0f;
        return 1;
    }

    return Widget::KeyDown(theKey);
}

void HelpTextScreen::RemovedFromManager(Sexy::WidgetManager* theWidgetManager)
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

HelpTextScreen::HelpTextScreen(LawnApp* theApp, int thePage)
    : LawnMenuWidget(Sexy::String("menu/HelpMenu.menu.txt"))
{
    mBackdropReanimID = REANIMATIONID_NULL;
    mCameraOffsetX = 0.0f;
    mCameraOffsetY = 0.0f;

    mApp = theApp;
    mPage = thePage;

    float initial = (float)thePage * 2560.0f + 15.0f;
    mScrollFrom = initial;
    mScrollTo = initial;
    mScrollTimer = 0.0f;
    mHelpBarWidget = mApp->mHelpBarWidget;

    mHasAlpha = true;
    mClip = false;
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
        if (mScrollTo > 15.0f && mScrollTimer == 0.0f)
        {
            mScrollTimer = 1.0f;
            mScrollTo -= 2560.0f;
        }
        return;
    }

    if (theButton == GAMEPAD_BUTTON_RIGHT)
    {
        if (mScrollTo < 2560.0f && mScrollTimer == 0.0f)
        {
            mScrollTimer = 1.0f;
            mScrollTo += 2560.0f;
        }
    }
}

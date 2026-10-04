// HelpBarWidget -- reconstructed from libGameMain.so.
//
// The bar of gamepad hints at the bottom of a screen. It keeps two button sets:
// mButtons is the incoming one and mButtons2 the outgoing one, and mAlpha cross-
// fades between them as the bar's height animates towards what the current set
// needs. The binary only draws it when a gamepad is attached.

#include "HelpBarWidget.h"
#include "GamepadApp.h"
#include "LawnApp.h"
#include "Graphics.h"
#include "ReconTypes.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// The two alignment values AddButton can assign. The binary only ever stores 0
// and 1 and the call sites pass them as bare integers, so the enumerators are
// not recoverable; Sexy\ReconTypes.h still carries the old placeholder the
// header's HelpAlign enum replaces.
static const HelpBarWidget::HelpAlign HELP_ALIGN_LEFT = (HelpBarWidget::HelpAlign)0;
static const HelpBarWidget::HelpAlign HELP_ALIGN_RIGHT = (HelpBarWidget::HelpAlign)1;

// ==========================================================================
// HelpBarWidget::Draw(Sexy::Graphics *)
// @00148608  356 bytes
// ==========================================================================
void HelpBarWidget::Draw(Sexy::Graphics * theGraphics)
{
    if (mFontId == nullptr)
    {
        // SexyAppBase::GetLocale() @0x33A2E8 is exactly
        // `Sexy::String(Sexy::setLocale(0, 0))`; the recon tree declares the
        // free function but not the accessor, so the check goes through it.
        const char * aLocale = Sexy::setLocale(nullptr, nullptr);
        if (aLocale != nullptr && Sexy::String(aLocale) == "zh_CN")
            mFontId = FONT_HOUSEOFTERROR20;
        else
            mFontId = FONT_HOUSEOFTERROR16;
    }

    // The bar's height animates towards 46 pixels per button row (at least 96),
    // and the counter walks 0..10 in whichever direction the button count just
    // moved.
    if (mButtons.size() > 2)
        --mAnimCounter;
    else
        ++mAnimCounter;
    if (mAnimCounter < 0)
        mAnimCounter = 0;
    else if (mAnimCounter > 10)
        mAnimCounter = 10;

    // gLawnApp is the game's LawnApp, whose GamepadApp base owns the gamepad
    // state at +0x839/+0x895.
    Sexy::GamepadApp * anApp = (Sexy::GamepadApp *)(void *)gLawnApp;
    if (anApp->HasGamepad() != 0 || (anApp->mGamePad1IsOn && anApp->mGamePad2IsOn))
    {
        int aSlide = TodAnimateCurve(0, 10, mAnimCounter, 0, 20, CURVE_EASE_IN_OUT);

        // The bar slides up from below the screen.
        theGraphics->mTransY = theGraphics->mTransY + (float)aSlide;

        if (mDrawShade)
        {
            Color aShade(0, 0, 0, 102);
            theGraphics->SetColor(aShade);
            theGraphics->FillRect(0, 0, mWidth, mHeight - aSlide + 4);
        }

        // Fade the outgoing set out and the incoming set in.
        DrawButtons(theGraphics, mButtons2, 0, 1.0f - mAlpha);
        DrawButtons(theGraphics, mButtons, 1, mAlpha);
    }
}

// ==========================================================================
// HelpBarWidget::HelpBarWidget(void)
// @00148810  174 bytes
// ==========================================================================
HelpBarWidget::HelpBarWidget()
{
    // A global instance counter the binary bumps but nothing here reads.
    int anId = gHelpBarInstanceCount;
    gHelpBarInstanceCount = anId + 1;

    mAlpha = 0.0f;
    mDrawShade = true;
    mState = -1;
    mFontId = nullptr;
    mAnimCounter = 0;

    // The bar sits at the bottom of the screen; a second construction reuses the
    // height the first one settled on.
    int aY;
    Sexy::SexyAppBase * anApp = (Sexy::SexyAppBase *)gLawnApp;
    const unsigned char * aPlatform = (const unsigned char *)anApp->mPlatformDriverOrQueue;
    if (aPlatform[0x60] != 0)
    {
        aY = (int)gHelpBarRestY;
    }
    else
    {
        aY = 504;
        gHelpBarRestY = 504.0f;
    }

    Resize(DEFAULT_POS, aY, LawnApp::FULLSCREEN_RECT.mWidth, 96);
}

// ==========================================================================
// HelpBarWidget::AddButton(Sexy::GamepadButton,Sexy::String,HelpBarWidget::HelpAlign)
// @00148C74  258 bytes
// ==========================================================================
void HelpBarWidget::AddButton(Sexy::GamepadButton theButton, Sexy::String theString, HelpAlign theHelpAlign)
{
    ButtonDef aDef;
    aDef.mButton = theButton;
    aDef.mLabel = theString;
    aDef.mAlign = (int)theHelpAlign;
    aDef.mLeaving = 1;
    aDef.mRow = -1;

    if (theHelpAlign == HELP_ALIGN_NONE)
    {
        // No alignment asked for: give the button to whichever side currently
        // holds fewer, and keep mState in step so the next call knows.
        if (mButtons.empty())
        {
            theHelpAlign = (mState == -1 || mState == 1) ? HELP_ALIGN_LEFT : HELP_ALIGN_RIGHT;
        }
        else
        {
            int aRightCount = 0;
            int aLeftCount = 0;
            for (unsigned int aIndex = 0; aIndex < mButtons.size(); ++aIndex)
            {
                if (mButtons[aIndex].mAlign != 0)
                    ++aRightCount;
                else
                    ++aLeftCount;
            }

            if (aRightCount > aLeftCount)
                theHelpAlign = HELP_ALIGN_LEFT;
            else if (aRightCount < aLeftCount)
                theHelpAlign = HELP_ALIGN_RIGHT;
            else
                theHelpAlign = (mState == -1 || mState == 1) ? HELP_ALIGN_LEFT : HELP_ALIGN_RIGHT;
        }
    }

    aDef.mAlign = (int)theHelpAlign;
    mState = (int)theHelpAlign;
    mButtons.push_back(aDef);
}

// ==========================================================================
// HelpBarWidget::ClearButtons(unsigned int)
// @00148D8C  380 bytes
// ==========================================================================
// Drops the outgoing set and then moves every button from index theFirstOff on
// into it, so the ones that stay are the first theFirstOff of the current set.
void HelpBarWidget::ClearButtons(unsigned int theFirstOff)
{
    mButtons2.clear();

    if (theFirstOff >= mButtons.size())
        return;

    for (unsigned int aIndex = theFirstOff; aIndex < mButtons.size(); ++aIndex)
    {
        mButtons[aIndex].mLeaving = 1;
        mButtons2.push_back(mButtons[aIndex]);
    }

    mButtons.resize(theFirstOff);

    mState = -1;
    mAlpha = 0.0f;
}

// ==========================================================================
// HelpBarWidget::Update(void)
// @00148F10  294 bytes
// ==========================================================================
void HelpBarWidget::Update()
{
    Widget::Update();

    mAlpha = mAlpha + 0.02f;
    if (mAlpha > 1.0f)
        mAlpha = 1.0f;

    if (mAlpha >= 1.0f)
    {
        // The cross-fade finished; the outgoing set is no longer needed.
        mButtons2.clear();
    }

    // Each button row is 46 pixels; the bar never gets shorter than 96.
    int aWantedHeight;
    if (mButtons.size() != 0)
    {
        int aMaxRows = 0;
        for (unsigned int aIndex = 0; aIndex < mButtons.size(); ++aIndex)
        {
            int aRows = mButtons[aIndex].mRow + 1;
            if (aMaxRows < aRows)
                aMaxRows = aRows;
        }

        aWantedHeight = 46 * aMaxRows + 4;
        if (aWantedHeight < 96)
            aWantedHeight = 96;
    }
    else
    {
        aWantedHeight = 96;
    }

    // Ease a tenth of the way towards the wanted height.
    if (mHeight != aWantedHeight)
        Resize(mX, mY, mWidth, (int)(float)((float)mHeight + (float)((aWantedHeight - mHeight) * 0.1)));
}

// ==========================================================================
// HelpBarWidget::AddedToManager(Sexy::WidgetManager *)
// @00149048  4 bytes
// ==========================================================================
void HelpBarWidget::AddedToManager(Sexy::WidgetManager * theWidgetManager)
{
    // Four bytes in the binary: an unconditional tail call to the base.
    Sexy::WidgetContainer::AddedToManager(theWidgetManager);
}

// ==========================================================================
// HelpBarWidget::RemovedFromManager(Sexy::WidgetManager *)
// @0014904C  4 bytes
// ==========================================================================
void HelpBarWidget::RemovedFromManager(Sexy::WidgetManager * theWidgetManager)
{
    Sexy::WidgetContainer::RemovedFromManager(theWidgetManager);
}

// ==========================================================================
// HelpBarWidget::~HelpBarWidget()
// @00146E90  252 bytes
// ==========================================================================
HelpBarWidget::~HelpBarWidget()
{
  // The vptr store and the trailing `Sexy::Widget::~Widget(this)` are the
  // compiler's and the base's; neither is written here.
  //
  // The two walks are the two button vectors, and both are the same operation:
  // each element's ButtonDef holds a Sexy::String label at its +0x04, and the
  // dump drops that rep's refcount (freeing it at zero) for every element
  // before freeing the vector's own block. The first walk steps 20 bytes
  // (mButtons, ButtonDef's stride) and the second steps 20 as well over
  // mButtons2; the `operator delete` after each is the vector's _M_start.
  //
  // Both members are declared `libstdcxx::vector<ButtonDef>`, whose own
  // destructor runs Clear() -- destroying each element -- and then deallocates
  // the block, and ButtonDef::mLabel is a Sexy::String whose destructor is
  // exactly that release. So the whole body is redundant here and the
  // destructor is empty.
}

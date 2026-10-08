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
#include "NativeDisplay.h"
#include "ButtonGlyphs.h"
#include "ImageFont.h"
#include "game/Board.h"
#include "GamepadControls.h"
#include "HouseChooserDialog.h"
#include "HelpTextScreen.h"
#include "SeedChooserScreen.h"
#include "VSSetupMenu.h"
#include <string>
#include <vector>
#include <map>
#include <set>

Sexy::TPoint<float> HelpBarWidget::DEFAULT_POS(-240.0f, 512.0f);
const Sexy::TPoint<float> HelpBarWidget::OFFSCREEN_POS(-240.0f, 816.0f);

namespace {

Sexy::String HelpButtonTag(Sexy::GamepadButton theButton)
{
    const char* tag = "";
    switch (theButton)
    {
    case GAMEPAD_BUTTON_UP:
    case GAMEPAD_BUTTON_DOWN:
    case GAMEPAD_BUTTON_LEFT:
    case GAMEPAD_BUTTON_RIGHT:
    case GAMEPAD_BUTTON_DPAD_UP:
    case GAMEPAD_BUTTON_DPAD_DOWN:
    case GAMEPAD_BUTTON_DPAD_LEFT:
    case GAMEPAD_BUTTON_DPAD_RIGHT: tag = "<D>"; break;
    case GAMEPAD_BUTTON_SELECT: tag = "<b>"; break;
    case GAMEPAD_BUTTON_START: tag = "<S>"; break;
    case GAMEPAD_BUTTON_A: tag = "<A>"; break;
    case GAMEPAD_BUTTON_B: tag = "<B>"; break;
    case GAMEPAD_BUTTON_X: tag = "<X>"; break;
    case GAMEPAD_BUTTON_Y: tag = "<Y>"; break;
    case GAMEPAD_BUTTON_TL: tag = "<L>"; break;
    case GAMEPAD_BUTTON_TR: tag = "<R>"; break;
    case GAMEPAD_BUTTON_TL2: tag = "<LT>"; break;
    case GAMEPAD_BUTTON_TR2: tag = "<RT>"; break;
    case GAMEPAD_BUTTON_THUMBL: tag = "<l>"; break;
    case GAMEPAD_BUTTON_THUMBR: tag = "<r>"; break;
    default: break;
    }
    // The help bar draws gamepad controls. OEM remote captions replace <X>
    // and <l> with words, losing the button identity before glyph lookup.
    return Sexy::String(tag);
}

} // namespace

Sexy::GamepadButton HelpBarWidget::HitTestButton(int x, int y) const
{
    if (mFontId == nullptr || IMAGE_HELP_BUTTONS == nullptr || !mVisible || mAlpha < 0.4f)
        return GAMEPAD_BUTTON_NONE;
    const bool wideScreen = gLawnApp->mPlatformDriverOrQueue->mWideScreen;
    const int glyphWidth = IMAGE_HELP_BUTTONS->GetCelWidth();
    const int spaceWidth = mFontId->StringWidth(Sexy::String(" "));
    const int slide = TodAnimateCurve(0, 10, mAnimCounter, 0, 20, CURVE_EASE_IN_OUT);
    int nextRow[2] = {0, 0};
    for (const ButtonDef& button : mButtons)
    {
        if (button.mButton == NUM_GAMEPAD_BUTTONS)
            continue;
        const bool right = button.mAlign == HELP_ALIGN_RIGHT;
        const int column = right ? 1 : 0;
        const int row = button.mRow < 0 ? nextRow[column] : button.mRow;
        ++nextRow[column];
        int glyphX = right ? (wideScreen ? 1156 : 1036) - glyphWidth : (wideScreen ? 124 : 244);
        if (button.mAnimating && mAlpha < 1.0f)
        {
            const float offscreenX = float(right ? LawnApp::FULLSCREEN_RECT.mWidth + 256
                                                : LawnApp::FULLSCREEN_RECT.mX - 256);
            glyphX = int(TodAnimateCurveFloatTime(0.4f, 1.0f, mAlpha, offscreenX, float(glyphX), CURVE_EASE_IN));
        }
        const int labelWidth = mFontId->StringWidth(button.mLabel);
        const int left = right ? glyphX - spaceWidth - labelWidth : glyphX;
        // Same columns, rows and slide as DrawButtons @0x1470BC / Draw @0x148608.
        if (Sexy::Rect(left, 42 * row + slide, glyphWidth + spaceWidth + labelWidth, 42).Contains(x, y))
            return button.mButton;
    }
    return GAMEPAD_BUTTON_NONE;
}

bool HelpBarWidget::IsPointVisible(int x, int y)
{
    return HitTestButton(x, y) != GAMEPAD_BUTTON_NONE;
}

void HelpBarWidget::MouseDown(int x, int y, int theClickCount)
{
    mMousePressedButton = theClickCount > 0 && theClickCount != 3
        ? HitTestButton(x, y) : GAMEPAD_BUTTON_NONE;
}

void HelpBarWidget::MouseUp(int x, int y, int theClickCount)
{
    const Sexy::GamepadButton button = mMousePressedButton;
    mMousePressedButton = GAMEPAD_BUTTON_NONE;
    if (button == GAMEPAD_BUTTON_NONE || theClickCount <= 0 || theClickCount == 3
        || HitTestButton(x, y) != button || mWidgetManager == nullptr)
        return;
    Sexy::Widget* target = mWidgetManager->mFocusWidget;
    if (target == nullptr || target == this)
        return;
    if (Board* board = dynamic_cast<Board*>(target))
    {
        if (button == GAMEPAD_BUTTON_Y && board->mApp->CanShopLevel())
        {
            board->ToggleToolBar();
            return;
        }
        if (button == GAMEPAD_BUTTON_X && board->mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN)
        {
            board->mApp->mZenGarden->OpenStore();
            return;
        }
        if (button == GAMEPAD_BUTTON_X && board->mApp->mGameMode == GAMEMODE_TREE_OF_WISDOM)
        {
            board->mChallenge->TreeOfWisdomOpenStore();
            return;
        }
        if (button == GAMEPAD_BUTTON_START || button == GAMEPAD_BUTTON_X)
        {
            if (board->mApp->mGameMode == GAMEMODE_CHALLENGE_LAST_STAND && button == GAMEPAD_BUTTON_X)
            {
                board->mChallenge->OnKeyDown(Sexy::KEYCODE_GAMEPAD_X, 0);
                board->mZombieCountDown = 9;
                board->mZombieCountDownStart = 9;
                board->RebuildHelpBar();
            }
            else if (board->OkToPause())
                board->mApp->DoNewOptions(false, board->mGamepadControls[0]->mGamepadIndex);
            return;
        }
    }
    if (HouseChooserDialog* chooser = dynamic_cast<HouseChooserDialog*>(target))
    {
        chooser->GameButtonDown(button, 0, 0);
        return;
    }
    if (VSSetupMenu* menu = dynamic_cast<VSSetupMenu*>(target))
    {
        menu->GameButtonDown(button, menu->mControllerIndex[0], 0);
        return;
    }
    if (SeedChooserScreen* chooser = dynamic_cast<SeedChooserScreen*>(target))
    {
        if (button == GAMEPAD_BUTTON_X)
            chooser->ButtonDepress(SeedChooserScreen::SeedChooserScreen_Store);
        else if (button == GAMEPAD_BUTTON_Y)
            chooser->ButtonDepress(SeedChooserScreen::SeedChooserScreen_Almanac);
        else if (button == GAMEPAD_BUTTON_SELECT)
            chooser->ButtonDepress(SeedChooserScreen::SeedChooserScreen_ViewLawn);
        else if (button == GAMEPAD_BUTTON_A)
            chooser->OnStartButton();
        else
            chooser->GameButtonDown(button, chooser->mPlayerIndex, 0);
        return;
    }
    if (HelpTextScreen* help = dynamic_cast<HelpTextScreen*>(target))
    {
        if (button == GAMEPAD_BUTTON_UP)
            help->GameButtonDown(help->mScrollTo > 15.0f ? GAMEPAD_BUTTON_LEFT : GAMEPAD_BUTTON_RIGHT, 0, 0);
        else
            help->GameButtonDown(button, 0, 0);
        return;
    }
    Sexy::KeyCode key = Sexy::KEYCODE_UNKNOWN;
    switch (button)
    {
    case GAMEPAD_BUTTON_A: key = Sexy::KEYCODE_RETURN; break;
    case GAMEPAD_BUTTON_B:
    case GAMEPAD_BUTTON_SELECT: key = Sexy::KEYCODE_ESCAPE; break;
    case GAMEPAD_BUTTON_X: key = Sexy::KEYCODE_GAMEPAD_X; break;
    case GAMEPAD_BUTTON_Y: key = Sexy::KEYCODE_GAMEPAD_Y; break;
    case GAMEPAD_BUTTON_START: key = Sexy::KEYCODE_GAMEPAD_START; break;
    case GAMEPAD_BUTTON_LEFT:
    case GAMEPAD_BUTTON_DPAD_LEFT: key = Sexy::KEYCODE_LEFT; break;
    case GAMEPAD_BUTTON_RIGHT:
    case GAMEPAD_BUTTON_DPAD_RIGHT: key = Sexy::KEYCODE_RIGHT; break;
    default: return;
    }
    target->KeyDown(key);
}

// @0x1470BC: two columns, 42-pixel rows, with sliding/fading page hints.
void HelpBarWidget::DrawButtons(Sexy::Graphics* theGraphics, std::vector<ButtonDef>& theButtons,
                                bool theIncoming, float theAlpha)
{
    if (mFontId == nullptr || IMAGE_HELP_BUTTONS == nullptr)
        return;
    int nextRow[2] = {0, 0};
    bool wideScreen = gLawnApp->mPlatformDriverOrQueue->mWideScreen;
    int spaceWidth = mFontId->StringWidth(Sexy::String(" "));
    for (ButtonDef& button : theButtons)
    {
        Sexy::String buttonTag = HelpButtonTag(button.mButton);
        int glyphCel = ButtonGlyphCel(buttonTag.c_str(), static_cast<int>(buttonTag.size()));
        int captionWidth = IMAGE_HELP_BUTTONS->GetCelWidth();
        int buttonX = button.mAlign == HELP_ALIGN_LEFT ? (wideScreen ? 124 : 244)
                                                      : (wideScreen ? 1156 : 1036) - captionWidth;
        Sexy::Color color = Sexy::Color::White;
        if (button.mAnimating)
        {
            color.mAlpha = static_cast<int>(theAlpha * 255.0f);
            float offscreenX = static_cast<float>(button.mAlign == HELP_ALIGN_LEFT
                ? LawnApp::FULLSCREEN_RECT.mX - 256 : LawnApp::FULLSCREEN_RECT.mWidth + 256);
            if (theIncoming && theAlpha < 1.0f)
                buttonX = static_cast<int>(TodAnimateCurveFloatTime(0.4f, 1.0f, theAlpha, offscreenX,
                                            static_cast<float>(buttonX), CURVE_EASE_IN));
            else if (!theIncoming && theAlpha > 0.0f)
                buttonX = static_cast<int>(TodAnimateCurveFloatTime(1.0f, 0.6f, theAlpha,
                                            static_cast<float>(buttonX), offscreenX, CURVE_EASE_IN));
            else
                button.mAnimating = false;
        }
        int column = button.mAlign == HELP_ALIGN_RIGHT ? 1 : 0;
        if (button.mRow == -1)
            button.mRow = nextRow[column];
        ++nextRow[column];
        int baseline = 42 * button.mRow + 32;
        if (glyphCel >= 0)
        {
            Sexy::Graphics glyphGraphics(*theGraphics);
            glyphGraphics.SetColor(color);
            glyphGraphics.SetColorizeImages(true);
            DrawButtonGlyph(&glyphGraphics, buttonX, baseline, mFontId, glyphCel, captionWidth);
        }
        else
            TodDrawString(theGraphics, buttonTag, buttonX, baseline, mFontId, color, 3);
        int textX = button.mAlign == HELP_ALIGN_RIGHT ? buttonX - spaceWidth
                                                     : buttonX + captionWidth + spaceWidth;
        if (mOutlineFont != nullptr)
            TodDrawString(theGraphics, button.mLabel, textX, baseline, mOutlineFont.get(), color,
                          button.mAlign == HELP_ALIGN_RIGHT ? 4 : 3);
        TodDrawString(theGraphics, button.mLabel, textX, baseline, mFontId, color,
                      button.mAlign == HELP_ALIGN_RIGHT ? 4 : 3);
    }
}

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
        {
            mFontId = FONT_HOUSEOFTERROR20;
            // The Chinese 20-point descriptor keeps its black outline in a separate atlas.
            mOutlineFont = std::make_unique<Sexy::ImageFont>(gLawnApp, "data/HouseofTerror20Outline.txt");
        }
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
    Sexy::GamepadApp* anApp = gLawnApp;
    if (anApp->HasGamepad() != 0 || (anApp->mGamePad1IsOn && anApp->mGamePad2IsOn)
        || mMouseVisible)
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
        DrawButtons(theGraphics, mButtons2, false, 1.0f - mAlpha);
        DrawButtons(theGraphics, mButtons, true, mAlpha);
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
    // Native button discs extend past the bar's 96-pixel layout after its 20-pixel slide.
    // Keep the parent's clip so the complete discs remain visible during the animation.
    mClip = false;
    mMousePreservesFocus = true;

    // The bar sits at the bottom of the screen; a second construction reuses the
    // height the first one settled on.
    int aY;
    if (gLawnApp->mPlatformDriverOrQueue->mWideScreen)
    {
        aY = static_cast<int>(DEFAULT_POS.mY);
    }
    else
    {
        aY = 504;
        DEFAULT_POS.mY = 504.0f;
    }

    Resize(static_cast<int>(DEFAULT_POS.mX), aY, LawnApp::FULLSCREEN_RECT.mWidth, 96);
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
    aDef.mAlign = theHelpAlign;
    aDef.mAnimating = true;
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

    aDef.mAlign = theHelpAlign;
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
        mButtons[aIndex].mAnimating = true;
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
}

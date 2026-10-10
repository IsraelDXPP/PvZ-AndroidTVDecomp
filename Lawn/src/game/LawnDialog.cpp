// LawnDialog -- reconstructed from libGameMain.so.
//
// Every raw this-relative offset, vptr store and hand-written vtable slot call
// the transcription carried is gone: each member access below is the named
// member the header recovered and each virtual dispatch is an ordinary call.
//
// Three transcription bugs specific to this file were found and fixed against
// the ARM:
//   * AddedToManager/RemovedFromManager read `(char *)this + 24` and `+ 28`.
//     Those are vtable BYTE offsets, not fields: slot 6 is
//     WidgetContainer::AddWidget and slot 7 is RemoveWidget.
//   * Update's tail dispatch is vtable byte 100, slot 25, i.e.
//     WidgetContainer::MarkDirty.
//   * Resize's last two button calls were written with three arguments; the ARM
//     stores the fifth argument (`STR R5,[SP,#0xC+var_C]`) before each BLX at
//     0x13f482 and 0x13f504, so all three calls take four.
//   * the constructor's `(char *)this + 256` is the Widget's ButtonListener
//     sub-object, which is what MakeButton's listener parameter wants.
//
// Untyped globals (`IMAGE_*`, `FONT_*`, `SOUND_*`) are `_DWORD` placeholders in
// Sexy\ReconFreeFunctions.h; they hold pointers and are cast at the use site,
// which is the established form in SeedChooserScreen.cpp and Dialog.cpp.

#include "LawnDialog.h"
#include "ReconData.h"
#include "Dialog.h"
#include "Font.h"
#include "GameButton.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "ReanimationWidget.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetManager.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ==========================================================================
// LawnDialog::SetButtonDelay(int)
// @0013E340  42 bytes
// ==========================================================================
void LawnDialog::SetButtonDelay(int theDelay)
{
    // A new delay restarts the countdown, so both buttons go back to disabled
    // until Update sees mUpdateCnt catch up with it. Vtable byte 192 is Widget
    // slot [48], SetDisabled(bool).
    mButtonDelay = theDelay;
    if (mLawnYesButton != nullptr)
        mLawnYesButton->SetDisabled(true);
    if (mLawnNoButton != nullptr)
        mLawnNoButton->SetDisabled(true);
}

// ==========================================================================
// LawnDialog::ButtonPress(int)
// @0013E36C  28 bytes
// ==========================================================================
void LawnDialog::ButtonPress(int theId)
{
    // Vtable byte 676 of the app is LawnApp slot [169], PlaySample(int, bool);
    // the two-argument overload hides SexyAppBase::PlaySample(int).
    mApp->PlaySample(SOUND_GRAVEBUTTON, true);
}

// ==========================================================================
// LawnDialog::CheckboxChecked(int,bool)
// @0013E398  28 bytes
// ==========================================================================
void LawnDialog::CheckboxChecked(int theId, bool checked)
{
    mApp->PlaySample(SOUND_BUTTONCLICK, true);
}

// ==========================================================================
// LawnDialog::~LawnDialog()
// @0013E3F4  72 bytes
// ==========================================================================
LawnDialog::~LawnDialog()
{
    // The three vtable byte 4 dispatches are the deleting destructors `delete`
    // emits; the two vptr stores and the explicit base call are the compiler's
    // job once the bases are real.
    delete mReanimationWidget;
    delete mLawnYesButton;
    delete mLawnNoButton;
}

// ==========================================================================
// LawnDialog::ButtonDepress(int)
// @0013E980  16 bytes
// ==========================================================================
void LawnDialog::ButtonDepress(int theId)
{
    // Before the delay has elapsed the press is swallowed; afterwards it reaches
    // the base, which records the result and tells the listener.
    if (mUpdateCnt > mButtonDelay)
        Sexy::Dialog::ButtonDepress(theId);
}

// ==========================================================================
// LawnDialog::GotFocus()
// @0013E9A8  46 bytes
// ==========================================================================
void LawnDialog::GotFocus()
{
    Sexy::Widget::GotFocus();

    // With no child focused yet the dialog hands focus to its yes button, then to
    // the no button. Vtable byte 228 is Widget slot [57], SetFocus(Widget *, bool).
    if (mFocusedChildWidget == nullptr)
    {
        if (mLawnYesButton != nullptr)
        {
            SetFocus(mLawnYesButton, true);
            return;
        }
        if (mLawnNoButton != nullptr)
            SetFocus(mLawnNoButton, true);
    }
}

// ==========================================================================
// LawnDialog::KeyDown(Sexy::KeyCode)
// @0013EB3C  66 bytes
// ==========================================================================
int LawnDialog::KeyDown(Sexy::KeyCode theKey)
{
    // The framework delivers the key through the top-level widget's event, whose
    // modifierMask (+4) carries the flags. Bit 7 means the key came from a gamepad,
    // and escape is left to the gamepad handler in that case.
    Sexy::Widget *aTopLevel = GetTopLevel();
    int aModifiers = 0;
    if (aTopLevel != nullptr && aTopLevel->mEvent != nullptr)
        aModifiers = aTopLevel->mEvent->modifierMask;

    if (theKey != Sexy::KEYCODE_ESCAPE)
        return Sexy::Dialog::KeyDown(theKey);
    if ((aModifiers & 0x80) != 0)
        return 0;
    if (mLawnNoButton == nullptr)
        return 0;

    // Vtable byte 516 is LawnDialog slot [129], ButtonDepress(int); the argument
    // is the button's own id at WidgetContainer::mWidgetId (+0x70).
    ButtonDepress(mLawnNoButton->mWidgetId);
    return 1;
}

// ==========================================================================
// LawnDialog::Draw(Sexy::Graphics *)
// @0013EF00  880 bytes
// ==========================================================================
void LawnDialog::Draw(Sexy::Graphics *g)
{
    if (mDrawFade)
    {
        mApp->DrawDialogFade(g);
        if (mDrawQRCode)
            mApp->DrawQRCode(g);
    }

    if (!mDrawStandardBack)
        return;

    Sexy::Image *aTopLeft = IMAGE_DIALOG_TOPLEFT;
    Sexy::Image *aTopMiddle = IMAGE_DIALOG_TOPMIDDLE;
    Sexy::Image *aTopRight = IMAGE_DIALOG_TOPRIGHT;
    Sexy::Image *aCenterLeft = IMAGE_DIALOG_CENTERLEFT;
    Sexy::Image *aCenterMiddle = IMAGE_DIALOG_CENTERMIDDLE;
    Sexy::Image *aCenterRight = IMAGE_DIALOG_CENTERRIGHT;
    Sexy::Image *aHeader = IMAGE_DIALOG_HEADER;
    Sexy::Image *aBottomLeft = IMAGE_DIALOG_BOTTOMLEFT;
    Sexy::Image *aBottomMiddle = IMAGE_DIALOG_BOTTOMMIDDLE;
    Sexy::Image *aBottomRight = IMAGE_DIALOG_BOTTOMRIGHT;
    if (mTallBottom)
    {
        aBottomLeft = IMAGE_DIALOG_BIGBOTTOMLEFT;
        aBottomMiddle = IMAGE_DIALOG_BIGBOTTOMMIDDLE;
        aBottomRight = IMAGE_DIALOG_BIGBOTTOMRIGHT;
    }

    // The frame is nine slices: the corners are drawn once, the edges are tiled
    // to the nearest whole piece, and the trims are cut 45 pixels down.
    int aStretchX = (mWidth - aTopLeft->mWidth - aTopRight->mWidth) / aTopMiddle->mWidth;
    int aStretchY = (mHeight - aTopLeft->mHeight - aBottomLeft->mHeight - 45) / aCenterLeft->mHeight;

    g->DrawImage(aTopLeft, 0, 45);
    int aX = aTopLeft->mWidth;
    for (int i = 0; i < aStretchX; ++i)
    {
        g->DrawImage(aTopMiddle, aX, 45);
        aX += aTopMiddle->mWidth;
    }
    g->DrawImage(aTopRight, aX, 45);

    int aY = aTopRight->mHeight + 45;
    for (int j = 0; j < aStretchY; ++j)
    {
        g->DrawImage(aCenterLeft, 0, aY);
        int aCenterX = aCenterLeft->mWidth;
        for (int i = 0; i < aStretchX; ++i)
        {
            g->DrawImage(aCenterMiddle, aCenterX, aY);
            aCenterX += aCenterMiddle->mWidth;
        }
        g->DrawImage(aCenterRight, aCenterX, aY);
        aY += aCenterLeft->mHeight;
    }

    g->DrawImage(aBottomLeft, 0, aY);
    int aBottomX = aBottomLeft->mWidth;
    for (int i = 0; i < aStretchX; ++i)
    {
        g->DrawImage(aBottomMiddle, aBottomX, aY);
        aBottomX += aBottomMiddle->mWidth;
    }
    g->DrawImage(aBottomRight, aBottomX, aY);

    g->DrawImage(aHeader, (mWidth - aHeader->mWidth) / 2 - 5, 0);

    // The header sits on the content insets 45 pixels down. Its ascent and ascent
    // padding place its baseline, and the lines start mSpaceAfterHeader below the
    // header's own height.
    int aTextY = mBackgroundInsets.mTop + 45 + mContentInsets.mTop;
    if (mDialogHeader.length() != 0)
    {
        int aHeaderY = aTextY + mHeaderFont->GetAscent() - mHeaderFont->GetAscentPadding();
        g->SetFont(mHeaderFont);
        g->SetColor(mColors[0]);
        // Widget slot [108], WriteCenteredLine(Graphics *, int, const String &);
        // its TRect<int> result is discarded here.
        WriteCenteredLine(g, aHeaderY, mDialogHeader);
        aTextY = aHeaderY + mHeaderFont->GetHeight() - mHeaderFont->GetAscent() + mSpaceAfterHeader;
    }

    g->SetFont(mLinesFont);
    g->SetColor(mColors[1]);

    // The lines are laid out inside the content insets, two pixels in from their
    // left edge and four narrower than the frame.
    Sexy::TRect<int> aTextRect(mBackgroundInsets.mLeft + mContentInsets.mLeft + 2,
        aTextY,
        mWidth - mContentInsets.mLeft - mContentInsets.mRight - mBackgroundInsets.mLeft - mBackgroundInsets.mRight - 4,
        0);
    if (mVerticalCenterText)
    {
        // Widget slot [112], GetWordWrappedHeight(Graphics *, int, const String &,
        // int). The 91 and the 55 are the tall and the short button row, and the
        // block is centred in what they leave.
        int aTextHeight =
            GetWordWrappedHeight(g, aTextRect.mWidth, mDialogLines, mLineSpacingOffset + mLinesFont->GetLineSpacing());
        int aButtonY = mHeight - mBackgroundInsets.mBottom - mContentInsets.mBottom - aTextY - mButtonHeight;
        int aTop = mTallBottom ? aButtonY - 91 : aButtonY - 55;
        aTextRect.mY += (aTop - aTextHeight) / 2;
    }

    // Widget slot [111], WriteWordWrapped(Graphics *, const TRect<int> &,
    // const String &, int, int).
    WriteWordWrapped(g, aTextRect, mDialogLines, mLineSpacingOffset + mLinesFont->GetLineSpacing(), mTextAlign);
}

// ==========================================================================
// LawnDialog::Resize(int,int,int,int)
// @0013F3B8  346 bytes
// ==========================================================================
void LawnDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    Sexy::Dialog::Resize(theX, theY, theWidth, theHeight);

    // The button row is built from two pieces: a two-button row is the left and
    // right caps plus three middles, a one-button row is the caps plus a single
    // middle repeated to the row's width.
    Sexy::Image *aButtonLeft = IMAGE_BUTTON_LEFT;
    Sexy::Image *aButtonMiddle = IMAGE_BUTTON_MIDDLE;
    Sexy::Image *aButtonRight = IMAGE_BUTTON_RIGHT;
    int aButtonMiddleWidth = aButtonMiddle->mWidth;
    int aButtonHeight = aButtonLeft->mHeight;
    int aButtonPairWidth = aButtonLeft->mWidth + aButtonRight->mWidth + 3 * aButtonMiddleWidth;

    int aInnerWidth = mWidth - mContentInsets.mLeft - mBackgroundInsets.mLeft - mContentInsets.mRight;
    int aRowX = mContentInsets.mLeft + mBackgroundInsets.mLeft - 5;
    int aRowWidth = aInnerWidth - mBackgroundInsets.mRight;
    int aRowSpan = aRowWidth + 8;
    int aRowBottom = mHeight + 10 - mContentInsets.mBottom;
    int aPairWidth = MakeMultiple((aRowWidth - 2) / 2 - aButtonPairWidth - aButtonMiddleWidth + 1, aButtonMiddleWidth);

    int aRowY = aRowBottom - mBackgroundInsets.mBottom - aButtonHeight;
    int aButtonY = mTallBottom ? aRowY - 3 : aRowY - 8;
    if (mLawnYesButton != nullptr)
    {
        if (mLawnNoButton != nullptr)
        {
            // Both buttons get the same width and the second starts where the first
            // ends (Widget slot [50], Resize(int,int,int,int)).
            int aButtonWidth = aButtonPairWidth + aPairWidth;
            mLawnYesButton->Resize(aRowX, aButtonY, aButtonWidth, aButtonHeight);
            mLawnNoButton->Resize(aRowX + aRowSpan - aButtonWidth, aButtonY, aButtonWidth, aButtonHeight);
        }
        else
        {
            int aMultiple =
                MakeMultiple(aRowSpan - aButtonPairWidth - aButtonMiddle->mWidth + 1, aButtonMiddle->mWidth);
            mLawnYesButton->Resize(aRowX + (aRowSpan - (aButtonPairWidth + aMultiple)) / 2,
                aButtonY,
                aButtonPairWidth + aMultiple,
                aButtonHeight);
        }
    }

    // The reanimation sits over the yes button, 45 pixels down and centred on it.
    if (mReanimationWidget->mReanim != nullptr)
    {
        mReanimationWidget->Resize(mLawnYesButton->mWidth / 2 - mReanimationWidget->mWidth / 2,
            (int)(mReanimationWidget->mPosY + 45.0f),
            mReanimationWidget->mWidth,
            mReanimationWidget->mHeight);
    }
}

// ==========================================================================
// LawnDialog::GetLeft()
// @0013F528  12 bytes
// ==========================================================================
int LawnDialog::GetLeft()
{
    return mBackgroundInsets.mLeft + mContentInsets.mLeft;
}

// ==========================================================================
// LawnDialog::GetWidth()
// @0013F534  32 bytes
// ==========================================================================
int LawnDialog::GetWidth()
{
    return mWidth - mContentInsets.mLeft - mContentInsets.mRight - mBackgroundInsets.mLeft - mBackgroundInsets.mRight;
}

// ==========================================================================
// LawnDialog::GetTop()
// @0013F554  14 bytes
// ==========================================================================
int LawnDialog::GetTop()
{
    // The frame's top inset plus the 45-pixel header strip plus 54: the content
    // starts 99 pixels below the dialog's own top edge.
    return mBackgroundInsets.mTop + mContentInsets.mTop + 99;
}

// ==========================================================================
// LawnDialog::CalcSize(int,int,bool)
// @0013FAFC  428 bytes
// ==========================================================================
int LawnDialog::CalcSize(int theWidth, int theHeight, bool theSetPos)
{
    // The frame is 45 pixels of header plus the four background/content insets on
    // each axis, and the header adds its own width when there is one.
    int aWidth =
        mBackgroundInsets.mLeft + mContentInsets.mLeft + mContentInsets.mRight + mBackgroundInsets.mRight + theWidth;
    if (!mDialogHeader.empty())
        aWidth += mHeaderFont->StringWidth(mDialogHeader);

    // The width is then rounded up to whole top-middle tiles.
    int aTopMiddleWidth = (IMAGE_DIALOG_TOPMIDDLE)->mWidth;
    int aFrameWidth = (IMAGE_DIALOG_TOPLEFT)->mWidth + (IMAGE_DIALOG_TOPRIGHT)->mWidth + 2 * aTopMiddleWidth;
    if (aWidth >= aFrameWidth)
        aFrameWidth += MakeMultiple(aWidth - aFrameWidth, aTopMiddleWidth);

    int aTotalHeight = mBackgroundInsets.mTop + mContentInsets.mTop + mContentInsets.mBottom +
        mBackgroundInsets.mBottom + 45 + theHeight;
    if (!mDialogHeader.empty())
    {
        // The header block contributes its own height less its ascent, plus the
        // space the dialog keeps after it.
        aTotalHeight += mHeaderFont->GetHeight() - mHeaderFont->GetAscent() + mSpaceAfterHeader;
    }

    if (!mDialogLines.empty())
    {
        if (theSetPos)
            aFrameWidth += aTopMiddleWidth;

        // A scratch Graphics measures how tall the wrapped lines come out.
        Sexy::Graphics aGraphics(nullptr);
        aGraphics.SetFont(mLinesFont);
        int aTextWidth = aFrameWidth - mContentInsets.mLeft - mContentInsets.mRight - mBackgroundInsets.mLeft -
            mBackgroundInsets.mRight - 4;
        aTotalHeight += GetWordWrappedHeight(
                            &aGraphics, aTextWidth, mDialogLines, mLineSpacingOffset + mLinesFont->GetLineSpacing()) +
            30;
    }

    // The bottom is the button row plus the frame's bottom trim, rounded up to
    // whole center-left tiles.
    int aContentHeight = mButtonHeight + aTotalHeight + 8;
    int aBottomHeight = (IMAGE_DIALOG_BOTTOMLEFT)->mHeight;
    if (mTallBottom)
        aBottomHeight = (IMAGE_DIALOG_BIGBOTTOMLEFT)->mHeight;
    int aFrameHeight = (IMAGE_DIALOG_TOPLEFT)->mHeight + 45 + aBottomHeight;
    if (aFrameHeight <= aContentHeight)
        aFrameHeight += MakeMultiple(aContentHeight - aFrameHeight, (IMAGE_DIALOG_CENTERLEFT)->mHeight);

    Resize(mX, mY, aFrameWidth, aFrameHeight);
    return 0;
}

// ==========================================================================
// LawnDialog::LawnDialog(LawnApp *,Sexy::Image *,int,bool,Sexy::String const&,Sexy::String const&,Sexy::String const&,int)
// @0013FCD0  1004 bytes
// ==========================================================================
LawnDialog::LawnDialog(LawnApp *theApp, Sexy::Image *theImage, int theId, bool isModal,
    Sexy::String const &theDialogHeader, Sexy::String const &theDialogLines, Sexy::String const &theDialogFooter,
    int theButtonMode)
    // The base's own construction. The dump's placement-new of Sexy::Dialog
    // over `this` is what this initialiser list does; the three StringCtor
    // temporaries it was handed (StringCtor) and the release blocks that
    // follow are the initialiser expressions and the compiler's cleanup.
    :
    Sexy::Dialog(nullptr, theImage, theId, isModal, theDialogHeader, theDialogLines, theDialogFooter, 0)
{
    mButtonDelay = -1;
    mApp = theApp;

    mReanimationWidget = new ReanimationWidget();
    // The overlay points back at its owner through its unnamed +0x108 word, and
    // starts with Widget's +0x9C flag clear.
    mReanimationWidget->mLawnDialog = this;
    mReanimationWidget->mFocusable = false;

    mTallBottom = false;
    mDrawQRCode = false;
    mClip = false;
    mDrawStandardBack = true;
    mVerticalCenterText = true;
    mDrawFade = true;

    // Both label strings are looked up before they are stored.
    Sexy::String aHeader;
    TodStringTranslate(&aHeader, theDialogHeader);
    mDialogHeader = aHeader;

    Sexy::String aLines;
    TodStringTranslate(&aLines, theDialogLines);
    mDialogLines = aLines;

    // Widget slot [43] is SetColors(int (*)[3], int): five entries of 255.
    SetColors(gLawnDialogColors, 7);
    // Dialog slots [123] and [124] set the two fonts.
    SetHeaderFont(FONT_DWARVENTODCRAFT24);
    SetLinesFont(FONT_BRIANNETOD16);

    // The four insets, in the order Insets(int,int,int,int) takes them; the ARM
    // stores them straight over mContentInsets (`STM.W R6,{R0-R3}`, R6 = this+0x2AC).
    mContentInsets = Sexy::Insets(36, 35, 46, 36);

    // Both colour entries are the dialog's gold (Widget slot [45], SetColor).
    SetColor(0, Sexy::Color(224, 187, 98));
    SetColor(1, Sexy::Color(224, 187, 98));
    mButtonMode = 3;

    if (theButtonMode == 1)
    {
        Sexy::String aYes;
        TodStringTranslate(&aYes, "[BUTTON_YES]");
        mLawnYesButton = (GameButton *)MakeButton(1000, static_cast<Sexy::ButtonListener *>(this), this, aYes);
        Sexy::String aNo;
        TodStringTranslate(&aNo, "[BUTTON_NO]");
        mLawnNoButton = (GameButton *)MakeButton(1001, static_cast<Sexy::ButtonListener *>(this), this, aNo);
    }
    else if (theButtonMode == 2)
    {
        Sexy::String aOk;
        TodStringTranslate(&aOk, "[BUTTON_OK]");
        mLawnYesButton = (GameButton *)MakeButton(1000, static_cast<Sexy::ButtonListener *>(this), this, aOk);
        Sexy::String aCancel;
        TodStringTranslate(&aCancel, "[BUTTON_CANCEL]");
        mLawnNoButton = (GameButton *)MakeButton(1001, static_cast<Sexy::ButtonListener *>(this), this, aCancel);
    }
    else
    {
        // Button mode 3 puts the caller's footer on the single button; every other
        // mode has no buttons at all.
        if (theButtonMode == 3)
            mLawnYesButton =
                (GameButton *)MakeButton(1000, static_cast<Sexy::ButtonListener *>(this), this, theDialogFooter);
        else
            mLawnYesButton = nullptr;
        mLawnNoButton = nullptr;
    }

    mApp->SetCursor(0);
    CalcSize(0, 0, true);
}

// ==========================================================================
// LawnDialog::AddedToManager(Sexy::WidgetManager *)
// @001410FC  60 bytes
// ==========================================================================
void LawnDialog::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    Sexy::Dialog::AddedToManager(theWidgetManager);
    // The manager only owns widgets that were added to it: the reanimation
    // overlay and then each button that exists (WidgetContainer slot [6]).
    AddWidget(mReanimationWidget);
    if (mLawnYesButton != nullptr)
        AddWidget(mLawnYesButton);
    if (mLawnNoButton != nullptr)
        AddWidget(mLawnNoButton);
    // A dialog on screen means the help bar has to go away again.
    mApp->ForceHideHelpBarWidget();
}

// ==========================================================================
// LawnDialog::Update()
// @00141188  64 bytes
// ==========================================================================
void LawnDialog::Update()
{
    Sexy::Dialog::Update();
    // The delay elapses when the update count catches up with it, and both
    // buttons come back to life together (Widget slot [48], SetDisabled).
    if (mUpdateCnt == mButtonDelay)
    {
        if (mLawnYesButton != nullptr)
            mLawnYesButton->SetDisabled(false);
        if (mLawnNoButton != nullptr)
            mLawnNoButton->SetDisabled(false);
    }
    // Slot [25] is WidgetContainer::MarkDirty, which the dialog runs last so the
    // frame is repainted after the buttons have been re-enabled.
    MarkDirty();
}

// ==========================================================================
// LawnDialog::RemovedFromManager(Sexy::WidgetManager *)
// @00141254  60 bytes
// ==========================================================================
void LawnDialog::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    Sexy::Dialog::RemovedFromManager(theWidgetManager);
    // The mirror of AddedToManager: the buttons leave the manager first and the
    // reanimation overlay last (WidgetContainer slot [7]).
    if (mLawnYesButton != nullptr)
        RemoveWidget(mLawnYesButton);
    if (mLawnNoButton != nullptr)
        RemoveWidget(mLawnNoButton);
    RemoveWidget(mReanimationWidget);
    // The overlay is owned by this dialog, not by the manager, so it is destroyed
    // here rather than left behind.
    mReanimationWidget->Dispose();
}

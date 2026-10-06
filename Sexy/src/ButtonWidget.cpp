// ButtonWidget -- reconstructed from libGameMain.so.
//
// Every raw this-relative offset, vptr store and hand-written vtable slot call
// the transcription carried is gone: each member access below is the named
// member the header recovered and each virtual dispatch is an ordinary call.

#include "ButtonWidget.h"

#include "ButtonListener.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "NativeDisplay.h"
#include "SexyAppBase.h"
#include "SelectableWidget.h"
#include "TextLayout.h"
#include "Widget.h"
#include "WidgetManager.h"
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy {

// Sexy::NativeDisplay is declared in Sexy\NativeDisplay.h, which SexyAppBase.h
// includes; this file only calls through the pointer SexyAppBase keeps at
// +0x2E4, so the class is not redeclared here.
} // namespace Sexy

// ==========================================================================
// Sexy::ButtonWidget::~ButtonWidget()
// @002F8EE0  208 bytes
// ==========================================================================
Sexy::ButtonWidget::~ButtonWidget()
{
  // The vptr store, the two String releases, the embedded TextLayout's
  // destructor and the base destructor are all the compiler's. Only the font
  // is released by hand, and the dump reaches it through the font's vtable
  // byte 4 -- the deleting destructor, which is what `delete` dispatches.
  if ( mFont != NULL )
    delete mFont;
}

// ==========================================================================
// Sexy::ButtonWidget::MouseDown(int,int,int)
// @001996E4  4 bytes
// ==========================================================================
void Sexy::ButtonWidget::MouseDown(int x, int y, int theClickCount)
{
  Widget::MouseDown(x, y, theClickCount);
}

// ==========================================================================
// Sexy::ButtonWidget::MouseUp(int,int)
// @001996E8  4 bytes
// ==========================================================================
void Sexy::ButtonWidget::MouseUp(int x, int y)
{
  Widget::MouseUp(x, y);
}

// ==========================================================================
// Sexy::ButtonWidget::MouseUp(int,int,int)
// @001996EC  4 bytes
// ==========================================================================
void Sexy::ButtonWidget::MouseUp(int x, int y, int theClickCount)
{
  Widget::MouseUp(x, y, theClickCount);
}

// ==========================================================================
// Sexy::ButtonWidget::LockState(int)
// @002F8D0C  12 bytes
// ==========================================================================
void Sexy::ButtonWidget::LockState(int theState)
{
  mLockedState = theState;
  mStateLocked = true;
}

// ==========================================================================
// Sexy::ButtonWidget::UnlockState()
// @002F8D18  8 bytes
// ==========================================================================
void Sexy::ButtonWidget::UnlockState()
{
  mStateLocked = false;
}

// ==========================================================================
// Sexy::ButtonWidget::IsLockedToState(int)const
// @002F8D20  22 bytes
// ==========================================================================
bool Sexy::ButtonWidget::IsLockedToState(int theState) const
{
  // The dump ANDs the two together and then tests for equality with the state
  // it was asked about, which is true only when every bit asked for is pinned.
  if ( !mStateLocked )
    return false;
  return theState == (mLockedState & theState);
}

// ==========================================================================
// Sexy::ButtonWidget::IsLockedToDownState() const
// @002F8D38  14 bytes
// ==========================================================================
bool Sexy::ButtonWidget::IsLockedToDownState() const
{
  // BUTTON_DOWN
  return IsLockedToState(2);
}

// ==========================================================================
// Sexy::ButtonWidget::TouchEnter()
// @002F8D48  12 bytes
// ==========================================================================
void Sexy::ButtonWidget::TouchEnter()
{
  // The dump dispatches the vtable's byte 292 -- slot [73], Widget::MouseEnter
  // -- so a touch arriving on the button is handled as the mouse entering it.
  MouseEnter();
}

// ==========================================================================
// Sexy::ButtonWidget::TouchLeave()
// @002F8D54  12 bytes
// ==========================================================================
void Sexy::ButtonWidget::TouchLeave()
{
  // Vtable byte 296, slot [74]: Widget::MouseLeave.
  MouseLeave();
}

// ==========================================================================
// Sexy::ButtonWidget::TouchDown(int,int,int)
// @002F8D60  28 bytes
// ==========================================================================
void Sexy::ButtonWidget::TouchDown(int theId, int x, int y)
{
  // ButtonPress(mId, 1): the dump dispatches the listener's vtable byte 12,
  // slot [3], with the widget id and a click count of one. It does not test the
  // listener for null, and neither does this.
  mListener->ButtonPress(mWidgetId, 1);
  Widget::TouchDown(theId, x, y);
}

// ==========================================================================
// Sexy::ButtonWidget::TouchUp(int,int,int)
// @002F8D7C  42 bytes
// ==========================================================================
void Sexy::ButtonWidget::TouchUp(int theId, int x, int y)
{
  // The dump only reports the release while the manager holds the focus; the
  // byte it reads there is WidgetManager::mHasFocus (+0x9C).
  if ( mIsDown && mWidgetManager->mHasFocus )
    mListener->ButtonDepress(mWidgetId);

  Widget::TouchUp(theId, x, y);
}

// ==========================================================================
// Sexy::ButtonWidget::SetFont(Sexy::Font *)
// @002F8DA8  44 bytes
// ==========================================================================
void Sexy::ButtonWidget::SetFont(Sexy::Font * theFont)
{
  // Vtable byte 4 of the outgoing font is its deleting destructor, i.e. the
  // dump is `delete mFont`. The incoming font is stored through its own
  // Duplicate (vtable byte 84, slot [21]) rather than directly.
  if ( mFont != NULL )
    delete mFont;

  mFont = theFont->Duplicate();
  mTextLayout.SetFont(mFont);
}

// ==========================================================================
// Sexy::ButtonWidget::SetLabel(Sexy::String const&)
// @002F8DD4  8 bytes
// ==========================================================================
void Sexy::ButtonWidget::SetLabel(const Sexy::String & theLabel)
{
  // The dump's AssignStringThunk is StringAssign, the copy-on-write assign.
  mLabel = theLabel;
}

// ==========================================================================
// Sexy::ButtonWidget::UpdateLabel()
// @002F8DDC  136 bytes
// ==========================================================================
void Sexy::ButtonWidget::UpdateLabel()
{
  // mCachedLabel caches the label the layout was last built from: when it still
  // matches mLabel byte for byte and the layout already holds mFont, there is
  // nothing to redo. The dump compares the two strings' stored lengths and then
  // memcmp's their characters.
  if ( mCachedLabel.size() == mLabel.size()
    && memcmp(mCachedLabel.data(), mLabel.data(), mLabel.size()) == 0 )
  {
    if ( mFont == mTextLayout.GetFont() )
      return;
  }

  mCachedLabel = mLabel;
  mTextLayout.SetFont(mFont);
  mTextLayout.SetText(mLabel, false);

  if ( mFont != NULL )
  {
    // StringExtents still fills six words through an int reference. Those words
    // are the label's float extents.
    int measured[6] = {};
    mFont->StringExtents(mLabel, measured[0]);
    std::memcpy(&mLabelExtents, measured, sizeof(mLabelExtents));
  }
  else
  {
    mLabelExtents = LabelExtents();
  }
}

// ==========================================================================
// Sexy::ButtonWidget::IsButtonDown()
// @002F8E64  46 bytes
// ==========================================================================
bool Sexy::ButtonWidget::IsButtonDown()
{
  // Vtable byte 528 is this class's own slot [132], IsLockedToDownState: a
  // locked-down button counts as pressed whatever the mouse is doing.
  if ( IsLockedToDownState() )
    return true;

  if ( mIsDown && mIsOver )
    return !mDisabled;

  // mReturnPressed is the return-key press KeyDown/KeyUp maintain.
  if ( mReturnPressed )
    return !mDisabled;

  return false;
}

// ==========================================================================
// Sexy::ButtonWidget::DrawButtonImage(Sexy::Graphics *,Sexy::Image *,Sexy::TRect<int> const&,int,int)
// @002F8E94  76 bytes
// ==========================================================================
void Sexy::ButtonWidget::DrawButtonImage(Sexy::Graphics * g, Sexy::Image * theImage,
                                         const TRect<int> & theRect, int theX, int theY)
{
  if ( theRect.mWidth != 0 )
  {
    if ( theImage == NULL )
      theImage = mButtonImage;

    // The dump builds a source rect of the widget's own size at (theX, theY)
    // and draws it into theRect, which is the 3-argument DrawImage overload.
    g->DrawImage(theImage, TRect<int>(theX, theY, mWidth, mHeight), theRect);
  }
  else
  {
    // No frame rect: the whole image, at (theX, theY).
    g->DrawImage(theImage, theX, theY, TRect<int>(0, 0, theImage->mWidth, theImage->mHeight));
  }
}

// ==========================================================================
// Sexy::ButtonWidget::ButtonWidget(int,Sexy::ButtonListener *)
// @002F8FC4  286 bytes
// ==========================================================================
Sexy::ButtonWidget::ButtonWidget(int theId, Sexy::ButtonListener * theButtonListener)
{
  // The base constructor, the embedded TextLayout's and the Insets' are the
  // compiler's; so is the vptr store the dump spells out.
  mLabel = String();
  mCachedLabel = String();

  mLabelExtents = LabelExtents();

  mFont = NULL;
  mButtonImage = NULL;
  mOverImage = NULL;
  mDownImage = NULL;
  mDisabledImage = NULL;
  mNormalRect = TRect<int>();
  mOverRect = TRect<int>();
  mDownRect = TRect<int>();
  mDisabledRect = TRect<int>();

  mLabelJustify = 0;
  mInverted = false;
  mBtnNoDraw = false;
  mFrameNoDraw = false;
  mListener = theButtonListener;
  mWidgetId = theId;
  mOverAlpha = 0.0;
  mOverAlphaSpeed = 0.0;
  mOverAlphaFadeInSpeed = 0.0;

  // Vtable byte 172, slot [43]: Widget::SetColors(int (*)[3], int).
  mHasAlpha = true;
  // SetColors loads the six default button colours.
  SetColors(gButtonWidgetColors, 6);

  // mUsesWidgetColor is Widget's own flag (+0xC0); the constructor raises it.
  mUsesWidgetColor = false;
  mDeselectOnEscape = false;

  mReturnPressed = false;
  mStateLocked = false;
  mLockedState = 0;

  mInsets = Insets(0, 0, 0, 0);
}

// ==========================================================================
// Sexy::ButtonWidget::HaveButtonImage(Sexy::Image *,Sexy::TRect<int> const&)
// @002F9120  16 bytes
// ==========================================================================
bool Sexy::ButtonWidget::HaveButtonImage(Sexy::Image * theImage, const TRect<int> & theRect)
{
  return theImage != NULL || theRect.mWidth != 0;
}

// ==========================================================================
// Sexy::ButtonWidget::Draw(Sexy::Graphics *)
// @002F9130  1714 bytes
// ==========================================================================
void Sexy::ButtonWidget::Draw(Sexy::Graphics * g)
{
  if ( mBtnNoDraw )
    return;

  if ( mFont == NULL && !mLabel.empty() )
  {
    // The default label font comes off the app's display object, which is what
    // the dump reads out of WidgetManager::mApp before dispatching slot [4].
    Sexy::SexyAppBase *aTheApp = mWidgetManager->mApp;
    Sexy::NativeDisplay *aDisplay =
      aTheApp->mPlatformDriverOrQueue;
    mFont = aDisplay->CreateFont(aTheApp, "Arial Unicode MS", 10, false, false, false);
  }

  // Vtable byte 532 is this class's own slot [133], UpdateLabel: the layout is
  // rebuilt only while the widget has never been updated.
  if ( mUpdateCnt == 0 )
    UpdateLabel();

  const bool aHasFocus = mHasFocus;

  // "The button looks pressed": a locked down state, or a press under the mouse
  // or on the return key, and never while the widget is disabled.
  bool aIsDown = IsLockedToDownState();
  if ( !aIsDown && ((mIsDown && mIsOver) || mReturnPressed) )
    aIsDown = !mDisabled;

  // mInverted flips which of the two frame palettes is used.
  const bool aInverted = aIsDown != mInverted;

  // The label's top-left corner. Both stay at zero when there is no font.
  int aLabelX = 0;
  int aLabelY = 0;

  if ( mFont != NULL )
  {
    if ( mLabelJustify == 1 )
      aLabelX = mWidth - mInsets.mRight + mInsets.mLeft - mTextLayout.GetWidth();
    else if ( mLabelJustify == 0 )
      aLabelX = mInsets.mLeft + (mWidth - mInsets.mLeft - mInsets.mRight - mTextLayout.GetWidth()) / 2;
    else
      aLabelX = mInsets.mLeft;

    if ( mInsets.mTop != 0 || mInsets.mBottom != 0 )
    {
      float innerHeight = (float)(mHeight - mInsets.mTop - mInsets.mBottom);
      aLabelY = (int)((float)mInsets.mTop - mLabelExtents.mMinY
                      + (innerHeight - mLabelExtents.mHeight) * 0.5f);
    }
    else
    {
      // No vertical insets: centre the ascent box, less the font's own ascent
      // padding and one more pixel. The binary spells the padding out as a
      // sixth of the ascent, which is what Font::GetAscentPadding returns.
      aLabelY = (mHeight + mFont->GetAscent() - mFont->GetAscentPadding() - 1) / 2;
    }
  }

  g->SetFont(mFont);

  if ( mButtonImage == NULL && mDownImage == NULL )
  {
    // No images at all: the button is drawn as a filled frame.
    if ( !mFrameNoDraw )
    {
      g->SetColor(mColors[5]);
      if ( mSelected )
      {
        // Scale the colour's alpha by the selection alpha at +0x110. The dump's
        // fixed-point form rounds the product by 255 with
        // `(v * s + 128 + ((v * s + 128) >> 8)) >> 8`.
        const unsigned int aScale = (unsigned int)(float)(mSelectAlpha * 255.0f);
        Sexy::Color aColor = g->GetColor();
        aColor.mAlpha = (aColor.mAlpha * aScale + 128 + ((aColor.mAlpha * aScale + 128) >> 8)) >> 8;
        g->SetColor(aColor);
      }

      g->FillRect(0, 0, mWidth, mHeight);

      if ( aInverted )
      {
        g->SetColor(mColors[2]);
        g->FillRect(0, 0, mWidth - 1, 1);
        g->FillRect(0, 0, 1, mHeight - 1);
        g->SetColor(mColors[3]);
        g->FillRect(0, mHeight - 1, mWidth, 1);
        g->FillRect(mWidth - 1, 0, 1, mHeight);
        g->SetColor(mColors[4]);
        g->FillRect(1, 1, mWidth - 3, 1);
        g->FillRect(1, 1, 1, mHeight - 3);
      }
      else
      {
        g->SetColor(mColors[3]);
        g->FillRect(0, 0, mWidth - 1, 1);
        g->FillRect(0, 0, 1, mHeight - 1);
        g->SetColor(mColors[2]);
        g->FillRect(0, mHeight - 1, mWidth, 1);
        g->FillRect(mWidth - 1, 0, 1, mHeight);
        g->SetColor(mColors[4]);
        g->FillRect(1, mHeight - 2, mWidth - 2, 1);
        g->FillRect(mWidth - 2, 1, 1, mHeight - 2);
      }
    }

    if ( aInverted )
    {
      // An inverted button shifts its label down-right by one pixel, as if the
      // text had moved with the pressed face.
      if ( mIsOver || aHasFocus )
        g->SetColor(mColors[1]);
      else
        g->SetColor(mColors[0]);

      mTextLayout.Draw(g, aLabelX + 1, aLabelY + 1, g->GetColor());
      return;
    }
  }
  else
  {
    // Images are available; pick the one the current state calls for. Every
    // call below is a virtual dispatch through slot [125], DrawButtonImage.
    if ( aInverted )
    {
      if ( HaveButtonImage(mDownImage, mDownRect) )
        DrawButtonImage(g, mDownImage, mDownRect, 0, 0);
      else if ( HaveButtonImage(mOverImage, mOverRect) )
        DrawButtonImage(g, mOverImage, mOverRect, 1, 1);
      else
        DrawButtonImage(g, mButtonImage, mNormalRect, 1, 1);

      if ( mSelected )
        g->SetColorizeImages(false);

      if ( mIsOver || aHasFocus )
        g->SetColor(mColors[1]);
      else
        g->SetColor(mColors[0]);

      mTextLayout.Draw(g, aLabelX, aLabelY, g->GetColor());
      return;
    }

    if ( mDisabled && HaveButtonImage(mDisabledImage, mDisabledRect) )
    {
      DrawButtonImage(g, mDisabledImage, mDisabledRect, 0, 0);
    }
    else if ( mOverAlpha > 0.0 && HaveButtonImage(mOverImage, mOverRect) )
    {
      // Fading the over image in: draw the normal face underneath while the
      // fade is still partial, then the over face at the fade's alpha.
      if ( HaveButtonImage(mButtonImage, mNormalRect) && mOverAlpha < 1.0 )
        DrawButtonImage(g, mButtonImage, mNormalRect, 0, 0);

      g->SetColorizeImages(true);
      g->SetColor(Sexy::Color(255, 255, 255, (int)(mOverAlpha * 255.0)));
      DrawButtonImage(g, mOverImage, mOverRect, 0, 0);
      g->SetColorizeImages(false);
    }
    else if ( (mIsOver || mIsDown || aHasFocus) && HaveButtonImage(mOverImage, mOverRect) )
    {
      if ( mSelected )
      {
        g->SetColorizeImages(true);
        g->SetColor(Sexy::Color(255, 255, 255, (int)(float)(mSelectAlpha * 255.0f)));
      }

      DrawButtonImage(g, mOverImage, mOverRect, 0, 0);

      if ( mSelected )
        g->SetColorizeImages(false);
    }
    else if ( HaveButtonImage(mButtonImage, mNormalRect) )
    {
      DrawButtonImage(g, mButtonImage, mNormalRect, 0, 0);
    }
  }

  // The common label path: over or focused text takes the highlight colour.
  if ( mIsOver || aHasFocus )
    g->SetColor(mColors[1]);
  else
    g->SetColor(mColors[0]);

  mTextLayout.Draw(g, aLabelX, aLabelY, g->GetColor());
}

// ==========================================================================
// Sexy::ButtonWidget::SetDisabled(bool)
// @002F9800  34 bytes
// ==========================================================================
void Sexy::ButtonWidget::SetDisabled(bool isDisabled)
{
  Widget::SetDisabled(isDisabled);

  if ( HaveButtonImage(mDisabledImage, mDisabledRect) )
    MarkDirty();
}

// ==========================================================================
// Sexy::ButtonWidget::MouseEnter()
// @002F9824  106 bytes
// ==========================================================================
void Sexy::ButtonWidget::MouseEnter()
{
  Widget::MouseEnter();

  // A fade-in with no step configured snaps straight to fully over.
  if ( mOverAlphaFadeInSpeed == 0.0 && mOverAlpha > 0.0 )
    mOverAlpha = 0.0;

  // Repaint when the button will actually look different: it is being pressed,
  // it has an over image, or its two label colours differ.
  if ( mIsDown || HaveButtonImage(mOverImage, mOverRect) || mColors[1] != mColors[0] )
    MarkDirty();

  mListener->ButtonMouseEnter(mWidgetId);
}

// ==========================================================================
// Sexy::ButtonWidget::MouseLeave()
// @002F9890  140 bytes
// ==========================================================================
void Sexy::ButtonWidget::MouseLeave()
{
  Widget::MouseLeave();

  // The fade starts at 0 and becomes 1 only in the second branch, so
  // this is "snap the fade to the end the configured speed asks for".
  double aTargetAlpha = 0.0;
  if ( mOverAlphaSpeed == 0.0 )
  {
    if ( mOverAlpha > 0.0 )
      mOverAlpha = aTargetAlpha;
  }
  else if ( mOverAlphaSpeed > 0.0 && mOverAlpha == 0.0 )
  {
    aTargetAlpha = 1.0;
    mOverAlpha = aTargetAlpha;
  }

  if ( mIsDown || HaveButtonImage(mOverImage, mOverRect) || mColors[1] != mColors[0] )
    MarkDirty();

  mListener->ButtonMouseLeave(mWidgetId);
}

// ==========================================================================
// Sexy::ButtonWidget::MouseMove(int,int)
// @002F991C  30 bytes
// ==========================================================================
void Sexy::ButtonWidget::MouseMove(int x, int y)
{
  Widget::MouseMove(x, y);
  mListener->ButtonMouseMove(mWidgetId, x, y);
}

// ==========================================================================
// Sexy::ButtonWidget::MouseDown(int,int,int,int)
// @002F993C  40 bytes
// ==========================================================================
void Sexy::ButtonWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
  Widget::MouseDown(x, y, theBtnNum, theClickCount);

  // ButtonPress(mId, theClickCount): listener vtable byte 12, slot [3].
  mListener->ButtonPress(mWidgetId, theClickCount);
  MarkDirty();
}

// ==========================================================================
// Sexy::ButtonWidget::MouseUp(int,int,int,int)
// @002F9964  54 bytes
// ==========================================================================
void Sexy::ButtonWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
  Widget::MouseUp(x, y, theBtnNum, theClickCount);

  bool aDepress = mIsDown && mWidgetManager != nullptr && mWidgetManager->mHasFocus;
  MarkDirty();
  if (aDepress && mListener != nullptr)
    mListener->ButtonDepress(mWidgetId);
}

// ==========================================================================
// Sexy::ButtonWidget::KeyDown(Sexy::KeyCode)
// @002F999C  48 bytes
// ==========================================================================
int Sexy::ButtonWidget::KeyDown(Sexy::KeyCode theKey)
{
  if ( theKey != KEYCODE_RETURN )
    return Widget::KeyDown(theKey);

  mReturnPressed = true;
  MarkDirty();
  if (mListener != nullptr)
    mListener->ButtonPress(mWidgetId, 1);
  return 1;
}

// ==========================================================================
// Sexy::ButtonWidget::KeyUp(Sexy::KeyCode)
// @002F99CC  56 bytes
// ==========================================================================
int Sexy::ButtonWidget::KeyUp(Sexy::KeyCode theKey)
{
  if ( theKey != KEYCODE_RETURN )
    return Widget::KeyUp(theKey);

  // Only a return that KeyDown actually registered counts as a release; the
  // dump reads the flag up front and hands that same value back.
  const int aWasDown = mReturnPressed;
  if ( !aWasDown )
    return Widget::KeyUp(theKey);

  // Opening another screen may remove this button. Finish its state before
  // handing control to the listener, and do not touch it after the callback.
  mReturnPressed = false;
  MarkDirty();
  if (mListener != nullptr)
    mListener->ButtonDepress(mWidgetId);
  return aWasDown;
}

// ==========================================================================
// Sexy::ButtonWidget::Update()
// @002F9A08  206 bytes
// ==========================================================================
void Sexy::ButtonWidget::Update()
{
  SelectableWidget::Update();

  // Vtable byte 532, slot [133]: UpdateLabel.
  UpdateLabel();

  if ( mIsDown )
  {
    // Pressed but the pointer has left: there is no over-state to fade.
    if ( !mIsOver )
      return;

    mListener->ButtonDownTick(mWidgetId);

    // The listener may have moved the widget out from under us, and the dump
    // re-tests both flags here rather than trusting the pair above.
    if ( mIsDown && !mIsOver )
      return;
  }

  if ( mIsOver )
  {
    // Fade the over-highlight in.
    if ( mOverAlphaFadeInSpeed > 0.0 && mOverAlpha < 1.0 )
    {
      mOverAlpha += mOverAlphaFadeInSpeed;
      if ( mOverAlpha > 1.0 )
        mOverAlpha = 1.0;

      MarkDirty();
    }
    return;
  }

  // Fade the over-highlight out.
  if ( mOverAlpha > 0.0 )
  {
    if ( mOverAlphaSpeed <= 0.0 )
    {
      mOverAlpha = 0.0;
    }
    else
    {
      mOverAlpha -= mOverAlphaSpeed;
      if ( mOverAlpha < 0.0 )
        mOverAlpha = 0.0;
    }

    MarkDirty();
  }
}

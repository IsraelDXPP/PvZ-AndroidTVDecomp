// ToolTipWidget -- restored from libGameMain.so.
//
// The small tool tip the seed chooser and the VS screens hang off a button: a
// title line, a flashing warning line and a wrapped body. CalculateSize() picks
// the box size from the widest of those, Draw() lays them out centred.
//
// The class is not polymorphic -- the constructor's three `Sexy::gEmptyStringData` stores
// are the shared empty string rep going into mTitle/mLabel/mWarningText, not a
// vptr -- so every call in here is a plain member call except the two virtuals
// on Sexy::Font (GetAscent and StringWidth).

#include "ToolTipWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "SexyAppBase.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ==========================================================================
// ToolTipWidget::ToolTipWidget()
// @001E8114  196 bytes
// ==========================================================================
ToolTipWidget::ToolTipWidget()
  : mTitleTextColor(32, 32, 100)
{
  // mTitle, mLabel and mWarningText are default-constructed Sexy::String, which
  // already leaves each of them holding the shared empty rep the binary stores.
  mX = 0;
  mY = 0;
  mWidth = 0;
  mHeight = 0;
  mVisible = true;
  mCenter = false;
  mMinLeft = 0;
  mMaxBottom = 600;
  mGetsLinesWidth = 0;
  mWarningFlashCounter = 0;

  // SexyAppBase::GetLocale() @0x33A2E8 is exactly
  // `Sexy::String(Sexy::setLocale(nullptr))`, falling back to the empty string,
  // and its r0 is the destination rather than an object -- it is a static-shaped
  // body -- so the locale check goes through the free function it wraps.
  const char *aLocale = Sexy::setLocale(nullptr, nullptr);
  bool aIsChinese = (aLocale != nullptr && Sexy::String(aLocale) == "zh_CN");

  // Only the warning line gets the locale-specific font; the title is always
  // BrianneTod16.
  mWarningTextFont = (Sexy::Font *)(aIsChinese ? FONT_BRIANNETOD16 : FONT_DWARVENTODCRAFT12);
  mTitleFont = FONT_BRIANNETOD16;
}

// ==========================================================================
// ToolTipWidget::Update()
// @001E821C  14 bytes
// ==========================================================================
void ToolTipWidget::Update()
{
  if (mWarningFlashCounter > 0)
    --mWarningFlashCounter;
}

// ==========================================================================
// ToolTipWidget::FlashWarning()
// @001E822C  6 bytes
// ==========================================================================
void ToolTipWidget::FlashWarning()
{
  // Draw() blinks the warning line while this counts down: 70 frames, black on
  // the first ten of every twenty.
  mWarningFlashCounter = 70;
}

// ==========================================================================
// ToolTipWidget::CalculateSize()
// @001E86B8  292 bytes
// ==========================================================================
void ToolTipWidget::CalculateSize()
{
  // The box is as wide as the widest of the title, the warning line and the
  // wrapped body lines, plus five pixels of padding on each side.
  int aMaxWidth = mTitleFont->StringWidth(mTitle);

  int aWarningWidth = mWarningTextFont->StringWidth(mWarningText);
  if (aWarningWidth > aMaxWidth)
    aMaxWidth = aWarningWidth;

  // GetLines() wraps the body to this width. The binary subtracts 30 from the
  // widest line and floors the result at 100.
  mGetsLinesWidth = aMaxWidth - 30;
  if (mGetsLinesWidth < 100)
    mGetsLinesWidth = 100;

  std::vector<Sexy::String> aLines;
  GetLines(aLines);

  // A wrapped body line can be wider than the title, so it feeds back into the
  // box width.
  for (unsigned int i = 0; i < aLines.size(); ++i)
  {
    int aLineWidth = mWarningTextFont->StringWidth(aLines[i]);
    if (aLineWidth > aMaxWidth)
      aMaxWidth = aLineWidth;
  }

  // The height starts at a six-pixel margin and grows by the title line and the
  // warning line, if either is present, with two pixels after each.
  int aHeight = 6;
  if (!mTitle.empty())
    aHeight = mTitleFont->GetAscent() + 8;
  if (!mWarningText.empty())
    aHeight += mWarningTextFont->GetAscent() + 2;

  // The body contributes one font line per wrapped line plus two pixels between
  // them. The binary folds the count in unguarded -- `2 * (count - 1)` -- so an
  // empty body does subtract its two pixels back off, and that is kept as is.
  int aLineCount = (int)aLines.size();

  mWidth = aMaxWidth + 10;
  mHeight = aHeight + 2 * (aLineCount - 1) + aLineCount * mWarningTextFont->GetAscent();
}

// ==========================================================================
// ToolTipWidget::SetLabel(Sexy::String const&)
// @001E87F0  86 bytes
// ==========================================================================
void ToolTipWidget::SetLabel(const Sexy::String& theLabel)
{
  // The dump spells TodStringTranslate destination-first -- that is the sret
  // register of the original call. The store into the member is the binary's
  // StringAssign (StringAssign), and the release block that follows it is the
  // temporary's Sexy::String destructor.
  Sexy::String aTranslated;
  TodStringTranslate(&aTranslated, theLabel);
  mLabel = aTranslated;

  CalculateSize();
}

// ==========================================================================
// ToolTipWidget::SetTitle(Sexy::String const&)
// @001E885C  86 bytes
// ==========================================================================
void ToolTipWidget::SetTitle(const Sexy::String& theTitle)
{
  Sexy::String aTranslated;
  TodStringTranslate(&aTranslated, theTitle);
  mTitle = aTranslated;

  CalculateSize();
}

// ==========================================================================
// ToolTipWidget::SetWarningText(Sexy::String const&)
// @001E88C8  88 bytes
// ==========================================================================
void ToolTipWidget::SetWarningText(const Sexy::String& theWarningText)
{
  Sexy::String aTranslated;
  TodStringTranslate(&aTranslated, theWarningText);
  mWarningText = aTranslated;

  CalculateSize();
}

// ==========================================================================
// ToolTipWidget::Draw(Sexy::Graphics *)
// @001E8938  888 bytes
// ==========================================================================
void ToolTipWidget::Draw(Sexy::Graphics *theGraphics)
{
  if (!mVisible)
    return;

  float aTransX = theGraphics->mTransX;
  float aTransY = theGraphics->mTransY;

  // The box is pushed off the left edge by the fullscreen rect's left coordinate
  // when the platform layer reports the app as fullscreen, and by -80 otherwise.
  // The platform object's type was never recovered, so the flag stays a byte
  // read at +0x60 of whatever mPlatformDriverOrQueue points at.
  LawnApp *aTheApp = (LawnApp *)gLawnApp;
  const unsigned char *aPlatform = (const unsigned char *)aTheApp->mPlatformDriverOrQueue;
  float aScreenOffsetX = (aPlatform[0x60] != 0) ? (float)LawnApp::FULLSCREEN_RECT.mX : -80.0f;

  int aDrawX = (int)((float)mX - aTransX - aScreenOffsetX);
  if (mCenter)
    aDrawX -= mWidth / 2;

  int aDrawY = mY;

  // Keep the box on screen: never left of mMinLeft and never above the top.
  if ((float)aDrawX < (float)mMinLeft - aTransX)
    aDrawX = mMinLeft - (int)aTransX;

  float aMinTop = -aTransY;
  if ((float)aDrawY < aMinTop)
    aDrawY = (int)aMinTop;

  // The panel, with a one-pixel black outline.
  if (mWidth > 0 && mHeight > 0)
  {
    theGraphics->SetColor(Sexy::Color(255, 255, 200, 255));
    theGraphics->FillRect(aDrawX, aDrawY, mWidth, mHeight);

    theGraphics->SetColor(Sexy::Color::Black);
    theGraphics->DrawRect(aDrawX, aDrawY, mWidth - 1, mHeight - 1);
  }

  int aTextY = aDrawY + 1;

  // Title line, centred and drawn in mTitleTextColor.
  if (!mTitle.empty())
  {
    theGraphics->SetFont(mTitleFont);
    theGraphics->SetColor(mTitleTextColor);

    int aTitleAscent = mTitleFont->GetAscent();
    int aTitleWidth = mTitleFont->StringWidth(mTitle);

    theGraphics->DrawString(mTitle, aDrawX + (mWidth - aTitleWidth) / 2, aTextY + aTitleAscent);
    aTextY += aTitleAscent + 2;
  }

  // Warning line, centred in red, blinking black on the first ten frames of
  // every twenty while the counter runs down.
  if (!mWarningText.empty())
  {
    theGraphics->SetFont(mWarningTextFont);

    int aWarningAscent = mWarningTextFont->GetAscent();
    int aWarningWidth = mWarningTextFont->StringWidth(mWarningText);

    Sexy::Color aWarningColor(255, 0, 0);
    if (mWarningFlashCounter > 0 && mWarningFlashCounter % 20 <= 9)
      aWarningColor = Sexy::Color::Black;

    theGraphics->SetColor(aWarningColor);
    theGraphics->DrawString(mWarningText, aDrawX + (mWidth - aWarningWidth) / 2, aTextY + aWarningAscent);

    theGraphics->SetColor(Sexy::Color::Black);
    aTextY += aWarningAscent + 2;
  }

  // The wrapped body: one line per element of what GetLines produces, two
  // pixels apart, in the warning font.
  int aLineY = aTextY + 1;

  std::vector<Sexy::String> aLines;
  GetLines(aLines);

  theGraphics->SetFont(mWarningTextFont);
  theGraphics->SetColor(Sexy::Color(32, 32, 32));

  for (unsigned int i = 0; i < aLines.size(); ++i)
  {
    // The binary copy-constructs each line before measuring it; on this string
    // type that copy is a refcount bump on the shared buffer.
    Sexy::String aLine(aLines[i]);

    int aLineAscent = mWarningTextFont->GetAscent();
    int aLineWidth = mWarningTextFont->StringWidth(aLine);

    theGraphics->DrawString(aLine, aDrawX + (mWidth - aLineWidth) / 2, aLineY + aLineAscent);
    aLineY += aLineAscent + 2;
  }
}

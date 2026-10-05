// TextLayout -- restored from libGameMain.so.
//
// The bodies below are the decompilation rewritten as ordinary C++: every
// this-relative offset is a named member, every hand-written vtable call is a
// virtual call on the real type, and the two-step `operator new` + placement-new
// pairs are real constructions. Where the binary's control flow is unusual the
// raw dump's labels are named in comments.
//
// Four transcription defects are fixed here, all the same 4x-offset mistake:
// Draw, DrawGlyphs (twice) and DrawLines took the receiver plus eight bytes --
// mFont -- where the raw dump reads dword 8, i.e. mRect.mX at +0x20. All four
// are the x offset added to the caller's x.
//
// TODO(recon): two members of this class are real functions in the binary but
// are not restored, because the types they need do not exist in this tree yet:
//   * BuildLine @0x3BE160 (1594 bytes) -- its exact signature is in
//     TextLayout.h; it needs Sexy::TextRun and the real 60-byte Sexy::Glyph.
//     The definition below is a stub that reports the missing piece.
//   * GetGlyphsWidth @0x3BD604 -- same Glyph problem.
// Also not restored: SetText(std::string const&, bool) @0x3BD244, the narrow
// front-end that converts through Graphics::WStringFromString and forwards to
// the wide worker @0x3BD160 that IS restored below (this tree maps both string
// types onto Sexy::String, so the two overloads would collide).
//
// And due to the missing BuildLine, BuildLines cannot lay anything out yet.

#include "TextLayout.h"
#include "Font.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ==========================================================================
// Sexy::TextLayout::Init()
// @003BD11C  66 bytes
// ==========================================================================
void Sexy::TextLayout::Init()
{
  mCachePolicy = 0;
  mCacheImageValid = false;
  mFont = nullptr;
  mWidth = 0;
  mHeight = 0;
  mNumGlyphs = 0;
  mWrap = false;
  mDrawStringUnicode = false;
  mRect.mX = 0;
  mRect.mY = 0;
  mRect.mWidth = 0;
  mRect.mHeight = 0;
  mCacheImageColorizeEnabled = false;
  mCacheImageColorizeCount = 0;
  mCacheImageColorizeLimit = 0;
  mLineSpacing = -1;
  mJustification = -1;
  mSingleLine = true;
  mWrapInSpace = true;
  mDirty = true;
}

// ==========================================================================
// Sexy::TextLayout::SetText(Sexy::String const&,bool)
// @003BD160  220 bytes
// ==========================================================================
void Sexy::TextLayout::SetText(Sexy::String const& theString, bool theDrawStringUnicode)
{
  // The binary's worker compares the new text against mString first (a
  // hand-inlined char_traits<int>::compare in the wide string) and returns
  // without touching anything when both the text and the flag already match.
  if (mString == theString && mDrawStringUnicode == theDrawStringUnicode)
    return;

  // libstdc++'s COW assign: refcount the source, release the target.
  mString = theString;
  mDrawStringUnicode = theDrawStringUnicode;
  mDirty = true;
}

// ==========================================================================
// Sexy::TextLayout::SetFont(Sexy::Font *)
// @003BD294  14 bytes
// ==========================================================================
void Sexy::TextLayout::SetFont(Sexy::Font * theFont)
{
  if (mFont != theFont)
  {
    mFont = theFont;
    mDirty = true;
  }
}

// ==========================================================================
// Sexy::TextLayout::GetFont()
// @003BD2A4  4 bytes
// ==========================================================================
Sexy::Font * Sexy::TextLayout::GetFont()
{
  return mFont;
}

// ==========================================================================
// Sexy::TextLayout::GetRect() const
// @003BD2A8  22 bytes
// ==========================================================================
TRect<int> Sexy::TextLayout::GetRect() const
{
  // The binary's r0 is the hidden return slot and r1 is the receiver, so this
  // is a TRect<int> returned by value.
  return mRect;
}

// ==========================================================================
// Sexy::TextLayout::SetRect(Sexy::TRect<int> const&)
// @003BD2C0  110 bytes
// ==========================================================================
void Sexy::TextLayout::SetRect(TRect<int> const &theRect)
{
  if (mRect != theRect)
  {
    mRect = theRect;
    if (mRect.mX < 0)
      mRect.mX = 0;
    if (mRect.mWidth != 0 && mRect.mWidth <= mRect.mX)
      mRect.mX = mRect.mWidth - 1;
    if (mRect.mY < 0)
      mRect.mY = 0;
    // The original test compares the height against mRect.mX and, on a hit,
    // stores mRect.mHeight - 1 into mRect.mY.
    if (mRect.mHeight != 0 && mRect.mHeight <= mRect.mX)
      mRect.mY = mRect.mHeight - 1;
    mDirty = true;
  }
}

// Dialog still passes the four rect words as an int array.
void Sexy::TextLayout::SetRect(int *theRect)
{
  // The four words are TRect's x, y, width and height, in that order.
  TRect<int> aRect(theRect[0], theRect[1], theRect[2], theRect[3]);
  SetRect(aRect);
}

// ==========================================================================
// Sexy::TextLayout::SetWidth(int)
// @003BD330  28 bytes
// ==========================================================================
void Sexy::TextLayout::SetWidth(int theWidth)
{
  TRect<int> aRect(mRect.mX, mRect.mY, theWidth, mRect.mHeight);
  SetRect(aRect);
}

// ==========================================================================
// Sexy::TextLayout::SetHeight(int)
// @003BD34C  28 bytes
// ==========================================================================
void Sexy::TextLayout::SetHeight(int theHeight)
{
  TRect<int> aRect(mRect.mX, mRect.mY, mRect.mWidth, theHeight);
  SetRect(aRect);
}

// ==========================================================================
// Sexy::TextLayout::SetCachePolicy(Sexy::TextCachePolicy)
// @003BD368  4 bytes
// ==========================================================================
void Sexy::TextLayout::SetCachePolicy(int theCachePolicy)
{
  mCachePolicy = theCachePolicy;
}

// ==========================================================================
// Sexy::TextLayout::GetCachePolicy() const
// @003BD36C  4 bytes
// ==========================================================================
int Sexy::TextLayout::GetCachePolicy() const
{
  return mCachePolicy;
}

// ==========================================================================
// Sexy::TextLayout::FastDrawLine(Sexy::Graphics *,Sexy::TextLine &,int,int,Sexy::Color const&,int,Sexy::TRect<int> const&,Sexy::TRect<int> const&)
// @003BD370  158 bytes
// ==========================================================================
int Sexy::TextLayout::FastDrawLine(Sexy::Graphics * theGraphics, Sexy::TextLine & theLine, int theX, int theY, Color const& theColor, int theAlign, TRect<int> const& theClipRect, TRect<int> const& theFullRect)
{
  int aX = theX;
  if (theAlign != 0)
  {
    if (theAlign == 1)
      aX = theX + theClipRect.mWidth - theLine.mWidth;
  }
  else
  {
    aX = theX + (theClipRect.mWidth - theLine.mWidth) / 2;
  }

  int aColorizeImages = theGraphics->GetColorizeImages();
  theGraphics->SetColor(theColor);
  if (!mFont->IsComposited() && !mDrawStringUnicode)
    theGraphics->SetColorizeImages(true);

  // Draw the cached image of the whole layout at the line's baseline.
  theGraphics->DrawImage(&mCacheImage, aX,
                         theY - mFont->GetAscent() + mFont->GetAscentPadding(),
                         theClipRect);
  theGraphics->SetColorizeImages(aColorizeImages != 0);
  return 0;
}

// ==========================================================================
// Sexy::TextLayout::DrawLine(Sexy::Graphics *,Sexy::TextLine &,unsigned int,unsigned int,int,int,Sexy::Color const&,int,Sexy::TRect<int> const&)
// @003BD410  358 bytes
// ==========================================================================
int Sexy::TextLayout::DrawLine(Sexy::Graphics * theGraphics, Sexy::TextLine & theLine, unsigned int theStart, unsigned int theCount, int theX, int theY, Color const& theColor, int theAlign, TRect<int> const& theClipRect)
{
  int aX = theX;
  if (theAlign != 0)
  {
    if (theAlign == 1)
      aX = theX + theClipRect.mWidth - theLine.mWidth;
  }
  else
  {
    aX = theX + (theClipRect.mWidth - theLine.mWidth) / 2;
  }

  // theStart is an absolute glyph index into the line. aEnd is exclusive.
  const unsigned int aEnd = theStart + theCount;
  unsigned int aPos = theStart;
  unsigned int aRunStart = 0;
  int result = (int)aEnd;
  for (Sexy::TextRun& aRun : theLine.mRuns)
  {
    const unsigned int aRunGlyphs = (unsigned int)aRun.mGlyphs.size();
    Color aRunColor = theColor;
    if (aRun.mHasColor)
      aRunColor = Color(aRun.mRed, aRun.mGreen, aRun.mBlue, theColor.mAlpha);

    unsigned int aDrawCount = 0;
    if (aPos >= aRunStart)
    {
      aDrawCount = (aEnd >= aRunGlyphs + aRunStart) ? aRunGlyphs + aRunStart - aPos
                                                    : aEnd - aPos;
      mFont->DrawGlyphs(theGraphics, aX, theY, aRun.mGlyphs,
                        aPos - aRunStart, aDrawCount, aRunColor,
                        theGraphics->mClipRect);
    }

    aPos += aDrawCount;
    aRunStart += aRunGlyphs;
    if (aPos == aEnd)
      break;
    aX += aRun.mWidth;
  }
  return result;
}

// ==========================================================================
// Sexy::TextLayout::SetLineSpacing(int)
// @003BD578  14 bytes
// ==========================================================================
void Sexy::TextLayout::SetLineSpacing(int theLineSpacing)
{
  if (mLineSpacing != theLineSpacing)
  {
    mLineSpacing = theLineSpacing;
    mDirty = true;
  }
}

// ==========================================================================
// Sexy::TextLayout::GetLineSpacing() const
// @003BD588  4 bytes
// ==========================================================================
int Sexy::TextLayout::GetLineSpacing() const
{
  return mLineSpacing;
}

// ==========================================================================
// Sexy::TextLayout::SetJustification(int)
// @003BD58C  22 bytes
// ==========================================================================
void Sexy::TextLayout::SetJustification(int theJustification)
{
  // The binary accepts only -1, 0 and 1.
  if ((static_cast<unsigned int>(theJustification + 1) <= 1 || theJustification == 1)
      && mJustification != theJustification)
    mJustification = theJustification;
}

// ==========================================================================
// Sexy::TextLayout::GetJustification() const
// @003BD5A4  4 bytes
// ==========================================================================
int Sexy::TextLayout::GetJustification() const
{
  return mJustification;
}

// ==========================================================================
// Sexy::TextLayout::SetWrap(bool)
// @003BD5A8  24 bytes
// ==========================================================================
void Sexy::TextLayout::SetWrap(bool theWrap)
{
  if (mWrap != theWrap)
  {
    // The binary loads mSingleLine (a byte at +0x40) before storing the new
    // mWrap, and marks the layout dirty only when it is clear.
    const bool aWasSingleLine = mSingleLine;
    mWrap = theWrap;
    if (!aWasSingleLine)
      mDirty = true;
  }
}

// ==========================================================================
// Sexy::TextLayout::GetWrap() const
// @003BD5C0  6 bytes
// ==========================================================================
int Sexy::TextLayout::GetWrap()
{
  return mWrap;
}

// ==========================================================================
// Sexy::TextLayout::SetWrapInSpace(bool)
// @003BD5C8  24 bytes
// ==========================================================================
void Sexy::TextLayout::SetWrapInSpace(bool theWrapInSpace)
{
  if (mWrapInSpace != theWrapInSpace)
  {
    const bool aWasSingleLine = mSingleLine;
    mWrapInSpace = theWrapInSpace;
    if (!aWasSingleLine)
      mDirty = true;
  }
}

// ==========================================================================
// Sexy::TextLayout::GetWrapInSpace() const
// @003BD5E0  6 bytes
// ==========================================================================
int Sexy::TextLayout::GetWrapInSpace()
{
  return mWrapInSpace;
}

// ==========================================================================
// Sexy::TextLayout::SetSingleLine(bool)
// @003BD5E8  18 bytes
// ==========================================================================
void Sexy::TextLayout::SetSingleLine(bool theSingleLine)
{
  if (mSingleLine != theSingleLine)
  {
    mSingleLine = theSingleLine;
    mDirty = true;
  }
}

// ==========================================================================
// Sexy::TextLayout::GetSingleLine() const
// @003BD5FC  6 bytes
// ==========================================================================
int Sexy::TextLayout::GetSingleLine()
{
  return mSingleLine;
}

// ==========================================================================
// Sexy::TextLayout::UpdateCache(Sexy::Color const&,bool)
// @003BD638  2 bytes
// ==========================================================================
void Sexy::TextLayout::UpdateCache(Color const& theColor, bool theForce)
{
  // The binary's body is a single `BX LR`: this Android build compiles the
  // colourised-cache builder out, which is also why mCacheImageValid,
  // mCacheBuiltColor, mCacheImageColorizeEnabled and mCacheImageColorizeLimit
  // are never written.
  (void)theColor;
  (void)theForce;
}

// ==========================================================================
// Sexy::TextLayout::BuildLines()
// @003BFA5C  3552 bytes
// ==========================================================================
void Sexy::TextLayout::BuildLines()
{
  // Free every line's glyph runs and rewind the line vector. The binary keeps
  // the vector's storage here (it only stores mLines.begin into mLines.end).
  DiscardLines();

  mNumGlyphs = 0;

  // The running colour and the flag that says whether it came from a `^`
  // escape. BuildLine appends runs to the line and writes both back.
  int aLineHeight = mLineSpacing;
  bool aHasColor = false;
  Color aColor = Color::White;
  if (aLineHeight <= 0)
    aLineHeight = mFont->GetLineSpacing();

  if (mSingleLine)
  {
    // One line holding the whole string; no wrapping at all.
    mLines.push_back(Sexy::TextLine());
    Sexy::TextLine &aLine = mLines[mLines.size() - 1];

    String aString(mString);
    const int aWidth = BuildLine(aString, 0, aString.size(), mDrawStringUnicode, aLine,
                                 aHasColor, aColor);

    mNumGlyphs = aLine.mGlyphCount;
    aLine.mWidth = aWidth;
    aLine.mHeight = aLineHeight;
    mWidth = aWidth;
    mHeight = aLineHeight;
    return;
  }

  // Walk the string, cut a line at the last space that still fits, and
  // keep the widest line plus the running height.
  const int aLength = mString.size();
  int aMaxWidth = 0;
  int aTotalHeight = 0;
  if (aLength <= 0)
  {
    mWidth = aMaxWidth;
    mHeight = aTotalHeight;
    return;
  }

  int aLineStart = 0;
  int aPos = 0;
  int aSpacePos = -1;
  int aCurWidth = 0;
  int aPrevChar = 0;
  int aLineOffset = mRect.mX;
  int aChar = 0;

  for (;;)
  {
    // ---- scan forward until the line has to break -------------------------
    bool aBreakLine = false;
    while (aPos < aLength)
    {
      aChar = mString[aPos];
      if (aChar == '^')
      {
        // A `^` escape. `^^` is a literal caret and measures one caret;
        // anything else skips eight characters and contributes no width. Only
        // the unicode path looks at escapes at all.
        if (mDrawStringUnicode && aLength > aPos + 1)
        {
          if (mString[aPos + 1] != '^')
          {
            aPos += 8;
            continue;
          }
          ++aPos;
        }
      }
      else if (aChar == ' ')
      {
        if (mWrapInSpace)
          aSpacePos = aPos;
      }
      else if (aChar == '\n')
      {
        aSpacePos = aPos;
        ++aPos;
        // The newline's own width is measured but not added to the line.
        mFont->CharWidthKern('\n', aPrevChar);
        aBreakLine = true;
        break;
      }

      aCurWidth += mFont->CharWidthKern(aChar, aPrevChar);
      if (mWrap && mRect.mWidth < aCurWidth)
      {
        aBreakLine = true;
        break;
      }
      ++aPos;
      aPrevChar = aChar;
    }

    if (!aBreakLine)
      break;   // the end of the text, or a caret escape that ran past it

    // Cut a line and append it.
    mLines.push_back(Sexy::TextLine());
    Sexy::TextLine &aLine = mLines[mLines.size() - 1];

    int aLineWidth;
    int aLineStep;   // the height this line adds to the total
    if (aSpacePos == -1)
    {
      // No break opportunity: cut at the current position. The guard is what
      // stops a single character that is already wider than the line from
      // looping forever.
      if (aLineStart >= aPos)
        ++aPos;

      String aString(mString);
      aLineWidth = BuildLine(aString, aLineStart, aPos - aLineStart, mDrawStringUnicode, aLine,
                             aHasColor, aColor);

      aLine.mWidth = aLineWidth;
      aLine.mHeight = aLineHeight;
      mNumGlyphs += aLine.mGlyphCount;
      aLineStart = aPos;
      aLineStep = aLineHeight;
    }
    else
    {
      String aString(mString);
      aLineWidth = BuildLine(aString, aLineStart, aSpacePos - aLineStart, mDrawStringUnicode, aLine,
                             aHasColor, aColor) + aLineOffset;

      aLine.mWidth = aLineWidth;
      aLine.mHeight = aLineHeight;
      mNumGlyphs += aLine.mGlyphCount;
      aLineStart = aSpacePos + 1;
      if (aChar == ' ' && mWrapInSpace && aLength > aLineStart)
      {
        // Swallow the run of spaces that follows the break.
        int aNext = aSpacePos + 1;
        while (aLength != aNext && mString[aNext] == ' ')
          ++aNext;
        aLineStart = aNext;
        aLineStep = aLine.mHeight;
      }
      else
      {
        aLineStep = aLineHeight;
      }
    }

    // ---- the tail: start the next line ------------------------------------
    aSpacePos = -1;
    aCurWidth = 0;
    aLineOffset = 0;
    if (aMaxWidth < aLineWidth)
      aMaxWidth = aLineWidth;
    aPrevChar = 0;
    aTotalHeight += aLineStep;
    aPos = aLineStart;
    if (aLength <= aLineStart)
      break;
  }

  // The leftover text that did not need another wrap.
  if (aLength > aLineStart)
  {
    mLines.push_back(Sexy::TextLine());
    Sexy::TextLine &aLine = mLines[mLines.size() - 1];

    String aString(mString);
    const int aWidth = BuildLine(aString, aLineStart, aLength - aLineStart, mDrawStringUnicode,
                                 aLine, aHasColor, aColor);

    aLine.mWidth = aWidth;
    aLine.mHeight = aLineHeight;
    aTotalHeight += aLineHeight;
    mNumGlyphs += aLine.mGlyphCount;
    if (aWidth >= aMaxWidth)
      aMaxWidth = aWidth;
  }
  else if (aChar == '\n')
  {
    // The text ended with a newline: the empty last line still takes a row.
    mLines.push_back(Sexy::TextLine());
    Sexy::TextLine &aLine = mLines[mLines.size() - 1];
    aLine.mWidth = 0;
    aLine.mHeight = aLineHeight;
    aLine.mGlyphCount = 0;
    aTotalHeight += aLineHeight;
  }

  mWidth = aMaxWidth;
  mHeight = aTotalHeight;
}

// ==========================================================================
// Sexy::TextLayout::TextLayout()
// @003C0850  70 bytes
// ==========================================================================
Sexy::TextLayout::TextLayout()
{
  // The binary zeroes mLines' three words, seeds mString with the empty rep,
  // zeroes mRect's four words and constructs mCacheImage and the two Colors at
  // +0x44 / +0x12C / +0x140 explicitly. All of that is member construction
  // here, so only Init is left.
  Init();
}

// ==========================================================================
// Sexy::TextLayout::TextLayout(Sexy::String const&,Sexy::Font *,bool,bool)
// @003C08C0  106 bytes
// ==========================================================================
Sexy::TextLayout::TextLayout(Sexy::String const& theString, Sexy::Font * theFont, bool theDrawStringUnicode, bool theSingleLine)
{
  Init();
  mFont = theFont;
  mDrawStringUnicode = theDrawStringUnicode;
  mSingleLine = theSingleLine;
  // The binary passes a literal `false` here, and SetText stores its bool back
  // into mDrawStringUnicode, so the third argument is overwritten unless the
  // text is empty and the flag is already clear. Kept as the binary behaves.
  SetText(theString, false);
}

// ==========================================================================
// Sexy::TextLayout::~TextLayout()
// @003C09E8  150 bytes
// ==========================================================================
Sexy::TextLayout::~TextLayout()
{
  // The binary destroys mCacheImage (0x3C09F8), then frees every line's glyph
  // runs and the runs array, then the line array (0x3C0A0A..0x3C0A54), then
  // releases mString. Here the member destructors free mCacheImage, the line
  // array and mString; this call frees the glyph runs.
  DiscardLines();
}

// ==========================================================================
// Sexy::TextLayout::DiscardLines(void)
// (inlined by the binary into BuildLines @003BFA5C, Update @003C0AA8 and
//  ~TextLayout @003C09E8. The destructor's disassembly at 0x3C0A0A is the
//  reference copy: a 24-byte stride over the lines, a 36-byte stride over each
//  line's runs, `operator delete` of each run's glyph array and then of the run
//  array; BuildLines and Update additionally rewind the vector.)
// ==========================================================================
void Sexy::TextLayout::DiscardLines()
{
  mLines.clear();
}

// ==========================================================================
// Sexy::TextLayout::Update()
// @003C0AA8  180 bytes
// ==========================================================================
void Sexy::TextLayout::Update()
{
  if (!mDirty)
    return;

  if (mFont != nullptr)
  {
    BuildLines();
  }
  else
  {
    // No font, so there is nothing to lay out: the lines are dropped.
    DiscardLines();
    mWidth = 0;
    mHeight = 0;
  }

  mDirty = false;
  mCacheImageValid = false;
  mCacheImage.Create(0, 0);
  UpdateCache(Color::White, false);
}

// ==========================================================================
// Sexy::TextLayout::GetNumGlyphs()
// @003C0B64  12 bytes
// ==========================================================================
int Sexy::TextLayout::GetNumGlyphs()
{
  Update();
  return mNumGlyphs;
}

// ==========================================================================
// Sexy::TextLayout::GetWidth()
// @003C0B70  12 bytes
// ==========================================================================
int Sexy::TextLayout::GetWidth()
{
  Update();
  return mWidth;
}

// ==========================================================================
// Sexy::TextLayout::GetHeight()
// @003C0B7C  12 bytes
// ==========================================================================
int Sexy::TextLayout::GetHeight()
{
  Update();
  return mHeight;
}

// ==========================================================================
// Sexy::TextLayout::Draw(Sexy::Graphics *,int,int,Sexy::Color const&)
// @003C0B88  792 bytes
// ==========================================================================
void Sexy::TextLayout::Draw(Sexy::Graphics * theGraphics, int theX, int theY, Color const& theColor)
{
  if (mFont == nullptr)
    return;
  Update();
  if (mLines.size() == 0)
    return;

  const Color aSavedColor = theGraphics->GetColor();

  // The colour-cache bookkeeping. UpdateCache is a `BX LR` stub in this build and
  // nothing ever sets mCacheImageColorizeEnabled, so this block cannot fire; it
  // is restored because it is what the binary computes.
  if (mCacheImageColorizeEnabled && (mFont->IsComposited() || mDrawStringUnicode))
  {
    if (mLastDrawColor == theColor)
    {
      ++mCacheImageColorizeCount;
      if (!mCacheImageValid && mCacheImageColorizeCount > mCacheImageColorizeLimit)
        UpdateCache(theColor, true);
    }
    else
    {
      mCacheImageColorizeCount = 0;
      mCacheImageValid = false;
      mLastDrawColor = theColor;
    }
  }

  if (mSingleLine)
  {
    // The whole layout is one line.
    Sexy::TextLine &aLine = mLines[0];
    TRect<int> aClipRect(mRect);
    const TRect<int> aLineRect(0, 0, aLine.mWidth, aLine.mHeight);
    if (mRect.mWidth == 0)
      aClipRect.mWidth = aLine.mWidth;

    if (mRect.mHeight == 0 || aLine.mHeight + mRect.mY <= mRect.mHeight)
    {
      const int aX = mRect.mX + theX;
      const int aY = mRect.mY + theY;
      if (mCacheImageValid)
      {
        FastDrawLine(theGraphics, aLine, aX, aY, theColor, mJustification, aClipRect, aLineRect);
      }
      else
      {
        // The dump passes the byte at +0x13C as the start index here; it is 0
        // on this path, which is why the line draws from its first glyph.
        DrawLine(theGraphics, aLine, 0, aLine.mGlyphCount, aX, aY, theColor, mJustification,
                 aClipRect);
      }
    }
  }
  else
  {
    // The first line sits one ascent below the rect.
    const int aFirstLineY = mRect.mY + mFont->GetAscent() - mFont->GetAscentPadding();
    int aLineY = 0;
    int aX = mRect.mX;
    for (unsigned int i = 0; i < mLines.size(); ++i)
    {
      Sexy::TextLine &aLine = mLines[i];
      TRect<int> aClipRect(mRect);
      if (mRect.mWidth == 0)
        aClipRect.mWidth = aLine.mWidth;
      const TRect<int> aLineRect(0, aLineY, aLine.mWidth, aLine.mHeight);

      if (mRect.mHeight == 0
          || (aLineY + mRect.mY >= 0 && aLineY + mRect.mY + aLine.mHeight <= mRect.mHeight))
      {
        if (mCacheImageValid)
          FastDrawLine(theGraphics, aLine, aX + theX, theY + aFirstLineY + aLineY, theColor,
                       mJustification, aClipRect, aLineRect);
        else
          DrawLine(theGraphics, aLine, 0, aLine.mGlyphCount, theX + aX, theY + aFirstLineY + aLineY,
                   theColor, mJustification, aClipRect);
      }

      aLineY += aLine.mHeight;
      aX = 0;
    }
  }

  theGraphics->SetColor(aSavedColor);
}

// ==========================================================================
// Sexy::TextLayout::DrawGlyphs(Sexy::Graphics *,unsigned int,unsigned int,int,int,Sexy::Color const&)
// @003C0EA0  420 bytes
// ==========================================================================
int Sexy::TextLayout::DrawGlyphs(Sexy::Graphics * theGraphics, unsigned int theStart, unsigned int theCount, int theX, int theY, Color const& theColor)
{
  Update();
  int result = mNumGlyphs;
  if (result == 0 || static_cast<unsigned int>(result - 1) < theStart)
    return result;

  unsigned int aLineCount = mLines.size();

  // The line the first requested glyph belongs to, and how many glyphs precede
  // it (the dump walks the same accumulating counter in two nested loops).
  unsigned int aLine = 0;
  unsigned int aGlyphBefore = 0;
  while (aLine < aLineCount && theStart >= aGlyphBefore + mLines[aLine].mGlyphCount)
  {
    aGlyphBefore += mLines[aLine].mGlyphCount;
    ++aLine;
  }

  unsigned int aEnd = theStart + theCount;
  if (aEnd >= static_cast<unsigned int>(mNumGlyphs))
    aEnd = mNumGlyphs;

  int aFirstLineY = mRect.mY;
  if (!mSingleLine)
  {
    const int aAscent = mFont->GetAscent();
    const int aAscentPadding = mFont->GetAscentPadding();
    result = aAscent - aAscentPadding;
    aFirstLineY += result;
    // The two virtual calls above can rebuild the layout, so the dump reloads
    // the line count here.
    aLineCount = mLines.size();
  }

  if (aLine < aLineCount)
  {
    int aX = mRect.mX;
    int aLineY = 0;
    unsigned int aIndex = aLine;
    unsigned int aPos = theStart;

    for (;;)
    {
      Sexy::TextLine &aThisLine = mLines[aIndex];
      TRect<int> aClipRect(mRect);
      if (mRect.mWidth == 0)
        aClipRect.mWidth = aThisLine.mWidth;

      const unsigned int aStartInLine = aPos - aGlyphBefore;
      unsigned int aLineGlyphs = aThisLine.mGlyphCount;
      unsigned int aDrawCount = aEnd - aPos;
      if (aEnd - aPos >= aLineGlyphs)
        aDrawCount = aLineGlyphs - aStartInLine;

      if (mRect.mHeight == 0
          || (aLineY + mRect.mY >= 0
              && aLineY + mRect.mY + aThisLine.mHeight <= mRect.mHeight))
      {
        result = DrawLine(theGraphics, aThisLine, aStartInLine, aDrawCount, theX + aX,
                          theY + aFirstLineY + aLineY, theColor, mJustification, aClipRect);
        aLineGlyphs = aThisLine.mGlyphCount;
      }

      aPos += aDrawCount;
      aGlyphBefore += aLineGlyphs;
      if (aPos == aEnd)
        break;

      ++aIndex;
      aLineY += aThisLine.mHeight;
      if (aIndex >= aLineCount)
        break;
      aX = 0;
    }
  }
  return result;
}

// ==========================================================================
// Sexy::TextLayout::DrawLines(Sexy::Graphics *,unsigned int,unsigned int,int,int,Sexy::Color const&)
// @003C1044  302 bytes
// ==========================================================================
void Sexy::TextLayout::DrawLines(Sexy::Graphics * theGraphics, unsigned int theStart, unsigned int theCount, int theX, int theY, Color const& theColor)
{
  Update();
  if (mSingleLine != 0)
  {
    // A single line is drawn by the whole-layout path, and only when the whole
    // range is asked for.
    if (theCount == 1 && theStart == 0)
      Draw(theGraphics, theX, theY, theColor);
    return;
  }

  const Color aSavedColor = theGraphics->GetColor();

  const unsigned int aEnd = theStart + theCount;
  const unsigned int aLineCount = mLines.size();
  const unsigned int aCount = (aLineCount >= aEnd) ? aEnd : aLineCount;

  // The dump re-tests mSingleLine here even though it is already known to be
  // clear; the layout is never single-line on this path.
  const int aFirstLineY = mRect.mY + mFont->GetAscent() - mFont->GetAscentPadding();

  if (aCount != 0)
  {
    int aLineY = 0;
    int aX = mRect.mX;
    for (unsigned int aLine = 0; aLine != aCount; ++aLine)
    {
      Sexy::TextLine &aThisLine = mLines[aLine];
      TRect<int> aClipRect(mRect);
      if (mRect.mWidth == 0)
        aClipRect.mWidth = aThisLine.mWidth;

      if (theStart <= aLine
          && (mRect.mHeight == 0
              || (aLineY + mRect.mY >= 0
                  && aLineY + mRect.mY + aThisLine.mHeight <= mRect.mHeight)))
      {
        DrawLine(theGraphics, aThisLine, 0, aThisLine.mGlyphCount, theX + aX,
                 theY + aFirstLineY + aLineY, theColor, mJustification, aClipRect);
      }

      aLineY += aThisLine.mHeight;
      aX = 0;
    }
  }

  theGraphics->SetColor(aSavedColor);
}

// ==========================================================================
// Sexy::TextLayout::DrawLine(Sexy::Graphics *,unsigned int,int,int,Sexy::Color const&)
// @003C1174  24 bytes
// ==========================================================================
void Sexy::TextLayout::DrawLine(Sexy::Graphics * theGraphics, unsigned int theStart, int theX, int theY, Color const& theColor)
{
  DrawLines(theGraphics, theStart, 1u, theX, theY, theColor);
}

// ==========================================================================
// Sexy::TextLayout::GetLines()
// @003C118C  14 bytes
// ==========================================================================
std::vector<Sexy::TextLine> & Sexy::TextLayout::GetLines()
{
  // The binary returns `this + 0x10`, i.e. the vector itself.
  Update();
  return mLines;
}

// ==========================================================================
// Sexy::TextLayout::GetLine(unsigned int)
// @003C119C  46 bytes
// ==========================================================================
Sexy::TextLine * Sexy::TextLayout::GetLine(unsigned int theIndex)
{
  Update();
  if (theIndex >= mLines.size())
    return nullptr;
  return &mLines[theIndex];
}

// ==========================================================================
// Sexy::TextLayout::GetLineExtents(unsigned int)
// @003C11CC  32 bytes
// ==========================================================================
TPoint<int> Sexy::TextLayout::GetLineExtents(unsigned int theLine)
{
  // The binary writes two words through r0, the hidden return slot, and reads
  // the line through r1, so this returns a TPoint<int> by value.
  Sexy::TextLine *aLine = GetLine(theLine);
  if (aLine == nullptr)
    return TPoint<int>(0, 0);
  return TPoint<int>(aLine->mWidth, aLine->mHeight);
}

// ==========================================================================
// Sexy::TextLayout::BuildLine(Sexy::String,int,int,bool,Sexy::TextLine &,bool &,Sexy::Color &)
// @003BE160  1594 bytes
// ==========================================================================
int Sexy::TextLayout::BuildLine(Sexy::String theString, int theStart, int theLength, bool theDrawStringUnicode, Sexy::TextLine & theLine, bool & theHasColor, Color & theColor)
{
  // TODO(recon): NOT RESTORED. The body is at 0x3BE160 and its mangled name
  // (_ZN4Sexy10TextLayout9BuildLineESbIiSt11char_traitsIiESaIiEEiibRNS_8TextLineERbRNS_5ColorE)
  // fixes the signature above. What it does, from the raw decompilation:
  //   * walks [theStart, theStart + theLength) of theString and handles the `^`
  //     escapes: `^^` emits one literal caret, `^` plus eight hex digits selects
  //     a colour, and `^oldclr` resets it to Sexy::Color::White;
  //   * appends one Sexy::TextRun (the 36-byte record at the top of this file)
  //     per colour run through theLine's run vector, storing the run's colour
  //     only when theHasColor is set on entry;
  //   * calls the font's StringToGlyphs (vtable offset 56) for each run and adds
  //     the run's glyph width, from GetGlyphsWidth @0x3BD604, to the run;
  //   * returns the sum of every run's width, i.e. the line's width, and writes
  //     back through theHasColor whether the line ended inside an escape.
  // It needs Sexy::TextRun and the real 60-byte Sexy::Glyph, neither of which is
  // in ReconTypes.h yet. Returning 0 leaves every line empty, so BuildLines
  // produces no glyphs until this is restored.
  (void)theString;
  (void)theStart;
  (void)theLength;
  (void)theDrawStringUnicode;
  (void)theLine;
  (void)theHasColor;
  (void)theColor;
  return 0;
}

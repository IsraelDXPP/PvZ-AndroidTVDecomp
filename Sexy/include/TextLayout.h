// TextLayout lays out a string into lines and draws them.
//
// No base class and no virtual function. The constructors place members through
// the embedded cache image and the two colours; Init, Draw and the host widgets
// also use the three words after that colour cache. mLines is the line vector
// callers already walk through its three-pointer layout.

#pragma once

#include "ReconTypes.h"
#include "Font.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy {

// One colour run on a laid-out line. mGlyphs is what Font::DrawGlyphs draws.
struct TextRun
{
    int mWidth = 0;
    bool mHasColor = false;
    int mRed = 0;
    int mGreen = 0;
    int mBlue = 0;
    int mAlpha = 0;
    std::vector<Glyph> mGlyphs;
};

// One laid-out line: its pixel size, how many glyphs it holds, and the runs.
struct TextLine
{
    int mWidth = 0;
    int mHeight = 0;
    int mGlyphCount = 0;
    std::vector<TextRun> mRuns;
};

class TextLayout
{
public:
    bool         mDirty;              // +0x000
    int          mCachePolicy;        // +0x004
    Sexy::Font  *mFont;               // +0x008
    Sexy::String mString;             // +0x00C
    std::vector<Sexy::TextLine> mLines;
    int          mNumGlyphs;          // +0x01C
    TRect<int>   mRect;               // +0x020
    int          mWidth;              // +0x030
    int          mHeight;             // +0x034
    int          mLineSpacing;        // +0x038
    int          mJustification;      // +0x03C
    bool         mSingleLine;         // +0x040
    bool         mWrap;               // +0x041
    bool         mWrapInSpace;        // +0x042
    bool         mDrawStringUnicode;  // +0x043
    Sexy::MemoryImage mCacheImage;    // +0x044
    Sexy::Color  mCacheBuiltColor;    // +0x12C
    bool         mCacheImageValid;    // +0x13C
    Sexy::Color  mLastDrawColor;      // +0x140
    int          mCacheImageColorizeCount;   // +0x150
    bool         mCacheImageColorizeEnabled; // +0x154
    int          mCacheImageColorizeLimit;   // +0x158

    void Init();
    void SetText(Sexy::String const &theString, bool theDrawStringUnicode);
    void SetFont(Sexy::Font *theFont);
    Sexy::Font *GetFont();
    TRect<int> GetRect() const;
    void SetRect(TRect<int> const &theRect);
    void SetRect(int *theRect);
    void SetWidth(int theWidth);
    void SetHeight(int theHeight);
    void SetCachePolicy(int theCachePolicy);
    int GetCachePolicy() const;
    int FastDrawLine(Sexy::Graphics *theGraphics, Sexy::TextLine &theLine, int theX, int theY,
                     Color const &theColor, int theAlign, TRect<int> const &theClipRect,
                     TRect<int> const &theFullRect);
    int DrawLine(Sexy::Graphics *theGraphics, Sexy::TextLine &theLine, unsigned int theStart,
                 unsigned int theCount, int theX, int theY, Color const &theColor, int theAlign,
                 TRect<int> const &theClipRect);
    void SetLineSpacing(int theLineSpacing);
    int GetLineSpacing() const;
    void SetJustification(int theJustification);
    int GetJustification() const;
    void SetWrap(bool theWrap);
    int GetWrap();
    void SetWrapInSpace(bool theWrapInSpace);
    int GetWrapInSpace();
    void SetSingleLine(bool theSingleLine);
    int GetSingleLine();
    void UpdateCache(Color const &theColor, bool theForce);
    void BuildLines();
    int BuildLine(Sexy::String theString, int theStart, int theLength, bool theDrawStringUnicode,
                  Sexy::TextLine &theLine, bool &theHasColor, Color &theColor);
    TextLayout();
    TextLayout(Sexy::String const &theString, Sexy::Font *theFont, bool theDrawStringUnicode,
               bool theSingleLine);
    ~TextLayout();
    void Update();
    int GetNumGlyphs();
    int GetWidth();
    int GetHeight();
    void Draw(Sexy::Graphics *theGraphics, int theX, int theY, Color const &theColor);
    int DrawGlyphs(Sexy::Graphics *theGraphics, unsigned int theStart, unsigned int theCount,
                   int theX, int theY, Color const &theColor);
    void DrawLines(Sexy::Graphics *theGraphics, unsigned int theStart, unsigned int theCount,
                   int theX, int theY, Color const &theColor);
    void DrawLine(Sexy::Graphics *theGraphics, unsigned int theStart, int theX, int theY,
                  Color const &theColor);
    std::vector<Sexy::TextLine> &GetLines();
    Sexy::TextLine *GetLine(unsigned int theIndex);
    TPoint<int> GetLineExtents(unsigned int theLine);

private:
    void DiscardLines();
};

} // namespace Sexy

using Sexy::TextLayout;

// Font is the base of ImageFont and the system fonts. Metrics, measurement
// and drawing are virtual so a derived font can replace them. Duplicate is
// the copy hook; the base body reports that a bare Font cannot be copied.
//
// The wide overloads take std::wstring. The parameter is a reference, so the
// choice does not change Font's own layout.
//
// After the vptr: ascent, ascent padding, height, line-spacing offset, the
// composited flag IsComposited reads, and a second flag the constructor
// clears and the copy constructor copies. No body reads that second flag yet.

#pragma once

#include "ReconTypes.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy {

class Graphics;

class Font
{
public:
    virtual ~Font();
    virtual int GetAscent();
    virtual int GetAscentPadding();
    virtual int GetDescent();
    virtual int GetHeight();
    virtual int GetLineSpacingOffset();
    virtual int GetLineSpacing();
    virtual int StringWidth(Sexy::String const& theString);
    virtual int StringWidthUnicode(std::wstring const& theString);
    virtual int CharWidth(int theChar);
    virtual int CharWidthKern(int theChar, int thePrevChar);
    virtual void DrawString(Sexy::Graphics * theGraphics, int theX, int theY,
                            Sexy::String const& theString, Sexy::Color const& theColor,
                            Sexy::TRect<int> const& theClipRect);
    virtual void DrawStringUnicode(Sexy::Graphics * theGraphics, int theX, int theY,
                                   std::wstring const& theString, Sexy::Color const& theColor,
                                   Sexy::TRect<int> const& theClipRect);
    virtual void StringToGlyphs(std::wstring const& theText,
                                std::vector<Sexy::Glyph> & theGlyphs);
    virtual void DrawGlyphs(Sexy::Graphics * theGraphics, int theX, int theY,
                            std::vector<Sexy::Glyph> & theGlyphs, unsigned int theStart,
                            unsigned int theCount, Sexy::Color const& theColor,
                            Sexy::TRect<int> const& theClipRect);
    virtual void GlyphExtentsVector(std::vector<Sexy::Glyph> const& theGlyphs,
                                    int & theExtents);
    virtual void GlyphExtentsList(int const& theGlyphsList, int & theExtents);
    virtual void StringExtents(Sexy::String const& theString, int & theExtents);
    virtual void StringExtentsUnicode(std::wstring const& theString, int & theExtents);
    virtual bool IsComposited();
    virtual Sexy::Font * Duplicate();

    int mAscent;
    int mAscentPadding;
    int mHeight;
    int mLineSpacingOffset;
    bool mIsComposited;
    bool mFontFlag;

    Font();
    Font(Sexy::Font const& theOther);
};

} // namespace Sexy

using Sexy::Font;

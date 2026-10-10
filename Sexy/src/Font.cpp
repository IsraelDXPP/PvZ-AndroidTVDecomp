// Font. The base implementations return the stored metrics, ignore drawing,
// and measure a string by turning it into glyphs. A concrete font replaces
// the virtuals that actually paint.

#include "Font.h"
#include "Buffer.h"
#include "Graphics.h"
#include "ReconTypes.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <cstdio>
#include <cstdlib>
#include <cstring>

Sexy::Font::~Font() {}

int Sexy::Font::GetAscent()
{
    return mAscent;
}

int Sexy::Font::GetAscentPadding()
{
    return mAscentPadding;
}

int Sexy::Font::GetDescent()
{
    return mHeight - mAscent;
}

int Sexy::Font::GetHeight()
{
    return mHeight;
}

int Sexy::Font::GetLineSpacingOffset()
{
    return mLineSpacingOffset;
}

int Sexy::Font::GetLineSpacing()
{
    return mLineSpacingOffset + mHeight;
}

// Only a derived override measures anything. The base answers zero.
int Sexy::Font::StringWidth(Sexy::String const &theString)
{
    (void)theString;
    return 0;
}

int Sexy::Font::StringWidthUnicode(std::wstring const &theString)
{
    (void)theString;
    return 0;
}

int Sexy::Font::CharWidthKern(int theChar, int thePrevChar)
{
    (void)thePrevChar;
    return CharWidth(theChar);
}

void Sexy::Font::DrawString(Sexy::Graphics *theGraphics, int theX, int theY, Sexy::String const &theString,
    Sexy::Color const &theColor, Sexy::TRect<int> const &theClipRect)
{
    (void)theGraphics;
    (void)theX;
    (void)theY;
    (void)theString;
    (void)theColor;
    (void)theClipRect;
}

void Sexy::Font::DrawStringUnicode(Sexy::Graphics *theGraphics, int theX, int theY, std::wstring const &theString,
    Sexy::Color const &theColor, Sexy::TRect<int> const &theClipRect)
{
    (void)theGraphics;
    (void)theX;
    (void)theY;
    (void)theString;
    (void)theColor;
    (void)theClipRect;
}

void Sexy::Font::StringToGlyphs(std::wstring const &theText, std::vector<Sexy::Glyph> &theGlyphs)
{
    (void)theText;
    (void)theGlyphs;
}

void Sexy::Font::DrawGlyphs(Sexy::Graphics *theGraphics, int theX, int theY, std::vector<Sexy::Glyph> &theGlyphs,
    unsigned int theStart, unsigned int theCount, Sexy::Color const &theColor, Sexy::TRect<int> const &theClipRect)
{
    (void)theGraphics;
    (void)theX;
    (void)theY;
    (void)theGlyphs;
    (void)theStart;
    (void)theCount;
    (void)theColor;
    (void)theClipRect;
}

bool Sexy::Font::IsComposited()
{
    return mIsComposited;
}

void Sexy::Font::GlyphExtentsList(std::list<const Glyph *> const &theGlyphsList, TextExtents &theExtents)
{
    (void)theGlyphsList;
    theExtents = TextExtents();
}

void Sexy::Font::StringExtents(Sexy::String const &theString, TextExtents &theExtents)
{
    Buffer aText;
    aText.mData.assign(theString.data(), theString.data() + theString.size());
    aText.mDataBitSize = (int)aText.mData.size() * 8;
    StringExtentsUnicode(aText.UTF8ToWideString(), theExtents);
}

int Sexy::Font::CharWidth(int theChar)
{
    char text[1];
    text[0] = static_cast<char>(theChar);
    return StringWidth(Sexy::String(text, 1u));
}

void Sexy::Font::GlyphExtentsVector(std::vector<Sexy::Glyph> const &theGlyphs, TextExtents &theExtents)
{
    std::list<const Glyph *> aGlyphList;
    for (const Glyph &aGlyph : theGlyphs)
        aGlyphList.push_back(&aGlyph);
    GlyphExtentsList(aGlyphList, theExtents);
}

void Sexy::Font::StringExtentsUnicode(std::wstring const &theString, TextExtents &theExtents)
{
    theExtents = TextExtents();
    if (theString.empty())
        return;

    std::vector<Sexy::Glyph> glyphs;
    StringToGlyphs(theString, glyphs);
    GlyphExtentsVector(glyphs, theExtents);
}

// A bare Font has no copy. Derived fonts replace this.
Sexy::Font *Sexy::Font::Duplicate()
{
    fprintf(stderr, "pure virtual method called\n");
    fflush(stderr);
    abort();
    return nullptr;
}

Sexy::Font::Font()
{
    mAscent = 0;
    mAscentPadding = 0;
    mHeight = 0;
    mLineSpacingOffset = 0;
    mIsComposited = false;
    mFontFlag = false;
}

Sexy::Font::Font(Sexy::Font const &theOther)
{
    mAscent = theOther.mAscent;
    mAscentPadding = theOther.mAscentPadding;
    mHeight = theOther.mHeight;
    mLineSpacingOffset = theOther.mLineSpacingOffset;
    mIsComposited = theOther.mIsComposited;
    mFontFlag = theOther.mFontFlag;
}

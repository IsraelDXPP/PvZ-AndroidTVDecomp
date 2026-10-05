// Font. The base implementations return the stored metrics, ignore drawing,
// and measure a string by turning it into glyphs. A concrete font replaces
// the virtuals that actually paint.

#include "Font.h"
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

namespace {

void ClearExtents(int & theExtents)
{
  int *extents = &theExtents;
  extents[0] = 0;
  extents[1] = 0;
  extents[2] = 0;
  extents[3] = 0;
  extents[4] = 0;
  extents[5] = 0;
}

} // namespace

Sexy::Font::~Font()
{
}

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
int Sexy::Font::StringWidth(Sexy::String const& theString)
{
  (void)theString;
  return 0;
}

int Sexy::Font::StringWidthUnicode(std::wstring const& theString)
{
  (void)theString;
  return 0;
}

int Sexy::Font::CharWidthKern(int theChar, int thePrevChar)
{
  (void)thePrevChar;
  return CharWidth(theChar);
}

void Sexy::Font::DrawString(Sexy::Graphics * theGraphics, int theX, int theY,
                            Sexy::String const& theString, Sexy::Color const& theColor,
                            Sexy::TRect<int> const& theClipRect)
{
  (void)theGraphics;
  (void)theX;
  (void)theY;
  (void)theString;
  (void)theColor;
  (void)theClipRect;
}

void Sexy::Font::DrawStringUnicode(Sexy::Graphics * theGraphics, int theX, int theY,
                                   std::wstring const& theString, Sexy::Color const& theColor,
                                   Sexy::TRect<int> const& theClipRect)
{
  (void)theGraphics;
  (void)theX;
  (void)theY;
  (void)theString;
  (void)theColor;
  (void)theClipRect;
}

void Sexy::Font::StringToGlyphs(std::wstring const& theText,
                                std::vector<Sexy::Glyph> & theGlyphs)
{
  (void)theText;
  (void)theGlyphs;
}

void Sexy::Font::DrawGlyphs(Sexy::Graphics * theGraphics, int theX, int theY,
                            std::vector<Sexy::Glyph> & theGlyphs, unsigned int theStart,
                            unsigned int theCount, Sexy::Color const& theColor,
                            Sexy::TRect<int> const& theClipRect)
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

void Sexy::Font::GlyphExtentsList(int const& theGlyphsList, int & theExtents)
{
  (void)theGlyphsList;
  ClearExtents(theExtents);
}

void Sexy::Font::StringExtents(Sexy::String const& theString, int & theExtents)
{
  // Widen, then measure through the wide overload. Sexy::String is the
  // one-word string this port uses for that widened copy.
  Sexy::String wide;
  if (theString.size() != 0)
    StringToWString(&wide, theString.c_str());

  const std::wstring &wideText = *static_cast<const std::wstring *>(static_cast<const void *>(&wide));
  StringExtentsUnicode(wideText, theExtents);
}

int Sexy::Font::CharWidth(int theChar)
{
  char text[1];
  text[0] = static_cast<char>(theChar);
  return StringWidth(Sexy::String(text, 1u));
}

void Sexy::Font::GlyphExtentsVector(std::vector<Sexy::Glyph> const& theGlyphs,
                                    int & theExtents)
{
  std::list<Sexy::Glyph const *> glyphList;
  for (unsigned int i = 0; i < theGlyphs.size(); ++i)
    glyphList.push_back(&theGlyphs[i]);

  // The list overload is declared against an int reference because callers
  // hold the glyph list that way. The list object is the same argument.
  const int &glyphListArg = *static_cast<int const *>(static_cast<const void *>(&glyphList));
  GlyphExtentsList(glyphListArg, theExtents);
}

void Sexy::Font::StringExtentsUnicode(std::wstring const& theString, int & theExtents)
{
  ClearExtents(theExtents);

  const Sexy::String &wide = *static_cast<const Sexy::String *>(static_cast<const void *>(&theString));
  if (wide.size() == 0)
    return;

  std::vector<Sexy::Glyph> glyphs;
  StringToGlyphs(theString, glyphs);
  GlyphExtentsVector(glyphs, theExtents);
}

// A bare Font has no copy. Derived fonts replace this.
Sexy::Font * Sexy::Font::Duplicate()
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

Sexy::Font::Font(Sexy::Font const& theOther)
{
  mAscent = theOther.mAscent;
  mAscentPadding = theOther.mAscentPadding;
  mHeight = theOther.mHeight;
  mLineSpacingOffset = theOther.mLineSpacingOffset;
  mIsComposited = theOther.mIsComposited;
  mFontFlag = theOther.mFontFlag;
}

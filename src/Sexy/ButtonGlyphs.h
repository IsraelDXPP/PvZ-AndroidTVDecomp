// Console button tags. The TV binary replaces <A>/<B>/<C>/<X> with translated
// captions when no pad is connected (TodReplaceButtons @0x1FD454). The PS3
// host keeps the tag and, at draw time, paints the matching cel of
// IMAGE_HELP_BUTTONS the same way that caption swap substitutes text.
//
// help_buttons.png is 13 columns of 42x42:
//   0 A, 1 B, 2 X, 3 Y, 4 Start, 5 RB, 6 LB, 7 RT, 8 LT, 9 d-pad,
//   10 L, 11 R, 12 stick.

#pragma once

#include "SexyString.h"

namespace Sexy {
class Font;
class Graphics;
class Image;
}

// True when a connected pad should draw icons instead of captions.
bool ButtonGlyphsActive();

// Cel index of a tag such as "<A>" or "<LT>", or -1 when the bytes are not a tag.
int ButtonGlyphCel(const char* theTag, int theLength);

// The label already reserves the tag's own advance. Replace those letters
// with spaces of the same width so the icon sits in that gap.
Sexy::String ButtonTagAsSpaces(Sexy::Font* theFont, const char* theTag, int theLength);

// Paint one icon. theY is the text baseline Graphics::DrawString already uses.
// theSlotWidth is the advance the spaces occupy; the cel is centered in it.
void DrawButtonGlyph(Sexy::Graphics* theGraphics, int theX, int theY, Sexy::Font* theFont,
                     int theCel, int theSlotWidth);

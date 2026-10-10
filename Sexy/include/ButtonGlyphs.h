// Console button tags. A pad found at startup keeps <A> and Graphics draws
// the matching cel of IMAGE_HELP_BUTTONS. No pad, TodReplaceButtons swaps
// <A> <B> <C> <X> for the language-table strings.
//
// help_buttons.png is 13 columns of 42x42:
//   0 A, 1 B, 2 X, 3 Y, 4 Start, 5 RB, 6 LB, 7 d-pad, 8 RT, 9 LT,
//   10 Back, 11 right stick, 12 left stick.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_BUTTONGLYPHS_H
#define LAWNPROJECTTV_SEXY_INCLUDE_BUTTONGLYPHS_H

#include "SexyString.h"

namespace Sexy
{
class Font;
class Graphics;
class Image;
}

// True when startup found a pad and LAWN_GAMEPAD_MODE is on.
bool ButtonGlyphsActive();

// Cel index of a tag such as "<A>" or "<LT>", or -1 when the bytes are not a tag.
int ButtonGlyphCel(const char *theTag, int theLength);

// The label already reserves the tag's own advance. Replace those letters
// with spaces of the same width so the icon sits in that gap.
Sexy::String ButtonTagAsSpaces(Sexy::Font *theFont, const char *theTag, int theLength);

// Paint one icon. theY is the text baseline Graphics::DrawString already uses.
// theSlotWidth is the advance the spaces occupy; the cel is centered in it.
void DrawButtonGlyph(
    Sexy::Graphics *theGraphics, int theX, int theY, Sexy::Font *theFont, int theCel, int theSlotWidth);

#endif // LAWNPROJECTTV_SEXY_INCLUDE_BUTTONGLYPHS_H

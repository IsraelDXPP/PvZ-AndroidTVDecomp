// Console button tags. A pad found at startup keeps <A> and Graphics draws
// the matching cel of IMAGE_HELP_BUTTONS. No pad, or LAWN_GAMEPAD_MODE off,
// replaces the tag with the keyboard key from setup.env. The table is read
// once; pads are not watched for a later connect.
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

// True when startup found a pad and LAWN_GAMEPAD_MODE is on.
bool ButtonGlyphsActive();

// First keyboard stand-in for a tag such as "<A>". Empty when the table has no entry.
Sexy::String ButtonKeyCaption(const char* theTag, int theLength);

// Every keyboard stand-in for a tag. A button can have more than one key.
int ButtonKeyCaptions(const char* theTag, int theLength, const char** theKeys, int theMax);

// True when theKey is the keyboard stand-in for the named pad button.
bool ButtonKeyMatches(const char* theButton, int theKey);

// Cel index of a tag such as "<A>" or "<LT>", or -1 when the bytes are not a tag.
int ButtonGlyphCel(const char* theTag, int theLength);

// The label already reserves the tag's own advance. Replace those letters
// with spaces of the same width so the icon sits in that gap.
Sexy::String ButtonTagAsSpaces(Sexy::Font* theFont, const char* theTag, int theLength);

// Paint one icon. theY is the text baseline Graphics::DrawString already uses.
// theSlotWidth is the advance the spaces occupy; the cel is centered in it.
void DrawButtonGlyph(Sexy::Graphics* theGraphics, int theX, int theY, Sexy::Font* theFont,
                     int theCel, int theSlotWidth);

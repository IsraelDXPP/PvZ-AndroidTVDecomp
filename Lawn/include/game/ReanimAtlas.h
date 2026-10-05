// ReanimAtlas -- the packed image table a ReanimatorDefinition points at.
//
// GetEncodedReanimAtlas answers `this + 20 * (id - 1)` for an id in 1..1000, so
// the table is 64 entries of 20 bytes. The packed image sits at +0x504 and the
// live count at +0x500. ReanimAtlasDispose drops the packed image, then clears
// the count.

#pragma once

#include "Image.h"

class ReanimAtlasImage
{
public:
    int          mX;             // +0x00
    int          mY;             // +0x04
    int          mWidth;         // +0x08
    int          mHeight;        // +0x0C
    Sexy::Image *mOriginalImage; // +0x10
};


class ReanimAtlas
{
public:
    ReanimAtlasImage *GetEncodedReanimAtlas(Sexy::Image *theImage);
    void ReanimAtlasDispose();

    ReanimAtlasImage mAtlasImages[64]; // +0x0000
    int              mNumAtlasImages;  // +0x0500
    Sexy::Image     *mImage;           // +0x0504
};


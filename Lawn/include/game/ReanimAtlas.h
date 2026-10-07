// Packed animation images are matched by their original Image pointer.

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
    ReanimAtlasImage *FindAtlasImage(Sexy::Image *theImage);
    void ReanimAtlasDispose();

    ReanimAtlasImage mAtlasImages[64]; // +0x0000
    int              mNumAtlasImages;  // +0x0500
    Sexy::Image     *mImage;           // +0x0504
};


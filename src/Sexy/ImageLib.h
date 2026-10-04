// ImageLib is the framework's image encoder and decoder. The writers report
// failure rather than producing a file: this port replaces the platform
// encoders, and the game only checks whether the call succeeded.

#pragma once

#include "ReconTypes.h"

namespace ImageLib {

class Image
{
public:
    int mOwnsBits;
    int mWidth;
    int mHeight;
    unsigned int * mBits;
    void * mText1;
    void * mText2;
    bool mPalettized;
    int mPalette[256];

    Image();
    ~Image();

    int GetWidth() { return mWidth; }
    int GetHeight() { return mHeight; }
    unsigned int * GetBits();
};

char * WritePNGImage(const Sexy::String& thePath, Image * thePixels);
char * WriteJPEGImage(const Sexy::String& thePath, Image * thePixels);

// When this is off, GetImage ignores the caller's alpha-mask request.
extern bool gAutoLoadAlpha;
extern unsigned int gAlphaComposeColor;

// Path may omit the extension. The search order is the binary's:
// .tga, .jpg, .png, .gif. A trailing "_" file, when present, supplies alpha.
Image* GetImage(Sexy::String const& thePath, bool lookForAlpha, bool thePurge, bool theFlags);
void InitJPEG2000(void * theContext);

} // namespace ImageLib

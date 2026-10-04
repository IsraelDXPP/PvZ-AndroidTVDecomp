// Pixel format helpers. Every method is a pure function of its arguments.
// The format table is the binary's 11 records at 0x66D5A0: name, bytes, alpha.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

enum
{
    PIXEL_FORMAT_NONE     = 0,
    PIXEL_FORMAT_LUT8     = 1,
    PIXEL_FORMAT_A8R8G8B8 = 2,
    PIXEL_FORMAT_A4R4G4B4 = 3,
    PIXEL_FORMAT_A1R5G5B5 = 4,
    PIXEL_FORMAT_R5G6R5   = 5,
    PIXEL_FORMAT_A8B8G8R8 = 6,
    PIXEL_FORMAT_R4G4B4A4 = 7,
    PIXEL_FORMAT_R5G5B5A1 = 8,
    PIXEL_FORMAT_X8R8G8B8 = 9,
    PIXEL_FORMAT_L8A8     = 10
};

class PixelUtil
{
public:
    static int GetBytePerPixel(PixelFormat theFormat);
    static int HasAlpha(PixelFormat theFormat);
    static const char* GetPixelFormatName(PixelFormat theFormat);
    static int GetPixelFormat(const Sexy::String& theName);

    // Components are the bytes of an A8R8G8B8 word: alpha, red, green, blue.
    static unsigned int PackPixel(PixelFormat theFormat,
                                  unsigned char theAlpha,
                                  unsigned char theRed,
                                  unsigned char theGreen,
                                  unsigned char theBlue);
    // thePixel is an A8R8G8B8 word. Format 2 is returned unchanged.
    static unsigned int PackPixel(PixelFormat theFormat, unsigned int thePixel);
    // Packs thePixel and stores it at theDest when the format has a known width.
    static unsigned int PackPixel(PixelFormat theFormat, unsigned int thePixel, void* theDest);

    // Writes the four channel bytes the format stores, in the order the binary
    // writes them (alpha, red, green, blue for A8R8G8B8).
    static void UnpackPixel(PixelFormat theFormat, unsigned int thePixel,
                            unsigned char* theAlpha, unsigned char* theRed,
                            unsigned char* theGreen, unsigned char* theBlue);
    // Expands a packed pixel to an A8R8G8B8 word.
    static int UnpackPixel(PixelFormat theFormat, unsigned int thePixel);
    // Reads one pixel from theBuffer (halfword or word, by format) and expands it.
    static int UnpackPixel(PixelFormat theFormat, const void* theBuffer);

    static unsigned int MultiplyPixelNoTrans(unsigned int thePixel);
    static unsigned int MultiplyPixelNoAlpha(unsigned int thePixel);
    static unsigned int MultiplyPixel(unsigned int thePixel);
    static unsigned int MultiplyPixelNoTransRev(unsigned int thePixel);
    static unsigned int MultiplyPixelNoAlphaRev(unsigned int thePixel);
    static unsigned int MultiplyPixelRev(unsigned int thePixel);
};

} // namespace Sexy

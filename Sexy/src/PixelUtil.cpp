#include "PixelUtil.h"
#include "ReconHelpers.h"

#include <cstring>

namespace
{

struct PixelFormatRecord
{
    const char *mName;
    int mBytesPerPixel;
    int mHasAlpha;
};

// Names, widths, and alpha flags copied from the table at 0x66D5A0.
// LUT8's alpha flag is 1. The 16-bit 565 name is the binary's "R5G6R5".
const PixelFormatRecord kFormatTable[] = {
    {"NONE", 0, 0},
    {"LUT8", 1, 1},
    {"A8R8G8B8", 4, 1},
    {"A4R4G4B4", 2, 1},
    {"A1R5G5B5", 2, 1},
    {"R5G6R5", 2, 0},
    {"A8B8G8R8", 4, 1},
    {"R4G4B4A4", 2, 1},
    {"R5G5B5A1", 2, 1},
    {"X8R8G8B8", 4, 0},
    {"L8A8", 2, 1},
};

const int kFormatCount = 11;

bool FormatInRange(int theFormat)
{
    return theFormat >= 0 && theFormat < kFormatCount;
}

unsigned int HighByte(unsigned int theValue)
{
    return theValue >> 24;
}

unsigned int MidByte(unsigned int theValue)
{
    return (theValue >> 16) & 0xFFu;
}

unsigned int LowMidByte(unsigned int theValue)
{
    return (theValue >> 8) & 0xFFu;
}

unsigned int LowByte(unsigned int theValue)
{
    return theValue & 0xFFu;
}

unsigned int ExpandNibble(unsigned int theNibble)
{
    unsigned int aNibble = theNibble & 0xFu;
    return aNibble | (aNibble << 4);
}

unsigned int Expand5(unsigned int theField)
{
    unsigned int aField = theField & 0x1Fu;
    return (aField << 3) | (aField >> 2);
}

unsigned int Expand6(unsigned int theField)
{
    unsigned int aField = theField & 0x3Fu;
    return (aField << 2) | (aField >> 4);
}

// 5-bit field to 8 bits, matching the shifts at 0x3C74FE.
int SignBit(unsigned int thePixel, int theBit)
{
    return ((int)(thePixel << (31 - theBit)) >> 31) & 0xFF;
}

// Premultiply an A8R8G8B8 word by its own alpha. The paired red/blue multiply
// and the +128 rounding are the sequence at 0x3C76D0.
unsigned int PremultiplyArgb(unsigned int thePixel)
{
    unsigned int aAlpha = HighByte(thePixel);
    unsigned int aRedBlue = thePixel & 0x00FF00FFu;
    unsigned int aGreen = LowMidByte(thePixel);
    unsigned int aScaledRb = aAlpha * aRedBlue + 0x800080u;
    unsigned int aScaledGreen = aAlpha * aGreen + 128u;
    unsigned int aRb = ((aScaledRb + ((aScaledRb >> 8) & 0x00FF00FFu)) >> 8) & 0x00FF00FFu;
    unsigned int aGreenOut = (aScaledGreen + (aScaledGreen >> 8)) & ~0xFFu;
    return aRb | aGreenOut | (aAlpha << 24);
}

unsigned int SwapRedBlue(unsigned int thePixel)
{
    return (thePixel & 0xFF00FF00u) | ((thePixel & 0xFFu) << 16) | ((thePixel >> 16) & 0xFFu);
}

} // namespace

int Sexy::PixelUtil::GetBytePerPixel(Sexy::PixelFormat theFormat)
{
    if (!FormatInRange(theFormat))
        return 0;
    return kFormatTable[theFormat].mBytesPerPixel;
}

int Sexy::PixelUtil::HasAlpha(Sexy::PixelFormat theFormat)
{
    if (!FormatInRange(theFormat))
        return 0;
    return kFormatTable[theFormat].mHasAlpha;
}

const char *Sexy::PixelUtil::GetPixelFormatName(Sexy::PixelFormat theFormat)
{
    if (!FormatInRange(theFormat))
        return "";
    return kFormatTable[theFormat].mName;
}

int Sexy::PixelUtil::GetPixelFormat(const Sexy::String &theName)
{
    if (theName.c_str() == nullptr)
        return PIXEL_FORMAT_NONE;

    Sexy::String aUpper;
    StringToUpper(&aUpper, theName.c_str());
    for (int aFormat = 0; aFormat < kFormatCount; ++aFormat)
    {
        if (strcmp(aUpper.c_str(), kFormatTable[aFormat].mName) == 0)
            return aFormat;
    }
    return PIXEL_FORMAT_NONE;
}

unsigned int Sexy::PixelUtil::PackPixel(Sexy::PixelFormat theFormat, unsigned char theAlpha, unsigned char theRed,
    unsigned char theGreen, unsigned char theBlue)
{
    switch ((int)theFormat)
    {
        case PIXEL_FORMAT_A8R8G8B8:
            return theBlue | (theGreen << 8) | (theRed << 16) | (theAlpha << 24);

        case PIXEL_FORMAT_A4R4G4B4:
            return ((theAlpha >> 4) << 12) | ((theRed >> 4) << 8) | (theGreen & 0xF0u) | (theBlue >> 4);

        case PIXEL_FORMAT_A1R5G5B5:
            return (theBlue >> 3) | ((theGreen >> 3) << 5) | ((theRed >> 3) << 10) | ((theAlpha >> 7) << 15);

        case PIXEL_FORMAT_R5G6R5:
            return (theBlue >> 3) | ((theGreen >> 2) << 5) | ((theRed >> 3) << 11);

        case PIXEL_FORMAT_A8B8G8R8:
            return theRed | (theGreen << 8) | (theBlue << 16) | (theAlpha << 24);

        case PIXEL_FORMAT_R4G4B4A4:
            return (theBlue & 0xF0u) | ((theGreen >> 4) << 8) | ((theRed >> 4) << 12) | (theAlpha >> 4);

        case PIXEL_FORMAT_R5G5B5A1:
            return ((theBlue >> 3) << 1) | ((theGreen >> 3) << 6) | ((theRed >> 3) << 11) | (theAlpha >> 7);

        default:
            return 0;
    }
}

unsigned int Sexy::PixelUtil::PackPixel(Sexy::PixelFormat theFormat, unsigned int thePixel)
{
    if ((int)theFormat == PIXEL_FORMAT_A8R8G8B8)
        return thePixel;

    // 0x3C7446 splits an A8R8G8B8 word into alpha, red, green, blue.
    return PackPixel(theFormat,
        (unsigned char)HighByte(thePixel),
        (unsigned char)MidByte(thePixel),
        (unsigned char)LowMidByte(thePixel),
        (unsigned char)LowByte(thePixel));
}

unsigned int Sexy::PixelUtil::PackPixel(Sexy::PixelFormat theFormat, unsigned int thePixel, void *theDest)
{
    unsigned int aPacked = PackPixel(theFormat, thePixel);
    int aFormat = theFormat;
    if ((unsigned int)(aFormat - 2) > 6u || theDest == nullptr)
        return aPacked;

    int aWidthBit = 1 << (aFormat - 2);
    if ((aWidthBit & 0x6E) != 0)
        *static_cast<unsigned short *>(theDest) = (unsigned short)aPacked;
    else if ((aWidthBit & 0x11) != 0)
        *static_cast<unsigned int *>(theDest) = aPacked;
    return aPacked;
}

void Sexy::PixelUtil::UnpackPixel(Sexy::PixelFormat theFormat, unsigned int thePixel, unsigned char *theAlpha,
    unsigned char *theRed, unsigned char *theGreen, unsigned char *theBlue)
{
    unsigned int aAlpha = 0;
    unsigned int aRed = 0;
    unsigned int aGreen = 0;
    unsigned int aBlue = 0;
    unsigned int aLow = thePixel & 0xFFFFu;

    switch ((int)theFormat)
    {
        case PIXEL_FORMAT_A8R8G8B8:
            aAlpha = HighByte(thePixel);
            aRed = MidByte(thePixel);
            aGreen = LowMidByte(thePixel);
            aBlue = LowByte(thePixel);
            break;

        case PIXEL_FORMAT_A4R4G4B4:
            aAlpha = ExpandNibble(thePixel >> 12);
            aRed = ExpandNibble(thePixel >> 8);
            aGreen = ExpandNibble(thePixel >> 4);
            aBlue = ExpandNibble(thePixel);
            break;

        case PIXEL_FORMAT_A1R5G5B5:
            aRed = Expand5(thePixel >> 10);
            aGreen = Expand5(thePixel >> 5);
            aBlue = Expand5(thePixel);
            aAlpha = (unsigned int)SignBit(aLow, 15);
            break;

        case PIXEL_FORMAT_R5G6R5:
            aRed = Expand5(aLow >> 11);
            aGreen = Expand6(thePixel >> 5);
            aBlue = Expand5(thePixel);
            aAlpha = 0xFFu;
            break;

        case PIXEL_FORMAT_A8B8G8R8:
            aAlpha = HighByte(thePixel);
            aRed = LowByte(thePixel);
            aGreen = LowMidByte(thePixel);
            aBlue = MidByte(thePixel);
            break;

        case PIXEL_FORMAT_R4G4B4A4:
            aRed = ExpandNibble(thePixel >> 12);
            aGreen = ExpandNibble(thePixel >> 8);
            aBlue = ExpandNibble(LowByte(thePixel) >> 4);
            aAlpha = ExpandNibble(thePixel);
            break;

        case PIXEL_FORMAT_R5G5B5A1:
            aRed = Expand5(aLow >> 11);
            aGreen = Expand5(thePixel >> 6);
            aBlue = Expand5(thePixel >> 1);
            aAlpha = (unsigned int)SignBit(thePixel, 0);
            break;

        default:
            break;
    }

    if (theAlpha != nullptr)
        *theAlpha = (unsigned char)aAlpha;
    if (theRed != nullptr)
        *theRed = (unsigned char)aRed;
    if (theGreen != nullptr)
        *theGreen = (unsigned char)aGreen;
    if (theBlue != nullptr)
        *theBlue = (unsigned char)aBlue;
}

int Sexy::PixelUtil::UnpackPixel(Sexy::PixelFormat theFormat, unsigned int thePixel)
{
    switch ((int)theFormat)
    {
        case PIXEL_FORMAT_NONE:
        case PIXEL_FORMAT_LUT8:
        case 11:
            return 0;

        case PIXEL_FORMAT_A8R8G8B8:
            return (int)thePixel;

        case PIXEL_FORMAT_A4R4G4B4:
        {
            unsigned int aRed = ExpandNibble(thePixel >> 8);
            unsigned int aAlpha = ExpandNibble((thePixel & 0xFFFFu) >> 12);
            unsigned int aGreen = ExpandNibble(LowByte(thePixel) >> 4);
            unsigned int aBlue = ExpandNibble(thePixel);
            return (int)(aBlue | (aAlpha << 24) | (aRed << 16) | (aGreen << 8));
        }

        case PIXEL_FORMAT_A1R5G5B5:
        {
            unsigned int aRed = Expand5(thePixel >> 10);
            unsigned int aGreen = Expand5(thePixel >> 5);
            unsigned int aBlue = Expand5(thePixel);
            unsigned int aAlpha = (unsigned int)SignBit(thePixel & 0xFFFFu, 15);
            return (int)(aBlue | (aAlpha << 24) | (aRed << 16) | (aGreen << 8));
        }

        case PIXEL_FORMAT_R5G6R5:
        {
            unsigned int aGreen = Expand6(thePixel >> 5);
            unsigned int aRed = Expand5((thePixel & 0xFFFFu) >> 11);
            unsigned int aBlue = Expand5(thePixel);
            return (int)(aBlue | 0xFF000000u | (aRed << 16) | (aGreen << 8));
        }

        case PIXEL_FORMAT_A8B8G8R8:
        {
            unsigned int aRed = LowByte(thePixel);
            unsigned int aGreen = LowMidByte(thePixel);
            unsigned int aBlue = MidByte(thePixel);
            unsigned int aAlpha = thePixel & 0xFF000000u;
            return (int)(aBlue | aAlpha | (aRed << 16) | (aGreen << 8));
        }

        case PIXEL_FORMAT_R4G4B4A4:
        {
            unsigned int aRed = ExpandNibble((thePixel & 0xFFFFu) >> 12);
            unsigned int aAlpha = ExpandNibble(thePixel);
            unsigned int aGreen = ExpandNibble(thePixel >> 8);
            unsigned int aBlue = ExpandNibble(LowByte(thePixel) >> 4);
            return (int)(aBlue | (aAlpha << 24) | (aRed << 16) | (aGreen << 8));
        }

        case PIXEL_FORMAT_R5G5B5A1:
        {
            unsigned int aRed = Expand5((thePixel & 0xFFFFu) >> 11);
            unsigned int aGreen = Expand5(thePixel >> 6);
            unsigned int aBlue = Expand5(thePixel >> 1);
            unsigned int aAlpha = (unsigned int)SignBit(thePixel, 0);
            return (int)(aBlue | (aAlpha << 24) | (aRed << 16) | (aGreen << 8));
        }

        default:
            return 0;
    }
}

int Sexy::PixelUtil::UnpackPixel(Sexy::PixelFormat theFormat, const void *theBuffer)
{
    int aFormat = theFormat;
    if (theBuffer == nullptr || aFormat > 11)
        return 0;

    unsigned int aMask = 1u << aFormat;
    if ((aMask & 0x803u) != 0)
        return 0;

    if ((aMask & 0x1B8u) != 0)
        return UnpackPixel(theFormat, (unsigned int)*static_cast<const unsigned short *>(theBuffer));

    if ((aMask & 0x44u) != 0)
        return UnpackPixel(theFormat, *static_cast<const unsigned int *>(theBuffer));

    return 0;
}

unsigned int Sexy::PixelUtil::MultiplyPixelNoTrans(unsigned int thePixel)
{
    return PremultiplyArgb(thePixel);
}

unsigned int Sexy::PixelUtil::MultiplyPixelNoAlpha(unsigned int thePixel)
{
    if (HighByte(thePixel) == 0)
        return 0;
    return thePixel;
}

unsigned int Sexy::PixelUtil::MultiplyPixel(unsigned int thePixel)
{
    if (thePixel == 0)
        return 0;
    return PremultiplyArgb(thePixel);
}

unsigned int Sexy::PixelUtil::MultiplyPixelNoTransRev(unsigned int thePixel)
{
    return SwapRedBlue(PremultiplyArgb(thePixel));
}

unsigned int Sexy::PixelUtil::MultiplyPixelNoAlphaRev(unsigned int thePixel)
{
    if (HighByte(thePixel) == 0)
        return 0;
    return SwapRedBlue(thePixel);
}

unsigned int Sexy::PixelUtil::MultiplyPixelRev(unsigned int thePixel)
{
    if (thePixel == 0)
        return 0;
    return SwapRedBlue(PremultiplyArgb(thePixel));
}

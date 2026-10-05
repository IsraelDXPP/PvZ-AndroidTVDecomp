#include "PixelData.h"

#include <cstring>

namespace
{

const unsigned int kNibbleRound = 526344u;

unsigned int ReadWord(const unsigned char* theBytes)
{
    unsigned int aWord = 0;
    memcpy(&aWord, theBytes, sizeof(aWord));
    return aWord;
}

unsigned short ReadHalf(const unsigned char* theBytes)
{
    unsigned short aHalf = 0;
    memcpy(&aHalf, theBytes, sizeof(aHalf));
    return aHalf;
}

void WriteHalf(unsigned char* theBytes, unsigned short theValue)
{
    memcpy(theBytes, &theValue, sizeof(theValue));
}

void WriteWord(unsigned char* theBytes, unsigned int theValue)
{
    memcpy(theBytes, &theValue, sizeof(theValue));
}

unsigned int ScaleNibbles(unsigned int theAlphaNibble, unsigned int theChannels)
{
    unsigned int aScaled = theAlphaNibble * theChannels + kNibbleRound;
    return (((aScaled >> 4) & 0xF0F0Fu) + aScaled) >> 4;
}

} // namespace

int Sexy::PixelData::Copy(const Sexy::PixelData& theSource)
{
    if (theSource.mWidth != mWidth || theSource.mHeight != mHeight || theSource.mFormat != mFormat)
        return 0;

    int aRowBytes = mWidth * PixelUtil::GetBytePerPixel(theSource.mFormat);
    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        memcpy(mBits + aRow * mPitch,
               theSource.mBits + aRow * theSource.mPitch,
               (size_t)aRowBytes);
    }
    return 1;
}

int Sexy::PixelData::ConvertA8R8G8B8ToA1R5G5B5(const Sexy::PixelData& theSource)
{
    if (theSource.mFormat != PIXEL_FORMAT_A8R8G8B8 || mFormat != PIXEL_FORMAT_A1R5G5B5)
        return 0;

    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            unsigned int aPixel = ReadWord(aSrc + aCol * 4);
            unsigned short aPacked = (unsigned short)(((aPixel & 0xFFu) >> 3)
                                    | ((aPixel & 0x80000000u) >> 16)
                                    | ((aPixel & 0xF80000u) >> 9)
                                    | ((unsigned short)(aPixel & 0xF800u) >> 6));
            WriteHalf(aDst + aCol * 2, aPacked);
        }
    }
    return 1;
}

int Sexy::PixelData::ConvertA8R8G8B8ToR5G6B5(const Sexy::PixelData& theSource)
{
    if (theSource.mFormat != PIXEL_FORMAT_A8R8G8B8 || mFormat != PIXEL_FORMAT_R5G6R5)
        return 0;

    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            unsigned int aPixel = ReadWord(aSrc + aCol * 4);
            unsigned short aPacked = (unsigned short)(((aPixel & 0xFFu) >> 3)
                                    | ((aPixel & 0xF80000u) >> 8)
                                    | ((unsigned short)(aPixel & 0xFC00u) >> 5));
            WriteHalf(aDst + aCol * 2, aPacked);
        }
    }
    return 1;
}

int Sexy::PixelData::ConvertA8R8G8B8ToA4R4G4B4(const Sexy::PixelData& theSource)
{
    if (theSource.mFormat != PIXEL_FORMAT_A8R8G8B8 || mFormat != PIXEL_FORMAT_A4R4G4B4)
        return 0;

    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            unsigned int aPixel = ReadWord(aSrc + aCol * 4);
            unsigned short aPacked = (unsigned short)(((aPixel & 0xFFu) >> 4)
                                    | ((aPixel & 0xF0000000u) >> 16)
                                    | ((aPixel & 0xF00000u) >> 12)
                                    | ((unsigned short)(aPixel & 0xF000u) >> 8));
            WriteHalf(aDst + aCol * 2, aPacked);
        }
    }
    return 1;
}

int Sexy::PixelData::Convert(const Sexy::PixelData& theSource)
{
    if (theSource.mWidth != mWidth || theSource.mHeight != mHeight)
        return 0;

    if (theSource.mFormat == mFormat)
        return Copy(theSource);

    if (ConvertA8R8G8B8ToA4R4G4B4(theSource) != 0
        || ConvertA8R8G8B8ToR5G6B5(theSource) != 0
        || ConvertA8R8G8B8ToA1R5G5B5(theSource) != 0)
        return 1;

    int aSrcPixelSize = PixelUtil::GetBytePerPixel(theSource.mFormat);
    int aDstPixelSize = PixelUtil::GetBytePerPixel(mFormat);
    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            int aPixel = PixelUtil::UnpackPixel(theSource.mFormat, aSrc);
            PixelUtil::PackPixel(mFormat, (unsigned int)aPixel, aDst);
            aSrc += aSrcPixelSize;
            aDst += aDstPixelSize;
        }
    }
    return 1;
}

int Sexy::PixelData::ConvertAndPremulA4R4G4B4ToA4R4G4B4(const Sexy::PixelData& theSource)
{
    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            unsigned int aPixel = ReadHalf(aSrc + aCol * 2);
            unsigned int aChannels = (aPixel & 0xFu)
                                   | (((aPixel & 0xFFu) >> 4) << 8)
                                   | (((aPixel >> 8) & 0xFu) << 16);
            unsigned int aNibbles = ScaleNibbles(aPixel >> 12, aChannels) & 0xF0F0Fu;
            unsigned short aPacked = (unsigned short)((aNibbles & 0xFu)
                                    | ((aPixel >> 12) << 12)
                                    | ((aNibbles >> 8) & 0xF00u)
                                    | ((aNibbles >> 4) & 0xF0u));
            WriteHalf(aDst + aCol * 2, aPacked);
        }
    }
    return 1;
}

int Sexy::PixelData::ConvertAndPremulR4G4B4A4ToA4R4G4B4(const Sexy::PixelData& theSource)
{
    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            unsigned int aPixel = ReadHalf(aSrc + aCol * 2);
            unsigned int aChannels = (aPixel & 0xF00u)
                                   | ((aPixel & 0xFFu) >> 4)
                                   | ((aPixel >> 12) << 16);
            unsigned int aNibbles = ScaleNibbles(aPixel & 0xFu, aChannels) & 0xF0F0Fu;
            unsigned short aPacked = (unsigned short)((aNibbles & 0xFu)
                                    | ((aPixel & 0xFu) << 12)
                                    | ((aNibbles >> 8) & 0xF00u)
                                    | ((aNibbles >> 4) & 0xF0u));
            WriteHalf(aDst + aCol * 2, aPacked);
        }
    }
    return 1;
}

int Sexy::PixelData::ConvertAndPremulR4G4B4A4ToR4G4B4A4(const Sexy::PixelData& theSource)
{
    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            unsigned int aPixel = ReadHalf(aSrc + aCol * 2);
            unsigned int aChannels = (aPixel & 0xF00u)
                                   | ((aPixel & 0xFFu) >> 4)
                                   | ((aPixel >> 12) << 16);
            unsigned int aNibbles = ScaleNibbles(aPixel & 0xFu, aChannels) & 0xF0F0Fu;
            unsigned int aLow = (unsigned char)(16u * aNibbles);
            unsigned short aPacked = (unsigned short)((aPixel & 0xFu)
                                    | (aNibbles & 0xF00u)
                                    | ((aNibbles >> 4) & 0xF000u)
                                    | aLow);
            WriteHalf(aDst + aCol * 2, aPacked);
        }
    }
    return 1;
}

int Sexy::PixelData::ConvertAndPremulA4R4G4B4ToR4G4B4A4(const Sexy::PixelData& theSource)
{
    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            unsigned int aPixel = ReadHalf(aSrc + aCol * 2);
            unsigned int aChannels = (aPixel & 0xFu)
                                   | (((aPixel & 0xFFu) >> 4) << 8)
                                   | (((aPixel >> 8) & 0xFu) << 16);
            unsigned int aNibbles = ScaleNibbles(aPixel >> 12, aChannels) & 0xF0F0Fu;
            unsigned int aLow = (unsigned char)(16u * aNibbles);
            unsigned short aPacked = (unsigned short)((aPixel >> 12)
                                    | (aNibbles & 0xF00u)
                                    | ((aNibbles >> 4) & 0xF000u)
                                    | aLow);
            WriteHalf(aDst + aCol * 2, aPacked);
        }
    }
    return 1;
}

int Sexy::PixelData::ConvertAndPremulA4R4G4B4(const Sexy::PixelData& theSource)
{
    int aDstFormat = mFormat;
    int aSrcFormat = theSource.mFormat;
    if ((aDstFormat & ~4) != PIXEL_FORMAT_A4R4G4B4)
        return 0;
    if ((aSrcFormat & ~4) != PIXEL_FORMAT_A4R4G4B4)
        return 0;

    if (aSrcFormat == PIXEL_FORMAT_R4G4B4A4)
    {
        if (aDstFormat == PIXEL_FORMAT_R4G4B4A4)
            ConvertAndPremulR4G4B4A4ToR4G4B4A4(theSource);
        else if (aDstFormat == PIXEL_FORMAT_A4R4G4B4)
            ConvertAndPremulR4G4B4A4ToA4R4G4B4(theSource);
        return 1;
    }

    if (aSrcFormat != PIXEL_FORMAT_A4R4G4B4)
        return 1;

    if (aDstFormat == PIXEL_FORMAT_A4R4G4B4)
        ConvertAndPremulA4R4G4B4ToA4R4G4B4(theSource);
    else if (aDstFormat == PIXEL_FORMAT_R4G4B4A4)
        ConvertAndPremulA4R4G4B4ToR4G4B4A4(theSource);
    return 1;
}

int Sexy::PixelData::ConvertAndPremulA4R4G4B4ToA8B8G8R8(const Sexy::PixelData& theSource)
{
    if (theSource.mFormat != PIXEL_FORMAT_A4R4G4B4 || mFormat != PIXEL_FORMAT_A8B8G8R8)
        return 0;

    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            unsigned int aPixel = ReadHalf(aSrc + aCol * 2);
            unsigned int aChannels = ((aPixel >> 8) & 0xFu)
                                   | ((aPixel & 0xFu) << 16)
                                   | (((aPixel & 0xFFu) >> 4) << 8);
            unsigned int aNibbles = ScaleNibbles(aPixel >> 12, aChannels) & 0xF0F0Fu;
            unsigned int aAlphaNibble = aPixel >> 12;
            WriteWord(aDst + aCol * 4,
                      aNibbles | (aAlphaNibble << 28) | (aAlphaNibble << 24) | (16u * aNibbles));
        }
    }
    return 1;
}

int Sexy::PixelData::ConvertAndPremul(const Sexy::PixelData& theSource,
                                      bool theNoTransparency,
                                      bool theNoAlpha)
{
    if (theSource.mWidth != mWidth || theSource.mHeight != mHeight)
        return 0;

    int aHasAlpha = PixelUtil::HasAlpha(theSource.mFormat);
    if (aHasAlpha == 0 || (!theNoTransparency && !theNoAlpha))
        return Convert(theSource);

    if (ConvertAndPremulA4R4G4B4(theSource) != 0
        || ConvertAndPremulA4R4G4B4ToA8B8G8R8(theSource) != 0)
        return aHasAlpha;

    unsigned int (*aMultiply)(unsigned int) = &PixelUtil::MultiplyPixel;
    if (theNoTransparency && !theNoAlpha)
        aMultiply = &PixelUtil::MultiplyPixelNoTrans;
    else if (!theNoTransparency && theNoAlpha)
        aMultiply = &PixelUtil::MultiplyPixelNoAlpha;

    int aSrcPixelSize = PixelUtil::GetBytePerPixel(theSource.mFormat);
    int aDstPixelSize = PixelUtil::GetBytePerPixel(mFormat);
    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        const unsigned char* aSrc = theSource.mBits + aRow * theSource.mPitch;
        unsigned char* aDst = mBits + aRow * mPitch;
        for (int aCol = 0; aCol < mWidth; ++aCol)
        {
            unsigned int aPixel = (unsigned int)PixelUtil::UnpackPixel(theSource.mFormat, aSrc);
            PixelUtil::PackPixel(mFormat, aMultiply(aPixel), aDst);
            aSrc += aSrcPixelSize;
            aDst += aDstPixelSize;
        }
    }
    return aHasAlpha;
}

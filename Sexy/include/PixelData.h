// A rectangular pixel buffer: format, bits, width, height, and byte pitch.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PIXELDATA_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PIXELDATA_H

#include "ReconTypes.h"
#include "PixelUtil.h"

namespace Sexy
{

class PixelData
{
public:
    PixelFormat mFormat;
    unsigned char *mBits;
    int mWidth;
    int mHeight;
    int mPitch;

    PixelData(PixelFormat theFormat, int theWidth, int theHeight, int theStride, void *theBits) :
        mFormat(theFormat),
        mBits(static_cast<unsigned char *>(theBits)),
        mWidth(theWidth),
        mHeight(theHeight),
        mPitch(theStride)
    {
    }

    int Copy(const PixelData &theSource);
    int ConvertA8R8G8B8ToA1R5G5B5(const PixelData &theSource);
    int ConvertA8R8G8B8ToR5G6B5(const PixelData &theSource);
    int ConvertA8R8G8B8ToA4R4G4B4(const PixelData &theSource);
    int Convert(const PixelData &theSource);
    int ConvertAndPremulA4R4G4B4ToA4R4G4B4(const PixelData &theSource);
    int ConvertAndPremulR4G4B4A4ToA4R4G4B4(const PixelData &theSource);
    int ConvertAndPremulR4G4B4A4ToR4G4B4A4(const PixelData &theSource);
    int ConvertAndPremulA4R4G4B4ToR4G4B4A4(const PixelData &theSource);
    int ConvertAndPremulA4R4G4B4(const PixelData &theSource);
    int ConvertAndPremulA4R4G4B4ToA8B8G8R8(const PixelData &theSource);
    int ConvertAndPremul(const PixelData &theSource, bool theNoTransparency, bool theNoAlpha);
};

} // namespace Sexy

using Sexy::PixelData;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PIXELDATA_H

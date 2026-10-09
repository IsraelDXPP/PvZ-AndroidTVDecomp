// SWHelper is the software rasterizer. The class has no state.
//
// SWDrawShape reads a strip of SWShapeVertex (x, y, u, v, colour), clips each
// triangle to the destination rect, and paints it into 32-bit destination bits.
// Texture tags are the format's bit layout, not the PixelFormat index.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

struct SWHelper_SWTextureInfo
{
    int          mFormat;      // +0x00
    unsigned int * mBits;      // +0x04
    int          mWidthShift;  // +0x08
    int          mWidthMask;   // +0x0C
    int          mHeightMask;  // +0x10
    int          mPitch;       // +0x14
    int          mPixelSize;   // +0x18
    int          mPixelCount;  // +0x1C
    int          mHeight;      // +0x20
    int          mAlpha;       // +0x24
};

struct SWHelper_SWDiffuse
{
    int mAlpha; // +0x00
    int mRed;   // +0x04
    int mGreen; // +0x08
    int mBlue;  // +0x0C
};

struct SWClipVertex
{
    float mX;
    float mU;
    float mV;
    float mW;
    unsigned int mColor;
};

// One corner of a software-blitted polygon. MemoryImage passes these in strip
// order: triangle i is vertices i, i+1, i+2. A quad stored as top-left,
// top-right, bottom-left, bottom-right therefore covers the rectangle.
struct SWShapeVertex
{
    float mX;
    float mY;
    float mU;
    float mV;
    unsigned int mColor;
};

using DrawTriFunc = int (*)(int, unsigned int *, unsigned int,
                           Sexy::SWHelper_SWTextureInfo const *,
                           Sexy::SWHelper_SWDiffuse &);
extern DrawTriFunc gDrawTriFuncs[128];

extern int gClipVertexCount;
extern SWClipVertex gClipVertices[192];

class SWHelper
{
public:
    int SWReadTexel(Sexy::SWHelper_SWTextureInfo const* theTexture, unsigned char const* theTexel);
    int SWDrawTriangle(bool theTextured, bool theAlpha, bool theVertexColor, bool theUseDiffuse,
                       SWShapeVertex const* theTriangle, unsigned int * theBits, unsigned int thePitch,
                       Sexy::SWHelper_SWTextureInfo const* theTexture,
                       Sexy::SWHelper_SWDiffuse & theDiffuse, int thePixelFormat, bool theAdditive);
    char * SWDrawShape(SWShapeVertex* theVertices, int theVertexCount, Sexy::MemoryImage * theImage,
                       Color const& theColor, int theDrawMode, TRect<int> const& theDest,
                       void * theBits, int thePitch, int thePixelFormat, bool theAdditive,
                       bool theVertexColor);
};

} // namespace Sexy

using Sexy::SWHelper_SWDiffuse;
using Sexy::SWHelper_SWTextureInfo;
using Sexy::SWShapeVertex;

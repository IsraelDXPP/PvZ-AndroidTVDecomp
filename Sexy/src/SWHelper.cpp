// SWHelper is the software rasterizer used when a MemoryImage blits a
// transformed quad or a triangle list. Each strip triangle is clipped to the
// destination rect, then filled with a barycentric scan. Texture tags are the
// bit layouts documented below, copied from the binary's sampler.

#include "SWHelper.h"

#include "MemoryImage.h"

#include <cmath>
#include <cstring>

namespace
{

const int kTextureTag8888 = 0x8888;
const int kTextureTag4444 = 0x4444;
const int kTextureTag1555 = 0x1555;
const int kTextureTag0565 = 0x0565;
const int kTextureTag14444 = 0x14444;
const int kTextureTag5551 = 0x5551;

int Expand4(int theNibble)
{
    int nibble = theNibble & 0xF;
    return (nibble << 4) | nibble;
}

int Expand5(int theFive)
{
    int bits = theFive & 0x1F;
    return (bits << 3) | (bits >> 2);
}

int Expand6(int theSix)
{
    int bits = theSix & 0x3F;
    return (bits << 2) | (bits >> 4);
}

unsigned int PackChannels(int theBlue, int theGreen, int theRed, int theAlpha)
{
    return (unsigned int)(theBlue & 0xFF) | ((unsigned int)(theGreen & 0xFF) << 8) |
        ((unsigned int)(theRed & 0xFF) << 16) | ((unsigned int)(theAlpha & 0xFF) << 24);
}

unsigned short ReadU16(unsigned char const *theTexel)
{
    return (unsigned short)(theTexel[0] | (theTexel[1] << 8));
}

unsigned int ReadU32(unsigned char const *theTexel)
{
    return (unsigned int)theTexel[0] | ((unsigned int)theTexel[1] << 8) | ((unsigned int)theTexel[2] << 16) |
        ((unsigned int)theTexel[3] << 24);
}

int LerpByte(int theStart, int theEnd, float theT)
{
    int value = (int)((float)theStart + ((float)theEnd - (float)theStart) * theT);
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return value;
}

unsigned int LerpColor(unsigned int theStart, unsigned int theEnd, float theT)
{
    return PackChannels(LerpByte((int)(theStart & 0xFF), (int)(theEnd & 0xFF), theT),
        LerpByte((int)((theStart >> 8) & 0xFF), (int)((theEnd >> 8) & 0xFF), theT),
        LerpByte((int)((theStart >> 16) & 0xFF), (int)((theEnd >> 16) & 0xFF), theT),
        LerpByte((int)((theStart >> 24) & 0xFF), (int)((theEnd >> 24) & 0xFF), theT));
}

Sexy::SWShapeVertex LerpVertex(Sexy::SWShapeVertex const &theStart, Sexy::SWShapeVertex const &theEnd, float theT)
{
    Sexy::SWShapeVertex out;
    out.mX = theStart.mX + (theEnd.mX - theStart.mX) * theT;
    out.mY = theStart.mY + (theEnd.mY - theStart.mY) * theT;
    out.mU = theStart.mU + (theEnd.mU - theStart.mU) * theT;
    out.mV = theStart.mV + (theEnd.mV - theStart.mV) * theT;
    out.mColor = LerpColor(theStart.mColor, theEnd.mColor, theT);
    return out;
}

bool InsideEdge(
    Sexy::SWShapeVertex const &theVertex, int theEdge, float theLeft, float theTop, float theRight, float theBottom)
{
    if (theEdge == 0)
        return theVertex.mX >= theLeft;
    if (theEdge == 1)
        return theVertex.mX <= theRight;
    if (theEdge == 2)
        return theVertex.mY >= theTop;
    return theVertex.mY <= theBottom;
}

float EdgePosition(Sexy::SWShapeVertex const &theVertex, int theEdge)
{
    if (theEdge < 2)
        return theVertex.mX;
    return theVertex.mY;
}

float EdgeLimit(int theEdge, float theLeft, float theTop, float theRight, float theBottom)
{
    if (theEdge == 0)
        return theLeft;
    if (theEdge == 1)
        return theRight;
    if (theEdge == 2)
        return theTop;
    return theBottom;
}

int ClipEdge(Sexy::SWShapeVertex const *theInput, int theCount, int theEdge, float theLeft, float theTop,
    float theRight, float theBottom, Sexy::SWShapeVertex *theOutput)
{
    if (theCount <= 0)
        return 0;
    int written = 0;
    float limit = EdgeLimit(theEdge, theLeft, theTop, theRight, theBottom);
    Sexy::SWShapeVertex previous = theInput[theCount - 1];
    bool previousIn = InsideEdge(previous, theEdge, theLeft, theTop, theRight, theBottom);
    for (int index = 0; index < theCount; ++index)
    {
        Sexy::SWShapeVertex current = theInput[index];
        bool currentIn = InsideEdge(current, theEdge, theLeft, theTop, theRight, theBottom);
        if (currentIn != previousIn)
        {
            float span = EdgePosition(current, theEdge) - EdgePosition(previous, theEdge);
            float t = span != 0.0f ? (limit - EdgePosition(previous, theEdge)) / span : 0.0f;
            if (written < 8)
                theOutput[written++] = LerpVertex(previous, current, t);
        }
        if (currentIn && written < 8)
            theOutput[written++] = current;
        previous = current;
        previousIn = currentIn;
    }
    return written;
}

int ClipTriangle(Sexy::SWShapeVertex const &theA, Sexy::SWShapeVertex const &theB, Sexy::SWShapeVertex const &theC,
    float theLeft, float theTop, float theRight, float theBottom, Sexy::SWShapeVertex *theOutput)
{
    Sexy::SWShapeVertex first[8];
    Sexy::SWShapeVertex second[8];
    first[0] = theA;
    first[1] = theB;
    first[2] = theC;
    int count = 3;
    for (int edge = 0; edge < 4; ++edge)
    {
        Sexy::SWShapeVertex *dest = (edge & 1) == 0 ? second : first;
        Sexy::SWShapeVertex const *src = (edge & 1) == 0 ? first : second;
        count = ClipEdge(src, count, edge, theLeft, theTop, theRight, theBottom, dest);
    }
    for (int index = 0; index < count; ++index)
        theOutput[index] = first[index];
    return count;
}

int WidthShift(int theWidth)
{
    unsigned int half = (unsigned int)theWidth >> 1;
    if (half == 0)
        return 16;
    int steps = 1;
    while ((half >>= 1) != 0)
        ++steps;
    return 16 - steps;
}

unsigned int MultiplyColor(unsigned int theLeft, unsigned int theRight)
{
    return PackChannels(((int)(theLeft & 0xFF) * (int)(theRight & 0xFF)) / 255,
        ((int)((theLeft >> 8) & 0xFF) * (int)((theRight >> 8) & 0xFF)) / 255,
        ((int)((theLeft >> 16) & 0xFF) * (int)((theRight >> 16) & 0xFF)) / 255,
        ((int)((theLeft >> 24) & 0xFF) * (int)((theRight >> 24) & 0xFF)) / 255);
}

unsigned int BlendPixel(unsigned int theDest, unsigned int theSrc, bool theAdditive)
{
    int srcAlpha = (int)((theSrc >> 24) & 0xFF);
    if (srcAlpha <= 0)
        return theDest;
    if (!theAdditive && srcAlpha >= 255)
        return theSrc;
    int inv = 255 - srcAlpha;
    int blue = (int)(theSrc & 0xFF);
    int green = (int)((theSrc >> 8) & 0xFF);
    int red = (int)((theSrc >> 16) & 0xFF);
    if (theAdditive)
    {
        blue = (int)(theDest & 0xFF) + blue * srcAlpha / 255;
        green = (int)((theDest >> 8) & 0xFF) + green * srcAlpha / 255;
        red = (int)((theDest >> 16) & 0xFF) + red * srcAlpha / 255;
        if (blue > 255)
            blue = 255;
        if (green > 255)
            green = 255;
        if (red > 255)
            red = 255;
        return PackChannels(blue, green, red, 255);
    }
    blue = (blue * srcAlpha + (int)(theDest & 0xFF) * inv) / 255;
    green = (green * srcAlpha + (int)((theDest >> 8) & 0xFF) * inv) / 255;
    red = (red * srcAlpha + (int)((theDest >> 16) & 0xFF) * inv) / 255;
    int alpha = (srcAlpha * srcAlpha + (int)((theDest >> 24) & 0xFF) * inv) / 255;
    return PackChannels(blue, green, red, alpha);
}

float EdgeCross(float theAx, float theAy, float theBx, float theBy, float theCx, float theCy)
{
    return (theBx - theAx) * (theCy - theAy) - (theBy - theAy) * (theCx - theAx);
}

} // namespace

Sexy::DrawTriFunc Sexy::gDrawTriFuncs[128] = {};
int Sexy::gClipVertexCount = 0;
Sexy::SWClipVertex Sexy::gClipVertices[192] = {};

int Sexy::SWHelper::SWReadTexel(Sexy::SWHelper_SWTextureInfo const *theTexture, unsigned char const *theTexel)
{
    if (theTexture == nullptr || theTexel == nullptr)
        return 0;
    int format = theTexture->mFormat;
    if (format == kTextureTag8888)
        return (int)ReadU32(theTexel);
    if (format == kTextureTag4444)
    {
        unsigned int texel = ReadU16(theTexel);
        return (int)PackChannels(
            Expand4((int)texel), Expand4((int)(texel >> 4)), Expand4((int)(texel >> 8)), Expand4((int)(texel >> 12)));
    }
    if (format == kTextureTag0565)
    {
        unsigned int texel = ReadU16(theTexel);
        return (int)PackChannels(Expand5((int)texel), Expand6((int)(texel >> 5)), Expand5((int)(texel >> 11)), 255);
    }
    if (format == kTextureTag1555)
    {
        unsigned int texel = ReadU16(theTexel);
        int alpha = (texel & 0x8000) != 0 ? 255 : 0;
        return (int)PackChannels(Expand5((int)texel), Expand5((int)(texel >> 5)), Expand5((int)(texel >> 10)), alpha);
    }
    if (format == kTextureTag14444)
    {
        unsigned int texel = ReadU16(theTexel);
        return (int)PackChannels(
            Expand4((int)(texel >> 4)), Expand4((int)(texel >> 8)), Expand4((int)(texel >> 12)), Expand4((int)texel));
    }
    if (format == kTextureTag5551)
    {
        unsigned int texel = ReadU16(theTexel);
        int alpha = (texel & 1) != 0 ? 255 : 0;
        return (int)PackChannels(
            Expand5((int)(texel >> 1)), Expand5((int)(texel >> 6)), Expand5((int)(texel >> 11)), alpha);
    }
    return 0;
}

int Sexy::SWHelper::SWDrawTriangle(bool theTextured, bool theAlpha, bool theVertexColor, bool theUseDiffuse,
    Sexy::SWShapeVertex const *theTriangle, unsigned int *theBits, unsigned int thePitch,
    Sexy::SWHelper_SWTextureInfo const *theTexture, Sexy::SWHelper_SWDiffuse &theDiffuse, int thePixelFormat,
    bool theAdditive)
{
    (void)thePixelFormat;
    if (theTriangle == nullptr || theBits == nullptr || thePitch < 4)
        return 0;

    float minX = theTriangle[0].mX;
    float maxX = theTriangle[0].mX;
    float minY = theTriangle[0].mY;
    float maxY = theTriangle[0].mY;
    for (int corner = 1; corner < 3; ++corner)
    {
        if (theTriangle[corner].mX < minX)
            minX = theTriangle[corner].mX;
        if (theTriangle[corner].mX > maxX)
            maxX = theTriangle[corner].mX;
        if (theTriangle[corner].mY < minY)
            minY = theTriangle[corner].mY;
        if (theTriangle[corner].mY > maxY)
            maxY = theTriangle[corner].mY;
    }
    int x0 = (int)floorf(minX);
    int x1 = (int)ceilf(maxX);
    int y0 = (int)floorf(minY);
    int y1 = (int)ceilf(maxY);
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;

    float area = EdgeCross(theTriangle[0].mX,
        theTriangle[0].mY,
        theTriangle[1].mX,
        theTriangle[1].mY,
        theTriangle[2].mX,
        theTriangle[2].mY);
    if (fabsf(area) < 0.0001f)
        return 0;

    unsigned int diffuse = PackChannels(theDiffuse.mBlue, theDiffuse.mGreen, theDiffuse.mRed, theDiffuse.mAlpha);
    int texWidth = 0;
    int texHeight = theTexture != nullptr ? theTexture->mHeight : 0;
    if (theTexture != nullptr && theTexture->mWidthMask != 0)
        texWidth = (theTexture->mWidthMask >> 16) + 1;

    unsigned char *destBytes = static_cast<unsigned char *>(static_cast<void *>(theBits));
    int maxPixel = (int)thePitch / 4 - 1;
    if (x1 > maxPixel)
        x1 = maxPixel;
    for (int y = y0; y <= y1; ++y)
    {
        float py = (float)y + 0.5f;
        for (int x = x0; x <= x1; ++x)
        {
            float px = (float)x + 0.5f;
            float w0 =
                EdgeCross(theTriangle[1].mX, theTriangle[1].mY, theTriangle[2].mX, theTriangle[2].mY, px, py) / area;
            float w1 =
                EdgeCross(theTriangle[2].mX, theTriangle[2].mY, theTriangle[0].mX, theTriangle[0].mY, px, py) / area;
            float w2 =
                EdgeCross(theTriangle[0].mX, theTriangle[0].mY, theTriangle[1].mX, theTriangle[1].mY, px, py) / area;
            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                continue;

            unsigned int color = 0xFFFFFFFFu;
            if (theVertexColor)
            {
                color = PackChannels(LerpByte(0,
                                         (int)((theTriangle[0].mColor & 0xFF) * w0 +
                                             (theTriangle[1].mColor & 0xFF) * w1 + (theTriangle[2].mColor & 0xFF) * w2),
                                         1.0f),
                    LerpByte(0,
                        (int)(((theTriangle[0].mColor >> 8) & 0xFF) * w0 + ((theTriangle[1].mColor >> 8) & 0xFF) * w1 +
                            ((theTriangle[2].mColor >> 8) & 0xFF) * w2),
                        1.0f),
                    LerpByte(0,
                        (int)(((theTriangle[0].mColor >> 16) & 0xFF) * w0 +
                            ((theTriangle[1].mColor >> 16) & 0xFF) * w1 + ((theTriangle[2].mColor >> 16) & 0xFF) * w2),
                        1.0f),
                    LerpByte(0,
                        (int)(((theTriangle[0].mColor >> 24) & 0xFF) * w0 +
                            ((theTriangle[1].mColor >> 24) & 0xFF) * w1 + ((theTriangle[2].mColor >> 24) & 0xFF) * w2),
                        1.0f));
            }
            if (theTextured && theTexture != nullptr && theTexture->mBits != nullptr && texWidth > 0 && texHeight > 0 &&
                theTexture->mPixelSize > 0)
            {
                float u = theTriangle[0].mU * w0 + theTriangle[1].mU * w1 + theTriangle[2].mU * w2;
                float v = theTriangle[0].mV * w0 + theTriangle[1].mV * w1 + theTriangle[2].mV * w2;
                int col = (int)(u * (float)texWidth);
                int rowIndex = (int)(v * (float)texHeight);
                if (col < 0)
                    col = 0;
                if (rowIndex < 0)
                    rowIndex = 0;
                if (col >= texWidth)
                    col = texWidth - 1;
                if (rowIndex >= texHeight)
                    rowIndex = texHeight - 1;
                unsigned char const *bytes =
                    static_cast<unsigned char const *>(static_cast<void const *>(theTexture->mBits));
                int offset = (rowIndex * theTexture->mPitch + col) * theTexture->mPixelSize;
                color = MultiplyColor(color, (unsigned int)SWReadTexel(theTexture, bytes + offset));
            }
            if (theUseDiffuse)
                color = MultiplyColor(color, diffuse);
            if (!theAlpha)
                color = (color & 0x00FFFFFFu) | 0xFF000000u;
            unsigned char *pixelBytes = destBytes + y * (int)thePitch + x * 4;
            unsigned int previous = 0;
            std::memcpy(&previous, pixelBytes, sizeof(previous));
            unsigned int blended = BlendPixel(previous, color, theAdditive);
            std::memcpy(pixelBytes, &blended, sizeof(blended));
        }
    }
    return 1;
}

char *Sexy::SWHelper::SWDrawShape(Sexy::SWShapeVertex *theVertices, int theVertexCount, Sexy::MemoryImage *theImage,
    Color const &theColor, int theDrawMode, TRect<int> const &theDest, void *theBits, int thePitch, int thePixelFormat,
    bool theAdditive, bool theVertexColor)
{
    (void)thePixelFormat;
    if (theVertices == nullptr || theVertexCount < 3 || theBits == nullptr)
        return nullptr;

    Sexy::SWHelper_SWDiffuse diffuse;
    diffuse.mRed = theColor.mRed;
    diffuse.mGreen = theColor.mGreen;
    diffuse.mBlue = theColor.mBlue;
    diffuse.mAlpha = theColor.mAlpha;
    bool useDiffuse = theColor.mRed != 255 || theColor.mGreen != 255 || theColor.mBlue != 255 || theColor.mAlpha != 255;

    Sexy::SWHelper_SWTextureInfo texture;
    texture.mFormat = 0;
    texture.mBits = nullptr;
    texture.mWidthShift = 16;
    texture.mWidthMask = 0;
    texture.mHeightMask = 0;
    texture.mPitch = 0;
    texture.mPixelSize = 0;
    texture.mPixelCount = 0;
    texture.mHeight = 0;
    texture.mAlpha = theDrawMode;

    bool textured = false;
    bool sourceAlpha = false;
    if (theImage != nullptr)
    {
        theImage->CommitBits();
        sourceAlpha = theImage->mHasAlpha || theImage->mHasTrans || theAdditive;
        int formatIndex = (int)theImage->GetPixelFormat() - 1;
        int pixelSize = 0;
        int tag = 0;
        unsigned int *pixels = nullptr;
        if (formatIndex == 0 || formatIndex == 1 || formatIndex == 5)
        {
            tag = kTextureTag8888;
            pixelSize = 4;
            pixels = theImage->GetBits();
        }
        else if (formatIndex == 2)
        {
            tag = kTextureTag4444;
            pixelSize = 2;
            pixels = theImage->GetPixels();
        }
        else if (formatIndex == 3)
        {
            tag = kTextureTag1555;
            pixelSize = 2;
            pixels = theImage->GetPixels();
        }
        else if (formatIndex == 4)
        {
            tag = kTextureTag0565;
            pixelSize = 2;
            pixels = theImage->GetPixels();
        }
        else if (formatIndex == 6)
        {
            tag = kTextureTag14444;
            pixelSize = 2;
            pixels = theImage->GetPixels();
        }
        else if (formatIndex == 7)
        {
            tag = kTextureTag5551;
            pixelSize = 2;
            pixels = theImage->GetPixels();
        }
        if (pixels != nullptr && pixelSize != 0)
        {
            textured = true;
            texture.mFormat = tag;
            texture.mBits = pixels;
            texture.mPixelSize = pixelSize;
            texture.mPitch = theImage->GetPitch();
            texture.mHeight = theImage->GetHeight();
            texture.mPixelCount = texture.mHeight * texture.mPitch;
            texture.mWidthShift = WidthShift(theImage->GetWidth());
            texture.mWidthMask = (theImage->mWidth - 1) << 16;
            texture.mHeightMask = (theImage->mHeight - 1) << 16;
        }
    }

    float left = (float)theDest.mX;
    float top = (float)theDest.mY;
    float right = (float)(theDest.mX + theDest.mWidth - 1);
    float bottom = (float)(theDest.mY + theDest.mHeight - 1);
    unsigned int *dest = static_cast<unsigned int *>(theBits);
    for (int index = 0; index <= theVertexCount - 3; ++index)
    {
        Sexy::SWShapeVertex clipped[8];
        int count = ClipTriangle(
            theVertices[index], theVertices[index + 1], theVertices[index + 2], left, top, right, bottom, clipped);
        gClipVertexCount = count;
        for (int corner = 0; corner < count && corner < 192; ++corner)
        {
            gClipVertices[corner].mX = clipped[corner].mX;
            gClipVertices[corner].mU = clipped[corner].mY;
            gClipVertices[corner].mV = clipped[corner].mU;
            gClipVertices[corner].mW = clipped[corner].mV;
            gClipVertices[corner].mColor = clipped[corner].mColor;
        }
        for (int fan = 1; fan < count - 1; ++fan)
        {
            Sexy::SWShapeVertex triangle[3];
            triangle[0] = clipped[0];
            triangle[1] = clipped[fan];
            triangle[2] = clipped[fan + 1];
            SWDrawTriangle(textured,
                sourceAlpha,
                theVertexColor,
                useDiffuse,
                triangle,
                dest,
                (unsigned int)thePitch,
                &texture,
                diffuse,
                thePixelFormat,
                theAdditive);
        }
    }
    return nullptr;
}

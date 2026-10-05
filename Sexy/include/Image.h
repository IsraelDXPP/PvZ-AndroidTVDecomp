// Image is the root of the image hierarchy. Width, height, pitch, stride
// and pixel format are stored here; drawing is virtual and empty, so a
// memory or device image replaces the slots that actually paint.
//
// After the vptr:
//   mCreateFlags       create flags. Bit 0x10 tells a device image to skip the 3D lock.
//   mPixelsLive        non-zero when the pixel buffer is live and can be recovered.
//   mAboutToSample     set by a blit before it samples the source
//   mSpareWidth, mSpareHeight  cleared by the constructor; Init clears the height word
//   mFileName          the file the image was loaded from
//   mFileVariant       the variant recorded beside that file
//   mFileAttributes    the attributes recorded beside that file
//   mWidth, mHeight, mPitch, mStride, mFormat, mPixelFormatHint
//   mNumRows, mNumCols the cel grid; the constructor writes 1 and 1
//   mAnimInfo          the animation strip, owned
//   mAttachedParent and the four rect words record the sub-image Attach built.
//   mAttachedPad is the unused sixth word of that record.
//   mHorizonWrapMode, mVertWrapMode
//   mTag               the four-character tag MakeTag packs

#pragma once

#include "ReconTypes.h"
#include "AnimInfo.h"

namespace Sexy {

class Image
{
public:
    virtual ~Image();

    virtual MemoryImage * AsMemoryImage();
    virtual DeviceImage * AsDeviceImage();
    virtual int GetWidth();
    virtual int GetHeight();
    virtual int GetPitch();
    virtual int GetStride();
    virtual PixelFormat GetPixelFormat();
    virtual PixelFormat GetPixelFormatHint();
    virtual void SetPixelFormatHint(PixelFormat theFormat);
    virtual void Finalize();
    virtual void SetBits(unsigned int * theBits, int theWidth, int theHeight, bool theOwnsBits);
    virtual unsigned int * GetBits();
    virtual unsigned int GetPixel(int theX, int theY);
    virtual unsigned int * GetPixels();
    virtual void PolyFill3D(const TPoint<int> * thePoints, int thePointCount, const TRect<int> * theClipRect, const Color & theColor, int theDrawMode, int theTx, int theTy, bool theConvex);
    virtual void FillRect(const TRect<int> & theRect, const Color & theColor, int theDrawMode);
    virtual void DrawRect(const TRect<int> & theRect, const Color & theColor, int theDrawMode);
    virtual void ClearRect(const TRect<int> & theRect);
    virtual void DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color & theColor, int theDrawMode);
    virtual void DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color & theColor, int theDrawMode);
    virtual void FillScanLines(int * theSpans, int theSpanCount, const Color & theColor, int theDrawMode);
    virtual void FillScanLinesWithCoverage(int * theSpans, int theSpanCount, const Color & theColor, int theDrawMode, const unsigned char * theCoverage, int theCoverageX, int theCoverageY, int theCoverageWidth, int theCoverageHeight);
    virtual void Blt(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect, const Color & theColor, int theDrawMode, bool theColorize = false);
    virtual void BltF(Image * theSource, float theX, float theY, const TRect<int> & theSourceRect, const TRect<int> & theClipRect, const Color & theColor, int theDrawMode);
    virtual void BltRotated(Image * theSource, float theX, float theY, const TRect<int> & theSourceRect, const TRect<int> & theClipRect, const Color & theColor, int theDrawMode, double theRotate, float theRotateCenterX, float theRotateCenterY);
    virtual void StretchBlt(Image * theSource, const TRect<int> & theDestRect, const TRect<int> & theSrcRect, const TRect<int> & theClipRect, const Color & theColor, int theDrawMode, bool theFastStretch);
    virtual void BltMatrix(Image * theSource, float theX, float theY, const SexyMatrix3 & theMatrix, const TRect<int> & theSrcRect, const Color & theColor, int theDrawMode, const TRect<int> & theClipRect, bool theLinearBlend, bool theMirror);
    virtual void BltTrianglesTex(Image * theSource, const SexyVertex2D (*theTriangles)[3], int theTriangleCount, const TRect<int> & theClipRect, const Color & theColor, int theDrawMode, float theTx, float theTy, bool theBlend);
    virtual void BltMirror(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect, const Color & theColor, int theDrawMode);
    virtual void StretchBltMirror(Image * theSource, const TRect<int> & theDestRect, const TRect<int> & theSrcRect, const TRect<int> & theClipRect, const Color & theColor, int theDrawMode, bool theFastStretch);
    virtual void Palletize();
    virtual void Flip(unsigned int theFlags);
    virtual bool HasTransform();
    virtual void PushTransform(const SexyMatrix3 & theTransform, bool theConcatenate);
    virtual void PopTransform();
    virtual void SetWrapMode(int theWrapModeX, int theWrapModeY);
    virtual bool Is3D();

    int mCreateFlags;
    int mPixelsLive;
    bool mAboutToSample;
    int mSpareWidth;
    int mSpareHeight;
    Sexy::String mFileName;
    Sexy::String mFileVariant;
    Sexy::String mFileAttributes;
    int mWidth;
    int mHeight;
    int mPitch;
    int mStride;
    PixelFormat mFormat;
    PixelFormat mPixelFormatHint;
    int mNumRows;
    int mNumCols;
    AnimInfo * mAnimInfo;
    MemoryImage* mAttachedParent;
    int mSubRectX;
    int mSubRectY;
    int mSubRectRight;
    int mSubRectBottom;
    int mAttachedPad;
    int mHorizonWrapMode;
    int mVertWrapMode;
    int mTag;

    int MakeTag(char theA, char theB, char theC, char theD);
    Image();
    Image(Sexy::Image const& theOther);
    int GetCelHeight();
    int GetCelWidth();
    int GetCelCount();
    TRect<int> GetCelRect(int theCol);
    TRect<int> GetCelRect(int theCol, int theRow);
    void * CreateRenderData();
    int GetAnimCel(int theTick);
    TRect<int> GetAnimCelRect(int theCel);
    void CopyAttributes(Sexy::Image * theSource);
    Sexy::Graphics * GetGraphics();
};

} // namespace Sexy

using Sexy::Image;

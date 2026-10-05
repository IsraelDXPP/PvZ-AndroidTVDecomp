// Image stores size, format and the cel grid. Drawing slots are empty here;
// a memory or device image replaces the ones that paint. DrawRect and
// FillScanLines are the exceptions: they build one-pixel rectangles and
// call the virtual FillRect, so a subclass still does the fill.

#include "Image.h"

#include "Graphics.h"
#include "IGraphicsDriver.h"
#include "SexyAppBase.h"

int Sexy::Image::MakeTag(char theA, char theB, char theC, char theD)
{
    return (int)(unsigned char)theC
        | ((int)(unsigned char)theB << 8)
        | ((int)(unsigned char)theA << 16)
        | ((int)(unsigned char)theD << 24);
}

Sexy::Image::Image()
    : mCreateFlags(0),
      mPixelsLive(0),
      mAboutToSample(false),
      mSpareWidth(0),
      mSpareHeight(0),
      mFileName(),
      mFileVariant(),
      mFileAttributes(),
      mWidth(0),
      mHeight(0),
      mPitch(0),
      mStride(0),
      mFormat(0),
      mPixelFormatHint(0),
      mNumRows(1),
      mNumCols(1),
      mAnimInfo(nullptr),
      mAttachedParent(nullptr),
      mSubRectX(0),
      mSubRectY(0),
      mSubRectRight(0),
      mSubRectBottom(0),
      mAttachedPad(0),
      mHorizonWrapMode(0),
      mVertWrapMode(0),
      mTag(0)
{
    mTag = MakeTag('m', 'g', 0, 0);
}

Sexy::Image::Image(Sexy::Image const& theOther)
    : mCreateFlags(theOther.mCreateFlags),
      mPixelsLive(0),
      mAboutToSample(false),
      mSpareWidth(0),
      mSpareHeight(0),
      mFileName(theOther.mFileName),
      mFileVariant(theOther.mFileVariant),
      mFileAttributes(theOther.mFileAttributes),
      mWidth(theOther.mWidth),
      mHeight(theOther.mHeight),
      mPitch(theOther.mPitch),
      mStride(theOther.mStride),
      mFormat(theOther.mFormat),
      mPixelFormatHint(theOther.mPixelFormatHint),
      mNumRows(theOther.mNumRows),
      mNumCols(theOther.mNumCols),
      mAnimInfo(nullptr),
      mAttachedParent(theOther.mAttachedParent),
      mSubRectX(theOther.mSubRectX),
      mSubRectY(theOther.mSubRectY),
      mSubRectRight(theOther.mSubRectRight),
      mSubRectBottom(theOther.mSubRectBottom),
      mAttachedPad(theOther.mAttachedPad),
      mHorizonWrapMode(theOther.mHorizonWrapMode),
      mVertWrapMode(theOther.mVertWrapMode),
      mTag(theOther.mTag)
{
    if (theOther.mAnimInfo != nullptr)
        mAnimInfo = new AnimInfo(*theOther.mAnimInfo);
}

Sexy::Image::~Image()
{
    delete mAnimInfo;
}

Sexy::MemoryImage* Sexy::Image::AsMemoryImage()
{
    return nullptr;
}

Sexy::DeviceImage* Sexy::Image::AsDeviceImage()
{
    return nullptr;
}

int Sexy::Image::GetWidth()
{
    return mWidth;
}

int Sexy::Image::GetHeight()
{
    return mHeight;
}

int Sexy::Image::GetPitch()
{
    return mPitch;
}

int Sexy::Image::GetStride()
{
    return mStride;
}

Sexy::PixelFormat Sexy::Image::GetPixelFormat()
{
    return mFormat;
}

Sexy::PixelFormat Sexy::Image::GetPixelFormatHint()
{
    return mPixelFormatHint;
}

void Sexy::Image::SetPixelFormatHint(PixelFormat theFormat)
{
    mPixelFormatHint = theFormat;
}

void Sexy::Image::Finalize()
{
}

void Sexy::Image::SetBits(unsigned int*, int, int, bool)
{
}

unsigned int* Sexy::Image::GetBits()
{
    return nullptr;
}

unsigned int Sexy::Image::GetPixel(int theX, int theY)
{
    unsigned int* bits = GetBits();
    if (bits == nullptr)
        return 0;
    return bits[theX + mWidth * theY];
}

unsigned int* Sexy::Image::GetPixels()
{
    return nullptr;
}

void Sexy::Image::PolyFill3D(const TPoint<int>*, int, const TRect<int>*, const Color&, int, int, int, bool)
{
}

void Sexy::Image::FillRect(const TRect<int>&, const Color&, int)
{
}

void Sexy::Image::DrawRect(const TRect<int>& theRect, const Color& theColor, int theDrawMode)
{
    FillRect(TRect<int>(theRect.mX, theRect.mY, theRect.mWidth + 1, 1), theColor, theDrawMode);
    FillRect(TRect<int>(theRect.mX, theRect.mY + theRect.mHeight, theRect.mWidth + 1, 1),
             theColor, theDrawMode);
    FillRect(TRect<int>(theRect.mX, theRect.mY + 1, 1, theRect.mHeight - 1), theColor, theDrawMode);
    FillRect(TRect<int>(theRect.mX + theRect.mWidth, theRect.mY + 1, 1, theRect.mHeight - 1),
             theColor, theDrawMode);
}

void Sexy::Image::ClearRect(const TRect<int>&)
{
}

void Sexy::Image::DrawLine(double, double, double, double, const Color&, int)
{
}

void Sexy::Image::DrawLineAA(double, double, double, double, const Color&, int)
{
}

void Sexy::Image::FillScanLines(int* theSpans, int theSpanCount, const Color& theColor, int theDrawMode)
{
    for (int index = 0; index < theSpanCount; ++index)
    {
        int spanY = theSpans[0];
        int spanX = theSpans[1];
        int spanCount = theSpans[2];
        theSpans += 3;
        FillRect(TRect<int>(spanX, spanY, spanCount, 1), theColor, theDrawMode);
    }
}

void Sexy::Image::FillScanLinesWithCoverage(int*, int, const Color&, int, const unsigned char*, int, int, int, int)
{
}

void Sexy::Image::Blt(Image*, int, int, const TRect<int>&, const Color&, int, bool)
{
}

void Sexy::Image::BltF(Image*, float, float, const TRect<int>&, const TRect<int>&, const Color&, int)
{
}

void Sexy::Image::BltRotated(Image*, float, float, const TRect<int>&, const TRect<int>&, const Color&, int, double, float, float)
{
}

void Sexy::Image::StretchBlt(Image*, const TRect<int>&, const TRect<int>&, const TRect<int>&, const Color&, int, bool)
{
}

void Sexy::Image::BltMatrix(Image*, float, float, const SexyMatrix3&, const TRect<int>&, const Color&, int, const TRect<int>&, bool, bool)
{
}

void Sexy::Image::BltTrianglesTex(Image*, const SexyVertex2D (*)[3], int, const TRect<int>&, const Color&, int, float, float, bool)
{
}

void Sexy::Image::BltMirror(Image*, int, int, const TRect<int>&, const Color&, int)
{
}

void Sexy::Image::StretchBltMirror(Image*, const TRect<int>&, const TRect<int>&, const TRect<int>&, const Color&, int, bool)
{
}

void Sexy::Image::Palletize()
{
}

void Sexy::Image::Flip(unsigned int)
{
}

bool Sexy::Image::HasTransform()
{
    return false;
}

void Sexy::Image::PushTransform(const SexyMatrix3&, bool)
{
}

void Sexy::Image::PopTransform()
{
}

void Sexy::Image::SetWrapMode(int theWrapModeX, int theWrapModeY)
{
    mHorizonWrapMode = theWrapModeX;
    mVertWrapMode = theWrapModeY;
}

bool Sexy::Image::Is3D()
{
    return false;
}

int Sexy::Image::GetCelHeight()
{
    return mHeight / mNumRows;
}

int Sexy::Image::GetCelWidth()
{
    return mWidth / mNumCols;
}

int Sexy::Image::GetCelCount()
{
    return mNumRows * mNumCols;
}

TRect<int> Sexy::Image::GetCelRect(int theCel)
{
    int celHeight = GetCelHeight();
    int celWidth = GetCelWidth();
    TRect<int> rect;
    rect.mX = (theCel % mNumCols) * celWidth;
    rect.mY = (theCel / mNumCols) * celHeight;
    rect.mWidth = celWidth;
    rect.mHeight = celHeight;
    return rect;
}

TRect<int> Sexy::Image::GetCelRect(int theCol, int theRow)
{
    int celHeight = GetCelHeight();
    int celWidth = GetCelWidth();
    TRect<int> rect;
    rect.mX = theCol * celWidth;
    rect.mY = theRow * celHeight;
    rect.mWidth = celWidth;
    rect.mHeight = celHeight;
    return rect;
}

void* Sexy::Image::CreateRenderData()
{
    unsigned int* bits = GetBits();
    if (bits == nullptr || gSexyAppBase == nullptr || gSexyAppBase->mDDInterface == nullptr)
        return bits;
    Sexy::RenderDevice3D* device = gSexyAppBase->mDDInterface->GetRenderDevice3D();
    if (device == nullptr)
        return bits;
    return device->CreateImageRenderData(AsMemoryImage());
}

int Sexy::Image::GetAnimCel(int theTick)
{
    if (mAnimInfo == nullptr)
        return 0;
    return mAnimInfo->GetCel(theTick);
}

TRect<int> Sexy::Image::GetAnimCelRect(int theTick)
{
    int cel = GetAnimCel(theTick);
    int celWidth = GetCelWidth();
    int celHeight = GetCelHeight();
    TRect<int> rect;
    if (mNumCols <= 1)
    {
        rect.mX = 0;
        rect.mY = celHeight * cel;
        rect.mWidth = mWidth;
        rect.mHeight = celHeight;
    }
    else
    {
        rect.mX = celWidth * cel;
        rect.mY = 0;
        rect.mWidth = celWidth;
        rect.mHeight = mHeight;
    }
    return rect;
}

void Sexy::Image::CopyAttributes(Sexy::Image* theSource)
{
    if (theSource == nullptr)
        return;
    mNumCols = theSource->mNumCols;
    mNumRows = theSource->mNumRows;
    delete mAnimInfo;
    mAnimInfo = nullptr;
    if (theSource->mAnimInfo != nullptr)
        mAnimInfo = new AnimInfo(*theSource->mAnimInfo);
}

Sexy::Graphics* Sexy::Image::GetGraphics()
{
    return new Sexy::Graphics(this);
}

#include "MemoryImage.h"
#include "Platform/Bgfx/BgfxRenderDevice.h"
#include "CritSect.h"
#include "Image.h"
#include "PixelData.h"
#include "PixelUtil.h"
#include "SWHelper.h"
#include "NativeDisplay.h"
#include "ImageLib.h"
#include "SexyAppBase.h"
#include "LawnApp.h"
#include "ReconData.h"
#include "ReconFreeFunctions.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace Sexy {

bool Quantize8Bit(const unsigned long * theSrcBits, int theWidth, int theHeight,
                  unsigned char * theDestIndices, unsigned long * theDestTable,
                  unsigned int * theUnused);

}

namespace {

const int kDrawNormal = 0;
const int kDrawAdditive = 1;

int FormatBytesPerPixel(Sexy::PixelFormat theFormat)
{
    switch ((int)theFormat)
    {
    case 1: // LUT8
        return 1;
    case 3: // A4R4G4B4
    case 4: // A1R5G5B5
    case 5: // R5G6R5
    case 7: // R4G4B4A4
    case 8: // R5G5B5A1
    case 10: // L8A8
        return 2;
    case 2: // A8R8G8B8
    case 6: // A8B8G8R8
    case 9: // X8R8G8B8
        return 4;
    default:
        return 0;
    }
}

bool FormatHasAlpha(Sexy::PixelFormat theFormat)
{
    switch ((int)theFormat)
    {
    case 1: // LUT8
    case 2: // A8R8G8B8
    case 3: // A4R4G4B4
    case 4: // A1R5G5B5
    case 6: // A8B8G8R8
    case 7: // R4G4B4A4
    case 8: // R5G5B5A1
    case 10: // L8A8
        return true;
    default:
        return false;
    }
}

unsigned int UnpackPackedPixel(Sexy::PixelFormat theFormat, unsigned int thePacked)
{
    switch ((int)theFormat)
    {
    case 3: // A4R4G4B4
    case 7: // R4G4B4A4
    {
        int aBlue = (thePacked & 0xF) * 17;
        int aGreen = ((thePacked >> 4) & 0xF) * 17;
        int aRed = ((thePacked >> 8) & 0xF) * 17;
        int aAlpha = ((thePacked >> 12) & 0xF) * 17;
        if ((int)theFormat == 7)
        {
            aAlpha = (thePacked & 0xF) * 17;
            aBlue = ((thePacked >> 4) & 0xF) * 17;
            aGreen = ((thePacked >> 8) & 0xF) * 17;
            aRed = ((thePacked >> 12) & 0xF) * 17;
        }
        return ((unsigned int)aAlpha << 24) | ((unsigned int)aRed << 16)
             | ((unsigned int)aGreen << 8) | (unsigned int)aBlue;
    }
    case 4: // A1R5G5B5
    {
        int aBlue = ((thePacked & 0x1F) * 255) / 31;
        int aGreen = (((thePacked >> 5) & 0x1F) * 255) / 31;
        int aRed = (((thePacked >> 10) & 0x1F) * 255) / 31;
        int aAlpha = (thePacked & 0x8000) ? 255 : 0;
        return ((unsigned int)aAlpha << 24) | ((unsigned int)aRed << 16)
             | ((unsigned int)aGreen << 8) | (unsigned int)aBlue;
    }
    case 8: // R5G5B5A1
    {
        int aAlpha = (thePacked & 1) ? 255 : 0;
        int aBlue = (((thePacked >> 1) & 0x1F) * 255) / 31;
        int aGreen = (((thePacked >> 6) & 0x1F) * 255) / 31;
        int aRed = (((thePacked >> 11) & 0x1F) * 255) / 31;
        return ((unsigned int)aAlpha << 24) | ((unsigned int)aRed << 16)
             | ((unsigned int)aGreen << 8) | (unsigned int)aBlue;
    }
    case 5: // R5G6R5
    {
        int aBlue = ((thePacked & 0x1F) * 255) / 31;
        int aGreen = (((thePacked >> 5) & 0x3F) * 255) / 63;
        int aRed = (((thePacked >> 11) & 0x1F) * 255) / 31;
        return 0xFF000000u | ((unsigned int)aRed << 16)
             | ((unsigned int)aGreen << 8) | (unsigned int)aBlue;
    }
    case 9: // X8R8G8B8
        return (thePacked & 0x00FFFFFFu) | 0xFF000000u;
    case 6: // A8B8G8R8
        return ((thePacked & 0xFF00FF00u))
             | ((thePacked & 0x00FF0000u) >> 16)
             | ((thePacked & 0x000000FFu) << 16);
    default:
        return thePacked;
    }
}

int PremultiplyComponent(int theComponent, int theAlpha)
{
    return (theComponent * theAlpha + 128) >> 8;
}

int ScaleComponent(int theComponent, int theAlpha)
{
    return (theComponent * theAlpha + 127) >> 8;
}

unsigned int ScaleColor(int theRed, int theGreen, int theBlue, int theAlpha)
{
    return ((unsigned int)ScaleComponent(theRed, theAlpha) << 16)
         | ((unsigned int)ScaleComponent(theGreen, theAlpha) << 8)
         | (unsigned int)ScaleComponent(theBlue, theAlpha)
         | 0xFF000000u;
}

unsigned int SourceColor(const Sexy::Color & theColor)
{
    if (theColor.mAlpha == 255)
        return ScaleColor(theColor.mRed, theColor.mGreen, theColor.mBlue, 255);

    return ScaleColor(PremultiplyComponent(theColor.mRed, theColor.mAlpha),
                      PremultiplyComponent(theColor.mGreen, theColor.mAlpha),
                      PremultiplyComponent(theColor.mBlue, theColor.mAlpha), 255);
}

unsigned int BlendPixel(unsigned int theDest, unsigned int theSrc, int theAlpha)
{
    if (theAlpha == 255)
        return theSrc;

    int aInvAlpha = 256 - theAlpha;
    return ((unsigned int)ScaleComponent((int)(theDest >> 16) & 0xFF, aInvAlpha) << 16)
         + ((unsigned int)ScaleComponent((int)(theDest >> 8) & 0xFF, aInvAlpha) << 8)
         + theSrc
         + (unsigned int)ScaleComponent((int)theDest & 0xFF, aInvAlpha);
}

unsigned int BlendOver(unsigned int theDest, unsigned int theSrc, int theSrcAlpha)
{
    if (theSrcAlpha <= 0)
        return theDest;
    if (theSrcAlpha >= 255 && (theDest >> 24) == 255)
        return (theSrc & 0x00FFFFFFu) | 0xFF000000u;

    int aDestAlpha = (int)(theDest >> 24);
    int aNewAlpha = aDestAlpha + ((255 - aDestAlpha) * theSrcAlpha) / 255;
    if (aNewAlpha <= 0)
        return theDest;
    int aWeight = 255 * theSrcAlpha / aNewAlpha;
    int aInv = 256 - aWeight;
    unsigned int aBlue = (((theDest & 0xFFu) * (unsigned int)aInv) >> 8)
                       + (((theSrc & 0xFFu) * (unsigned int)aWeight) >> 8);
    unsigned int aGreen = (((theDest & 0xFF00u) * (unsigned int)aInv) >> 8)
                        + (((theSrc & 0xFF00u) * (unsigned int)aWeight) >> 8);
    unsigned int aRed = (((theDest & 0xFF0000u) * (unsigned int)aInv) >> 8)
                      + (((theSrc & 0xFF0000u) * (unsigned int)aWeight) >> 8);
    return (aBlue & 0xFFu) | (aGreen & 0xFF00u) | (aRed & 0xFF0000u)
         | ((unsigned int)aNewAlpha << 24);
}

unsigned int ColorizeSource(unsigned int theSrc, const Sexy::Color & theColor)
{
    if (theColor == Sexy::Color::White)
        return theSrc;
    unsigned int aRed = ((theSrc >> 16) & 0xFF) * (unsigned int)theColor.mRed / 255u;
    unsigned int aGreen = ((theSrc >> 8) & 0xFF) * (unsigned int)theColor.mGreen / 255u;
    unsigned int aBlue = (theSrc & 0xFF) * (unsigned int)theColor.mBlue / 255u;
    unsigned int aAlpha = (theSrc >> 24) * (unsigned int)theColor.mAlpha / 255u;
    return (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | aBlue;
}

unsigned int AdditivePixel(const unsigned char * theTable, unsigned int theDest, unsigned int theAdd)
{
    return (theDest & 0xFF000000u)
         | ((unsigned int)theTable[((theDest >> 16) & 0xFF) + ((theAdd >> 16) & 0xFF)] << 16)
         | ((unsigned int)theTable[((theDest >> 8) & 0xFF) + ((theAdd >> 8) & 0xFF)] << 8)
         | (unsigned int)theTable[(theDest & 0xFF) + (theAdd & 0xFF)];
}

unsigned int PremultiplyAdd(unsigned int theSrc, bool theHasAlpha)
{
    if (!theHasAlpha)
        return theSrc & 0x00FFFFFFu;
    unsigned int aAlpha = theSrc >> 24;
    return (((aAlpha * ((theSrc >> 16) & 0xFF)) >> 8) << 16)
         | (((aAlpha * ((theSrc >> 8) & 0xFF)) >> 8) << 8)
         | ((aAlpha * (theSrc & 0xFF)) >> 8);
}



int ClipSpan(int theValue, int theLimit)
{
    if (theValue < 0)
        return 0;
    if (theValue > theLimit)
        return theLimit;
    return theValue;
}

} // namespace

Sexy::MemoryImage * Sexy::MemoryImage::AsMemoryImage()
{
    return this;
}

int * Sexy::MemoryImage::Get3D()
{
    return nullptr;
}

bool Sexy::MemoryImage::CanFillPoly()
{
    return false;
}

bool Sexy::MemoryImage::CreateContext(Sexy::Image * theImage, const int & theContext)
{
    (void)theImage;
    (void)theContext;
    return true;
}

void Sexy::MemoryImage::DeleteContext(const int & theContext)
{
    (void)theContext;
}

void Sexy::MemoryImage::SetCurrentContext(const int & theContext)
{
    (void)theContext;
}

int Sexy::MemoryImage::GetCurrentContext() const
{
    return 1;
}

void Sexy::MemoryImage::PushState()
{
}

void Sexy::MemoryImage::PopState()
{
}

void Sexy::MemoryImage::SetImageMode(bool theHasTrans, bool theHasAlpha)
{
    mHasTrans = theHasTrans;
    mHasAlpha = theHasAlpha;
    mForcedMode = true;
}

void Sexy::MemoryImage::SetVolatile(bool theIsVolatile)
{
    mIsVolatile = theIsVolatile;
}

void * Sexy::MemoryImage::GetNativeAlphaData(int * theDisplay)
{
    (void)theDisplay;
    if (mNativeAlphaData != nullptr)
        return mNativeAlphaData;

    CommitBits();
    unsigned int * aBits = GetBits();
    if (aBits == nullptr || mWidth <= 0 || mHeight <= 0)
        return nullptr;

    int aCount = mWidth * mHeight;
    unsigned int * aNative = new unsigned int[(size_t)aCount];
    for (int i = 0; i < aCount; ++i)
    {
        unsigned int aPixel = aBits[i];
        int aAlpha = (int)(aPixel >> 24);
        unsigned int aRed = ((aPixel & 0xFF0000u) * (unsigned int)(aAlpha + 1)) >> 8;
        unsigned int aGreen = ((aPixel & 0x00FF00u) * (unsigned int)(aAlpha + 1)) >> 8;
        unsigned int aBlue = ((aPixel & 0x0000FFu) * (unsigned int)(aAlpha + 1)) >> 8;
        aNative[i] = (aRed & 0xFF0000u) | (aGreen & 0x00FF00u) | (aBlue & 0x0000FFu)
                   | ((unsigned int)aAlpha << 24);
    }
    mNativeAlphaData = aNative;
    return mNativeAlphaData;
}

unsigned char * Sexy::MemoryImage::GetRLAlphaData()
{
    CommitBits();
    if (mRLAlphaData != nullptr)
        return mRLAlphaData;
    if (mWidth <= 0 || mHeight <= 0)
        return nullptr;

    unsigned char * aRuns = new unsigned char[(size_t)mWidth * (size_t)mHeight];
    mRLAlphaData = aRuns;
    if (mWidth == 1)
    {
        std::memset(aRuns, 1, (size_t)mHeight);
        return mRLAlphaData;
    }

    unsigned int * aBits = GetBits();
    unsigned char * aWrite = aRuns;
    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        int aRead = 1;
        int aRun = 1;
        unsigned int aFirst = aBits != nullptr ? aBits[aRow * mWidth] : 0;
        int aAlpha = (int)(aFirst >> 24);
        int aLastClass = aAlpha == 0 ? 0 : (aAlpha == 255 ? 1 : 2);
        while (aRead < mWidth)
        {
            ++aRead;
            unsigned int aPixel = aBits != nullptr ? aBits[aRow * mWidth + aRead - 1] : 0;
            int aThisAlpha = (int)(aPixel >> 24);
            int aThisClass = aThisAlpha == 0 ? 0 : (aThisAlpha == 255 ? 1 : 2);
            if (aThisClass != aLastClass || aRead == mWidth)
            {
                if (aThisClass == aLastClass)
                    ++aRun;
                for (int aLeft = aRun; aLeft > 0; --aLeft)
                    *aWrite++ = (unsigned char)(aLeft >= 255 ? 255 : aLeft);
                if (aRead == mWidth && aThisClass != aLastClass)
                    *aWrite++ = 1;
                aLastClass = aThisClass;
                aRun = 1;
            }
            else
            {
                ++aRun;
            }
        }
    }
    return mRLAlphaData;
}

unsigned char * Sexy::MemoryImage::GetRLAdditiveData(int * theDisplay)
{
    (void)theDisplay;
    if (mRLAdditiveData != nullptr)
        return mRLAdditiveData;
    if (mWidth <= 0 || mHeight <= 0)
        return nullptr;

    unsigned int * aNative = (unsigned int *)GetNativeAlphaData(theDisplay);
    unsigned char * aRuns = new unsigned char[(size_t)mWidth * (size_t)mHeight];
    mRLAdditiveData = aRuns;
    if (mWidth == 1)
    {
        std::memset(aRuns, 1, (size_t)mHeight);
        return mRLAdditiveData;
    }

    unsigned char * aWrite = aRuns;
    for (int aRow = 0; aRow < mHeight; ++aRow)
    {
        int aRead = 1;
        int aRun = 1;
        unsigned int aFirst = aNative != nullptr ? aNative[aRow * mWidth] : 0;
        int aLastClass = (aFirst & 0xFFFFFFu) != 0 ? 1 : 0;
        while (aRead < mWidth)
        {
            ++aRead;
            unsigned int aPixel = aNative != nullptr ? aNative[aRow * mWidth + aRead - 1] : 0;
            int aThisClass = (aPixel & 0xFFFFFFu) != 0 ? 1 : 0;
            if (aThisClass != aLastClass || aRead == mWidth)
            {
                if (aThisClass == aLastClass)
                    ++aRun;
                for (int aLeft = aRun; aLeft > 0; --aLeft)
                    *aWrite++ = (unsigned char)(aLeft >= 255 ? 255 : aLeft);
                if (aRead == mWidth && aThisClass != aLastClass)
                    *aWrite++ = 1;
                aLastClass = aThisClass;
                aRun = 1;
            }
            else
            {
                ++aRun;
            }
        }
    }
    return mRLAdditiveData;
}

void Sexy::MemoryImage::DeleteExtraBuffers()
{
    DeleteSWBuffers();
    Delete3DBuffers();
}

Sexy::PixelFormat Sexy::MemoryImage::GetPixelFormat()
{
    if (mParent != nullptr)
        return mParent->GetPixelFormat();
    return mFormat;
}

void Sexy::MemoryImage::Clear()
{
    TRect<int> aRect(0, 0, mWidth, mHeight);
    ClearRect(aRect);
}

void Sexy::MemoryImage::BltF(Sexy::Image * theSource, float theX, float theY, const TRect<int> & theSourceRect,
                             const TRect<int> & theClipRect, const Color & theColor, int theDrawMode)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    BltRotated(theSource, theX, theY, theSourceRect, theClipRect, theColor, theDrawMode, 0, 0, 0);
}

void Sexy::MemoryImage::BltTriangles(Sexy::Image * theSource, const SexyVertex2D (*theTriangles)[3],
                                     int theTriangleCount, const Color & theColor, int theDrawMode,
                                     float theTx, float theTy, bool theBlend, const TRect<int> * theClipRect)
{
    TRect<int> aClipRect(0, 0, mWidth, mHeight);
    if (theClipRect != nullptr)
        aClipRect = *theClipRect;

    BltTrianglesTex(theSource, theTriangles, theTriangleCount, aClipRect, theColor, theDrawMode,
                    theTx, theTy, theBlend);
}

void Sexy::MemoryImage::NormalStretchBltMirror(Sexy::Image * theSource, const TRect<int> & theDestRect,
                                               const TRect<int> & theSrcRect, const TRect<int> & theClipRect,
                                               const Color & theColor, bool theFastStretch)
{
    StretchBltMirror(theSource, theDestRect, theSrcRect, theClipRect, theColor, kDrawNormal, theFastStretch);
}

void Sexy::MemoryImage::AdditiveStretchBltMirror(Sexy::Image * theSource, const TRect<int> & theDestRect,
                                                 const TRect<int> & theSrcRect, const TRect<int> & theClipRect,
                                                 const Color & theColor, bool theFastStretch)
{
    StretchBltMirror(theSource, theDestRect, theSrcRect, theClipRect, theColor, kDrawAdditive, theFastStretch);
}

void Sexy::MemoryImage::FillPoly(const TPoint<int> * thePoints, int thePointCount, const TRect<int> * theClipRect,
                                 const Color & theColor, int theDrawMode, int theTx, int theTy)
{
    PolyFill3D(thePoints, thePointCount, theClipRect, theColor, theDrawMode, theTx, theTy, false);
}

unsigned int * Sexy::MemoryImage::GetPallete()
{
    return mColorTable;
}

void Sexy::MemoryImage::ClearRect(const TRect<int> & theRect)
{
    unsigned int * aBits = GetBits();
    if (aBits == nullptr)
        return;

    int aPitch = GetPitch();
    int aLeft = ClipSpan(theRect.mX, mWidth);
    int aTop = ClipSpan(theRect.mY, mHeight);
    int aRight = ClipSpan(theRect.mX + theRect.mWidth, mWidth);
    int aBottom = ClipSpan(theRect.mY + theRect.mHeight, mHeight);
    for (int aRow = aTop; aRow < aBottom; ++aRow)
        std::memset(aBits + aLeft + aPitch * aRow, 0, (size_t)(aRight - aLeft) * 4);

    BitsChanged();
}

void Sexy::MemoryImage::BitsChanged()
{
    FilterEffectDisposeForImage(this);
    ++mBitsChangedCount;
    mBitsChanged = true;

    if (mParent != nullptr)
        mParent->BitsChanged();

    delete[] mNativeAlphaData;
    mNativeAlphaData = nullptr;
    delete[] mRLAlphaData;
    mRLAlphaData = nullptr;
    delete[] mRLAdditiveData;
    mRLAdditiveData = nullptr;
}

void Sexy::MemoryImage::DeleteSWBuffers()
{
    if (mNativeAlphaData != nullptr || mRLAdditiveData != nullptr || mRLAlphaData != nullptr)
    {
        if (mBits == nullptr && mColorIndices == nullptr && mPixels == nullptr)
            GetBits();

        delete[] mNativeAlphaData;
        mNativeAlphaData = nullptr;
        delete[] mRLAdditiveData;
        mRLAdditiveData = nullptr;
        delete[] mRLAlphaData;
        mRLAlphaData = nullptr;
    }
}

void Sexy::MemoryImage::DeleteNativeData()
{
    if (mBits == nullptr && mColorIndices == nullptr)
        GetBits();

    delete[] mNativeAlphaData;
    mNativeAlphaData = nullptr;
    delete[] mRLAdditiveData;
    mRLAdditiveData = nullptr;
}

void Sexy::MemoryImage::TakeBits(unsigned int * theBits, int theWidth, int theHeight, bool theCommitBits)
{
    delete[] mColorIndices;
    mColorIndices = nullptr;
    delete[] mColorTable;
    mColorTable = nullptr;
    if (mBits != nullptr && mBits != (unsigned long *)theBits)
        delete[] mBits;

    mWidth = theWidth;
    mHeight = theHeight;
    mPitch = theWidth;
    mStride = 4 * theWidth;
    mFormat = PIXEL_FORMAT_A8R8G8B8;

    unsigned int * aBits = theBits;
    if (theBits != nullptr)
    {
        mBits = (unsigned long *)theBits;
    }
    else
    {
        int aCount = theHeight * theWidth;
        aBits = new unsigned int[(size_t)aCount + 1];
        mBits = (unsigned long *)aBits;
        std::memset(aBits, 0, (size_t)aCount * 4);
    }

    if (aBits != nullptr && theWidth > 0 && theHeight > 0)
        aBits[theHeight * theWidth] = kMemoryCheckId;
    BitsChanged();
    if (theCommitBits)
        CommitBits();
}

void Sexy::MemoryImage::SetBits(unsigned int * theBits, int theWidth, int theHeight, bool theOwnsBits)
{
    if (mBits == (unsigned long *)theBits && mWidth == theWidth && mHeight == theHeight)
        return;

    delete[] mColorIndices;
    mColorIndices = nullptr;
    delete[] mColorTable;
    mColorTable = nullptr;

    int aPixelCount = theHeight * theWidth;
    unsigned int * aBits;
    if (mWidth == theWidth && mHeight == theHeight && mBits != nullptr)
    {
        aBits = (unsigned int *)mBits;
    }
    else
    {
        delete[] mBits;
        aBits = new unsigned int[(size_t)aPixelCount + 1];
        mBits = (unsigned long *)aBits;
        mWidth = theWidth;
        mHeight = theHeight;
        mPitch = theWidth;
        mStride = 4 * theWidth;
    }

    if (theBits != nullptr)
        std::memcpy(aBits, theBits, (size_t)aPixelCount * 4);
    else
        std::memset(aBits, 0, (size_t)aPixelCount * 4);

    if (aPixelCount >= 0)
        aBits[aPixelCount] = kMemoryCheckId;
    mFormat = PIXEL_FORMAT_A8R8G8B8;

    BitsChanged();
    if (theOwnsBits)
        CommitBits();
}

void Sexy::MemoryImage::Create(int theWidth, int theHeight)
{
    Detach(false);

    delete[] mColorIndices;
    mColorIndices = nullptr;
    delete[] mColorTable;
    mColorTable = nullptr;
    delete[] mNativeAlphaData;
    mNativeAlphaData = nullptr;
    delete[] mBits;
    mBits = nullptr;

    mFormat = PIXEL_FORMAT_A8R8G8B8;
    mStride = 4 * theWidth;
    mWidth = theWidth;
    mHeight = theHeight;
    mPitch = theWidth;
    mHasTrans = true;
    mHasAlpha = true;

    BitsChanged();
}

void Sexy::MemoryImage::ForcePalletize()
{
    if (mColorIndices == nullptr && mWidth > 0 && mHeight > 0)
        mColorIndices = new unsigned char[(size_t)mHeight * (size_t)mWidth];

    if (mColorTable == nullptr)
        mColorTable = new unsigned int[256];
    std::memset(mColorTable, 0, 256 * sizeof(unsigned int));

    delete[] mBits;
    mBits = nullptr;
    delete[] mNativeAlphaData;
    mNativeAlphaData = nullptr;

    mWantPal = true;
    mPitch = mWidth;
    mStride = mWidth;
    mFormat = PIXEL_FORMAT_LUT8;
}

void Sexy::MemoryImage::DetachSubImages(bool theCopyBits)
{
    for (;;)
    {
        mSubImagesLock.Lock();
        if (mSubImages.empty())
        {
            mSubImagesLock.Unlock();
            return;
        }
        MemoryImage * aSubImage = *mSubImages.begin();
        mSubImagesLock.Unlock();
        if (aSubImage == nullptr)
            return;
        aSubImage->Detach(theCopyBits);
    }
}

void Sexy::MemoryImage::FillRect(const TRect<int> & theRect, const Color & theColor, int theDrawMode)
{
    (void)theDrawMode;
    unsigned int * aBits = GetBits();
    if (aBits == nullptr)
        return;

    unsigned int aColor = (unsigned int)theColor.ToInt();
    int aAlpha = (int)(aColor >> 24);
    int aLeft = ClipSpan(theRect.mX, mWidth);
    int aTop = ClipSpan(theRect.mY, mHeight);
    int aRight = ClipSpan(theRect.mX + theRect.mWidth, mWidth);
    int aBottom = ClipSpan(theRect.mY + theRect.mHeight, mHeight);

    for (int aRow = aTop; aRow < aBottom; ++aRow)
    {
        unsigned int * aDest = aBits + aLeft + mWidth * aRow;
        int aCount = aRight - aLeft;
        if (aAlpha == 255)
        {
            for (int aCol = 0; aCol < aCount; ++aCol)
                *aDest++ = aColor;
        }
        else if (aAlpha != 0)
        {
            for (int aCol = 0; aCol < aCount; ++aCol)
            {
                *aDest = BlendOver(*aDest, aColor, aAlpha);
                ++aDest;
            }
        }
    }

    BitsChanged();
}

void Sexy::MemoryImage::FillScanLinesWithCoverageRenderDevice(Sexy::RenderDevice::Span * theSpans, int theSpanCount,
                                                              const Color & theColor, int theDrawMode,
                                                              const unsigned char * theCoverage, int theCoverageX,
                                                              int theCoverageY, int theCoverageWidth,
                                                              int theCoverageHeight)
{
    (void)theDrawMode;
    (void)theCoverageHeight;
    unsigned int * aBits = GetBits();
    if (aBits == nullptr || theSpans == nullptr)
        return;

    int aPitch = GetPitch();
    unsigned int aColor = (unsigned int)theColor.ToInt();
    int aColorAlpha = theColor.mAlpha;

    for (int aSpan = 0; aSpan < theSpanCount; ++aSpan)
    {
        int aSpanY = theSpans[aSpan].mY;
        int aSpanX = theSpans[aSpan].mX;
        int aSpanLength = theSpans[aSpan].mCount;
        if (aSpanY < 0 || aSpanY >= mHeight)
            continue;
        unsigned int * aDest = aBits + aSpanX + aSpanY * aPitch;
        const unsigned char * aCoverage = nullptr;
        if (theCoverage != nullptr)
            aCoverage = theCoverage + (aSpanX - theCoverageX) + theCoverageWidth * (aSpanY - theCoverageY);

        for (int aPixel = 0; aPixel < aSpanLength; ++aPixel)
        {
            if (aSpanX + aPixel < 0 || aSpanX + aPixel >= mWidth)
            {
                ++aDest;
                continue;
            }
            int aCover = aCoverage != nullptr ? aCoverage[aPixel] : 255;
            int aAlpha = (aColorAlpha * aCover) >> 8;
            if (aAlpha > 0)
                *aDest = BlendOver(*aDest, aColor, aAlpha);
            ++aDest;
        }
    }

    BitsChanged();
}

void Sexy::MemoryImage::Delete3DBuffers()
{
    if (mApp != nullptr)
        mApp->Remove3DData(this);
}

void Sexy::MemoryImage::TakePixels(Sexy::PixelFormat theFormat, void * thePixels, int theWidth, int theHeight,
                                   bool theCommitBits)
{
    delete[] mColorIndices;
    mColorIndices = nullptr;
    delete[] mColorTable;
    mColorTable = nullptr;
    delete[] mBits;
    mBits = nullptr;
    delete[] mPixels;
    mPixels = nullptr;

    mWidth = theWidth;
    mHeight = theHeight;
    mPitch = theWidth;
    mStride = FormatBytesPerPixel(theFormat) * theWidth;
    mFormat = theFormat;

    if (theFormat == PIXEL_FORMAT_A8R8G8B8)
        mBits = (unsigned long *)thePixels;
    else
        mPixels = (unsigned int *)thePixels;

    BitsChanged();
    if (theCommitBits)
        CommitBits();
}

unsigned int * Sexy::MemoryImage::GetPixels()
{
    if (mParent != nullptr)
    {
        unsigned int * aPixels = mParent->GetPixels();
        if (aPixels == nullptr)
            return nullptr;
        int aBytePerPixel = FormatBytesPerPixel(mParent->GetPixelFormat());
        int aByteOffset = (mParentX + mParentY * mParent->GetPitch()) * aBytePerPixel;
        return (unsigned int *)((char *)aPixels + aByteOffset);
    }

    if (mPixels != nullptr)
        return mPixels;
    if (mColorIndices != nullptr)
        return (unsigned int *)mColorIndices;
    return GetBits();
}

void Sexy::MemoryImage::Finalize()
{
    if (mParent != nullptr || mPixels != nullptr || mColorTable != nullptr || mIsVolatile)
        return;
    if (mWidth == 0 || mHeight == 0 || mBits == nullptr)
        return;

    CommitBits();

    if ((int)mPixelFormatHint <= PIXEL_FORMAT_A8R8G8B8 || mPixelFormatHint == PIXEL_FORMAT_R5G5B5A1
        || mPixelFormatHint == PIXEL_FORMAT_A8B8G8R8)
        return;

    int aBytePerPixel = FormatBytesPerPixel(mPixelFormatHint);
    if (aBytePerPixel == 0)
        return;

    int aByteCount = (mHeight * mWidth + 1) * aBytePerPixel;
    mPixels = (unsigned int *)new unsigned char[(size_t)aByteCount];
    std::memset(mPixels, 0, (size_t)aByteCount);

    PixelData aPixelsData(mPixelFormatHint, mWidth, mHeight, mWidth * aBytePerPixel, mPixels);
    PixelData aBitsData(PIXEL_FORMAT_A8R8G8B8, mWidth, mHeight, mStride, (void *)mBits);
    aPixelsData.Convert(aBitsData);

    delete[] mBits;
    mBits = nullptr;
    mPitch = mWidth;
    mFormat = mPixelFormatHint;
    mStride = mWidth * aBytePerPixel;
}

void Sexy::MemoryImage::DrawLineEx(double theStartX, double theStartY, double theEndX, double theEndY,
                                   const Color & theColor, int theDrawMode, bool theAntiAlias)
{
    if (theAntiAlias)
        DrawLineAA(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
    else
        DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void Sexy::MemoryImage::ReInit()
{
    if (mWantPal)
        Palletize();
    if (mPurgeBits)
        PurgeBits();
}

void Sexy::MemoryImage::BltStretched(Sexy::Image * theSource, const TRect<int> & theDestRect,
                                     const TRect<int> & theSrcRect, const TRect<int> & theClipRect,
                                     const Color & theColor, int theDrawMode, bool theFastStretch, bool theMirror)
{
    if (theMirror)
        StretchBltMirror(theSource, theDestRect, theSrcRect, theClipRect, theColor, theDrawMode, theFastStretch);
    else
        StretchBlt(theSource, theDestRect, theSrcRect, theClipRect, theColor, theDrawMode, theFastStretch);
}

void Sexy::MemoryImage::CommitBits()
{
    if (!mBitsChanged || mForcedMode)
        return;

    if (mParent != nullptr)
    {
        mParent->CommitBits();
        mHasTrans = mParent->mHasTrans;
        mHasAlpha = mParent->mHasAlpha;
        mIsAlphaOnly = mParent->mIsAlphaOnly;
        mBitsChanged = false;
        return;
    }

    mHasTrans = false;
    mHasAlpha = false;
    mIsAlphaOnly = false;

    if (mBits != nullptr)
    {
        for (int aRow = 0; aRow < mHeight && !(mHasTrans && mHasAlpha); ++aRow)
        {
            unsigned long * aPixel = mBits + mPitch * aRow;
            for (int aCol = 0; aCol < mWidth; ++aCol)
            {
                unsigned long aAlpha = *aPixel++ >> 24;
                if (aAlpha == 0)
                    mHasTrans = true;
                else if (aAlpha != 255)
                    mHasAlpha = true;
            }
        }
    }
    else if (mColorTable != nullptr)
    {
        for (int i = 0; i < 256; ++i)
        {
            unsigned int aAlpha = mColorTable[i] >> 24;
            if (aAlpha == 0)
                mHasTrans = true;
            else if (aAlpha != 255)
                mHasAlpha = true;
        }

        if (mHasAlpha)
        {
            int aEntry = 0;
            for (; aEntry < 256; ++aEntry)
                if ((mColorTable[aEntry] & 0xFFFFFFu) != 0xFFFFFFu)
                    break;
            if (aEntry == 256)
                mIsAlphaOnly = true;
        }
    }
    else if (mPixels == nullptr)
    {
        mHasAlpha = false;
        mHasTrans = true;
    }
    else if (mFormat == PIXEL_FORMAT_R5G5B5A1 || mFormat == PIXEL_FORMAT_A1R5G5B5)
    {
        mIsAlphaOnly = false;
        mHasAlpha = false;
        mHasTrans = true;
    }
    else if (FormatHasAlpha(mFormat))
    {
        for (int aRow = 0; aRow < mHeight && !(mHasTrans && mHasAlpha); ++aRow)
        {
            for (int aCol = 0; aCol < mWidth; ++aCol)
            {
                unsigned int aAlpha = GetPixel(aCol, aRow) >> 24;
                if (aAlpha == 0)
                    mHasTrans = true;
                else if (aAlpha != 255)
                    mHasAlpha = true;
            }
        }
    }

    mBitsChanged = false;
}

void Sexy::MemoryImage::Palletize()
{
    CommitBits();

    if (mParent != nullptr || mColorTable != nullptr || mForcedMode)
        return;
    if (FormatBytesPerPixel(GetPixelFormat()) != 4)
        return;

    GetBits();
    if (mBits == nullptr || mWidth <= 0 || mHeight <= 0)
        return;

    int aCount = mHeight * mWidth;
    unsigned char * aIndices = new unsigned char[(size_t)aCount];
    unsigned int * aTable = new unsigned int[256];
    std::memset(aTable, 0, 256 * sizeof(unsigned int));

    unsigned int aUnused = 0;
    bool aPalettized = Sexy::Quantize8Bit(mBits, mWidth, mHeight, aIndices,
                                          (unsigned long *)aTable, &aUnused);
    if (aPalettized)
    {
        mColorIndices = aIndices;
        mColorTable = aTable;
        delete[] mBits;
        mBits = nullptr;
        delete[] mNativeAlphaData;
        mNativeAlphaData = nullptr;
        mWantPal = true;
        mPitch = mWidth;
        mStride = mWidth;
        mFormat = PIXEL_FORMAT_LUT8;
    }
    else
    {
        delete[] aIndices;
        delete[] aTable;
        mWantPal = false;
    }
}

unsigned int Sexy::MemoryImage::GetPixel(int theX, int theY)
{
    if (mParent != nullptr)
        return mParent->GetPixel(theX + mParentX, theY + mParentY);

    if (mColorTable != nullptr && mColorIndices != nullptr)
        return mColorTable[mColorIndices[mWidth * theY + theX]];

    unsigned int * aPixels = GetPixels();
    if (aPixels == nullptr)
        return 0;
    PixelFormat aFormat = GetPixelFormat();
    int aPitch = GetPitch();
    if (aFormat == PIXEL_FORMAT_A8R8G8B8)
        return aPixels[theX + aPitch * theY];

    int aBytes = FormatBytesPerPixel(aFormat);
    const unsigned char * aBytesPtr = (const unsigned char *)aPixels + aBytes * (theX + aPitch * theY);
    unsigned int aPacked = 0;
    for (int i = 0; i < aBytes && i < 4; ++i)
        aPacked |= (unsigned int)aBytesPtr[i] << (8 * i);
    return UnpackPackedPixel(aFormat, aPacked);
}

void Sexy::MemoryImage::PurgeBits()
{
    mPurgeBits = true;

    if (mPixelsLive == 0)
        return;

    if (mApp != nullptr)
    {
        Sexy::NativeDisplay * aDisplay = mApp->mPlatformDriverOrQueue;
        if (aDisplay != nullptr && aDisplay->mRetainSoftwareBits)
        {
            if (!mApp->mSafeReload)
                return;
            if (mFileName.empty())
                return;
        }
    }

    if (mParent != nullptr)
    {
        if (mBits == nullptr || mColorIndices == nullptr)
            return;
    }
    else if (mBits == nullptr && mColorIndices == nullptr)
    {
        return;
    }

    delete[] mBits;
    mBits = nullptr;
    delete[] mColorIndices;
    mColorIndices = nullptr;
    delete[] mColorTable;
    mColorTable = nullptr;
}

unsigned int * Sexy::MemoryImage::GetBits()
{
    if (mParent != nullptr)
    {
        unsigned int * aParentBits = mParent->GetBits();
        if (aParentBits != nullptr)
            aParentBits += mParentX + mParentY * mParent->GetPitch();
        return aParentBits;
    }

    if (mBits != nullptr)
        return (unsigned int *)mBits;

    PixelFormat aOldFormat = mFormat;
    int aOldStride = mStride;
    int aPixelCount = mHeight * mWidth;
    if (aPixelCount < 0)
        aPixelCount = 0;

    unsigned int * aBits = new unsigned int[(size_t)aPixelCount + 1];
    mBits = (unsigned long *)aBits;
    aBits[aPixelCount] = kMemoryCheckId;
    mFormat = PIXEL_FORMAT_A8R8G8B8;
    mPitch = mWidth;
    mStride = 4 * mWidth;

    if (mColorTable != nullptr && mColorIndices != nullptr)
    {
        for (int i = 0; i < aPixelCount; ++i)
            aBits[i] = mColorTable[mColorIndices[i]];

        delete[] mColorIndices;
        mColorIndices = nullptr;
        delete[] mColorTable;
        mColorTable = nullptr;
        delete[] mNativeAlphaData;
        mNativeAlphaData = nullptr;
        return (unsigned int *)mBits;
    }

    if (mPixels != nullptr)
    {
        PixelData aPixelsData(aOldFormat, mWidth, mHeight, aOldStride, mPixels);
        PixelData aBitsData(PIXEL_FORMAT_A8R8G8B8, mWidth, mHeight, mStride, aBits);
        aBitsData.Convert(aPixelsData);

        delete[] mPixels;
        mPixels = nullptr;
        return (unsigned int *)mBits;
    }

    bool aRecovered = false;
    if (mPixelsLive != 0 && mApp != nullptr)
    {
        Sexy::NativeDisplay * aDisplay = mApp->mPlatformDriverOrQueue;
        if (aDisplay != nullptr && aDisplay->RecoverBits(this))
            aRecovered = true;
    }
    if (!aRecovered && mApp != nullptr && mApp->mSafeReload && !mFileName.empty())
    {
        if (mApp->ReloadImage(this))
            aRecovered = true;
    }
    if (!aRecovered)
        std::memset(aBits, 0, (size_t)aPixelCount * 4);
    return (unsigned int *)mBits;
}

void Sexy::MemoryImage::Init()
{
    mPixels = nullptr;
    mBits = nullptr;
    mColorTable = nullptr;
    mColorIndices = nullptr;
    mNativeAlphaData = nullptr;
    mRLAlphaData = nullptr;
    mRLAdditiveData = nullptr;
    mHasTrans = false;
    mHasAlpha = false;
    mIsAlphaOnly = false;
    mBitsChanged = false;
    mForcedMode = false;
    mIsVolatile = false;
    mD3DData = nullptr;
    mD3DFlags = 0;
    mSpareHeight = 0;
    mBitsChangedCount = 0;
    mPurgeBits = false;
    mWantPal = false;
    mParent = nullptr;
    mParentX = 0;
    mParentY = 0;

    if (mApp != nullptr)
        mApp->AddMemoryImage(this);
}

void Sexy::MemoryImage::NormalDrawLine(double theStartX, double theStartY, double theEndX, double theEndY,
                                       const Color & theColor)
{
    double aMinX = theStartX <= theEndX ? theStartX : theEndX;
    double aMinY = theStartY <= theEndY ? theStartY : theEndY;
    double aMaxX = theStartX >= theEndX ? theStartX : theEndX;
    double aMaxY = theStartY >= theEndY ? theStartY : theEndY;

    unsigned int * aBits = GetBits();
    if (aBits == nullptr)
        return;

    int aAlpha = theColor.mAlpha;
    unsigned int aSource = SourceColor(theColor);
    int aPitch = GetPitch();

    double aStartX = theStartX;
    double aStartY = theStartY;
    double aEndX = theEndX;
    double aEndY = theEndY;
    double aDeltaX = theEndX - theStartX;
    double aDeltaY = theEndY - theStartY;

    if (std::fabs(aDeltaY) >= std::fabs(aDeltaX))
    {
        if (aDeltaY < 0.0)
        {
            aDeltaY = -aDeltaY;
            aStartY = theEndY;
            aDeltaX = -aDeltaX;
            aEndY = theStartY;
            aStartX = theEndX;
        }

        int aXStep = 1;
        if (aDeltaX < 0.0)
        {
            aXStep = -1;
            aDeltaX = -aDeltaX;
        }

        int aX = (int)aStartX;
        int aIndex = aX + (int)aStartY * aPitch;
        int aRow = (int)(aStartY + 1.0);
        double aY = (double)aRow;
        unsigned int * aPixel = aBits + aIndex + aPitch;
        int aTwiceDeltaX = (int)(aDeltaX + aDeltaX);
        int aError = (int)((double)aTwiceDeltaX * (aStartX - (double)aX))
                   + (int)((aDeltaX + aDeltaX) - aDeltaY);
        int aErrorStep = (int)((aDeltaX - aDeltaY) + (aDeltaX - aDeltaY));

        if ((int)aStartY >= 0 && (int)aStartY < mHeight && aX >= 0 && aX < mWidth)
            aBits[aIndex] = BlendPixel(aBits[aIndex], aSource, aAlpha);

        if (aEndY >= aY)
        {
            do
            {
                ++aRow;
                if (aError > 0)
                {
                    aX += aXStep;
                    aError += aErrorStep;
                    aPixel += aXStep;
                    if ((double)aX < aMinX || aMinY > aY || (double)aX > aMaxX || aMaxY < aY)
                        return;
                }
                else
                {
                    aError += aTwiceDeltaX;
                }

                aY = (double)aRow;
                if (aRow - 1 >= 0 && aRow - 1 < mHeight && aX >= 0 && aX < mWidth)
                    *aPixel = BlendPixel(*aPixel, aSource, aAlpha);
                aPixel += aPitch;
            }
            while (aEndY >= (double)aRow);
        }
    }
    else
    {
        if (aDeltaX < 0.0)
        {
            double aOldStartX = aStartX;
            aDeltaX = -aDeltaX;
            aStartX = theEndX;
            aDeltaY = -aDeltaY;
            aEndX = aOldStartX;
            aStartY = theEndY;
        }

        int aRowStep = aPitch;
        int aYStep = 1;
        if (aDeltaY < 0.0)
        {
            aRowStep = -aPitch;
            aYStep = -1;
            aDeltaY = -aDeltaY;
        }

        int aY = (int)aStartY;
        int aIntStartX = (int)aStartX;
        int aIndex = aIntStartX + aY * aPitch;
        int aTwiceDeltaY = (int)(aDeltaY + aDeltaY);
        unsigned int * aPixel = aBits + aIndex + 1;
        int aCol = (int)(aStartX + 1.0);
        double aX = (double)aCol;
        double aFraction = aAlpha == 255 ? aStartY - (double)aY : aStartX - (double)aIntStartX;
        int aError = (int)((double)aTwiceDeltaY * aFraction)
                   + (int)((aDeltaY + aDeltaY) - aDeltaX);
        int aErrorStep = (int)((aDeltaY - aDeltaX) + (aDeltaY - aDeltaX));

        if (aY >= 0 && aY < mHeight && aIntStartX >= 0 && aIntStartX < mWidth)
            aBits[aIndex] = BlendPixel(aBits[aIndex], aSource, aAlpha);

        if (aEndX >= aX)
        {
            do
            {
                ++aCol;
                unsigned int * aNext = aPixel + aRowStep;
                if (aError > 0)
                {
                    aY += aYStep;
                    aError += aErrorStep;
                    if (aMinX > aX || (double)aY < aMinY || aMaxX < aX || (double)aY > aMaxY)
                        return;
                }
                else
                {
                    aError += aTwiceDeltaY;
                    aNext = aPixel;
                }

                aPixel = aNext + 1;
                aX = (double)aCol;
                int aDrawX = aCol - 1;
                if (aY >= 0 && aY < mHeight && aDrawX >= 0 && aDrawX < mWidth)
                    *aNext = BlendPixel(*aNext, aSource, aAlpha);
            }
            while (aEndX >= (double)aCol);
        }
    }
}

void Sexy::MemoryImage::AdditiveDrawLine(double theStartX, double theStartY, double theEndX, double theEndY,
                                         const Color & theColor)
{
    unsigned int * aBits = GetBits();
    if (aBits == nullptr || mApp == nullptr)
        return;

    const unsigned char * aTable = mApp->mAdd8BitMaxTable;
    int aAddRed = theColor.mRed * theColor.mAlpha / 255;
    int aAddGreen = theColor.mGreen * theColor.mAlpha / 255;
    int aAddBlue = theColor.mBlue * theColor.mAlpha / 255;
    unsigned int aAdd = ((unsigned int)aAddRed << 16) | ((unsigned int)aAddGreen << 8) | (unsigned int)aAddBlue;

    double aMinX = theStartX <= theEndX ? theStartX : theEndX;
    double aMinY = theStartY <= theEndY ? theStartY : theEndY;
    double aMaxX = theStartX >= theEndX ? theStartX : theEndX;
    double aMaxY = theStartY >= theEndY ? theStartY : theEndY;
    int aPitch = GetPitch();

    double aStartX = theStartX;
    double aStartY = theStartY;
    double aEndX = theEndX;
    double aEndY = theEndY;
    double aDeltaX = theEndX - theStartX;
    double aDeltaY = theEndY - theStartY;

    auto aStore = [aTable, aAdd](unsigned int theDest) -> unsigned int
    {
        return AdditivePixel(aTable, theDest, aAdd) | 0xFF000000u;
    };

    if (std::fabs(aDeltaY) >= std::fabs(aDeltaX))
    {
        if (aDeltaY < 0.0)
        {
            aDeltaY = -aDeltaY;
            aStartY = aEndY;
            aDeltaX = -aDeltaX;
            aEndY = theStartY;
            aStartX = aEndX;
        }
        int aXStep = aDeltaX < 0.0 ? -1 : 1;
        if (aDeltaX < 0.0)
            aDeltaX = -aDeltaX;

        int aX = (int)aStartX;
        int aIndex = aX + mWidth * (int)aStartY;
        int aRow = (int)(aStartY + 1.0);
        unsigned int * aPixel = aBits + aIndex + aPitch;
        int aTwiceDeltaX = (int)(aDeltaX + aDeltaX);
        int aError = (int)((aDeltaX + aDeltaX) - aDeltaY);
        int aErrorStep = (int)((aDeltaX - aDeltaY) + (aDeltaX - aDeltaY));
        if ((int)aStartY >= 0 && (int)aStartY < mHeight && aX >= 0 && aX < mWidth)
            aBits[aIndex] = aStore(aBits[aIndex]);

        while (aEndY >= (double)aRow)
        {
            ++aRow;
            if (aError > 0)
            {
                aX += aXStep;
                aError += aErrorStep;
                aPixel += aXStep;
                if ((double)aX < aMinX || aMinY > (double)(aRow - 1) || (double)aX > aMaxX || aMaxY < (double)(aRow - 1))
                    return;
            }
            else
            {
                aError += aTwiceDeltaX;
            }
            if (aRow - 1 >= 0 && aRow - 1 < mHeight && aX >= 0 && aX < mWidth)
                *aPixel = aStore(*aPixel);
            aPixel += aPitch;
        }
    }
    else
    {
        if (aDeltaX < 0.0)
        {
            aDeltaX = -aDeltaX;
            aStartX = aEndX;
            aDeltaY = -aDeltaY;
            aEndX = theStartX;
            aStartY = aEndY;
        }
        int aRowStep = aPitch;
        int aYStep = 1;
        if (aDeltaY < 0.0)
        {
            aRowStep = -aPitch;
            aYStep = -1;
            aDeltaY = -aDeltaY;
        }
        int aY = (int)aStartY;
        int aX = (int)aStartX;
        int aIndex = aX + aY * aPitch;
        unsigned int * aPixel = aBits + aIndex + 1;
        int aCol = aX + 1;
        int aTwiceDeltaY = (int)(aDeltaY + aDeltaY);
        int aError = (int)((aDeltaY + aDeltaY) - aDeltaX);
        int aErrorStep = (int)((aDeltaY - aDeltaX) + (aDeltaY - aDeltaX));
        if (aY >= 0 && aY < mHeight && aX >= 0 && aX < mWidth)
            aBits[aIndex] = aStore(aBits[aIndex]);

        while (aEndX >= (double)aCol)
        {
            ++aCol;
            unsigned int * aNext = aPixel + aRowStep;
            if (aError > 0)
            {
                aY += aYStep;
                aError += aErrorStep;
                if (aMinX > (double)(aCol - 1) || (double)aY < aMinY || aMaxX < (double)(aCol - 1) || (double)aY > aMaxY)
                    return;
            }
            else
            {
                aError += aTwiceDeltaY;
                aNext = aPixel;
            }
            aPixel = aNext + 1;
            if (aY >= 0 && aY < mHeight && aCol - 1 >= 0 && aCol - 1 < mWidth)
                *aNext = aStore(*aNext);
        }
    }
}

void Sexy::MemoryImage::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY,
                                 const Color & theColor, int theDrawMode)
{
    if (theStartY == theEndY)
    {
        double aMinX = theStartX <= theEndX ? theStartX : theEndX;
        double aMaxX = theStartX < theEndX ? theEndX : theStartX;
        TRect<int> aRect((int)aMinX, (int)theEndY, (int)aMaxX - (int)aMinX + 1, (int)(1.0 - theEndY + theEndY));
        FillRect(aRect, theColor, theDrawMode);
        return;
    }

    if (theStartX == theEndX)
    {
        double aMinY = theEndY <= theStartY ? theEndY : theStartY;
        double aMaxY = theEndY < theStartY ? theStartY : theEndY;
        TRect<int> aRect((int)theStartX, (int)aMinY, (int)(theStartX + 1.0 - theStartX), (int)aMaxY - (int)aMinY + 1);
        FillRect(aRect, theColor, theDrawMode);
        return;
    }

    if (theDrawMode == kDrawNormal)
        NormalDrawLine(theStartX, theStartY, theEndX, theEndY, theColor);
    else if (theDrawMode == kDrawAdditive)
        AdditiveDrawLine(theStartX, theStartY, theEndX, theEndY, theColor);

    BitsChanged();
}

void Sexy::MemoryImage::NormalDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY,
                                         const Color & theColor)
{
    unsigned int * aBits = GetBits();
    if (aBits == nullptr)
        return;

    double aX0 = theStartX;
    double aY0 = theStartY;
    double aX1 = theEndX;
    double aY1 = theEndY;
    if ((int)theStartY > (int)theEndY)
    {
        aX0 = theEndX;
        aY0 = theEndY;
        aX1 = theStartX;
        aY1 = theStartY;
    }

    int aPitch = GetPitch();
    unsigned int aPacked = (unsigned int)theColor.ToInt();
    int aSteps = (int)aY1 - (int)aY0;
    if (aSteps < 1)
        aSteps = 1;
    double aStepX = (aX1 - aX0) / (double)aSteps;

    for (int aStep = 0; aStep <= aSteps; ++aStep)
    {
        double aX = aX0 + aStepX * aStep;
        int aY = (int)aY0 + aStep;
        int aLeft = (int)std::floor(aX);
        int aFrac = (int)((aX - (double)aLeft) * 255.0);
        if (aFrac < 0)
            aFrac = 0;
        if (aFrac > 255)
            aFrac = 255;
        if (aY < 0 || aY >= mHeight)
            continue;
        if (aLeft >= 0 && aLeft < mWidth)
        {
            unsigned int * aPixel = aBits + aLeft + aY * aPitch;
            *aPixel = BlendOver(*aPixel, aPacked, theColor.mAlpha * (255 - aFrac) / 255);
        }
        if (aLeft + 1 >= 0 && aLeft + 1 < mWidth)
        {
            unsigned int * aPixel = aBits + aLeft + 1 + aY * aPitch;
            *aPixel = BlendOver(*aPixel, aPacked, theColor.mAlpha * aFrac / 255);
        }
    }
    BitsChanged();
}

void Sexy::MemoryImage::AdditiveDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY,
                                           const Color & theColor)
{
    (void)theStartX;
    (void)theStartY;
    (void)theEndX;
    (void)theEndY;
    (void)theColor;
}

void Sexy::MemoryImage::DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY,
                                   const Color & theColor, int theDrawMode)
{
    if (theStartY == theEndY)
    {
        double aMinX = theStartX <= theEndX ? theStartX : theEndX;
        double aMaxX = theStartX < theEndX ? theEndX : theStartX;
        TRect<int> aRect((int)aMinX, (int)theEndY, (int)aMaxX - (int)aMinX + 1, (int)(1.0 - theEndY + theEndY));
        FillRect(aRect, theColor, theDrawMode);
        return;
    }

    if (theStartX == theEndX)
    {
        double aMinY = theEndY <= theStartY ? theEndY : theStartY;
        double aMaxY = theEndY < theStartY ? theStartY : theEndY;
        TRect<int> aRect((int)theStartX, (int)aMinY, (int)(theStartX + 1.0 - theStartX), (int)aMaxY - (int)aMinY + 1);
        FillRect(aRect, theColor, theDrawMode);
        return;
    }

    if (theDrawMode == kDrawNormal)
        NormalDrawLineAA(theStartX, theStartY, theEndX, theEndY, theColor);
    else if (theDrawMode == kDrawAdditive)
        AdditiveDrawLineAA(theStartX, theStartY, theEndX, theEndY, theColor);

    BitsChanged();
}

namespace {

unsigned int SampleSource(Sexy::MemoryImage * theSource, int theX, int theY)
{
    if (theSource == nullptr || theX < 0 || theY < 0 || theX >= theSource->mWidth || theY >= theSource->mHeight)
        return 0;
    if (theSource->mColorTable != nullptr && theSource->mColorIndices != nullptr)
        return theSource->mColorTable[theSource->mColorIndices[theY * theSource->GetPitch() + theX]];
    unsigned int * aBits = theSource->GetBits();
    if (aBits == nullptr)
        return 0;
    return aBits[theY * theSource->GetPitch() + theX];
}

} // namespace

void Sexy::MemoryImage::AdditiveBlt(Sexy::Image * theSource, int theX, int theY,
                                    const TRect<int> & theSourceRect, const Color & theColor)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    MemoryImage * aSrc = dynamic_cast<MemoryImage *>(theSource);
    if (aSrc == nullptr || mApp == nullptr)
        return;

    const unsigned char * aTable = mApp->mAdd8BitMaxTable;
    unsigned int * aDestBits = GetBits();
    if (aDestBits == nullptr)
        return;
    int aDestPitch = GetPitch();
    bool aWhite = theColor == Color::White;

    for (int aRow = 0; aRow < theSourceRect.mHeight; ++aRow)
    {
        int aDestY = theY + aRow;
        if (aDestY < 0 || aDestY >= mHeight)
            continue;
        for (int aCol = 0; aCol < theSourceRect.mWidth; ++aCol)
        {
            int aDestX = theX + aCol;
            if (aDestX < 0 || aDestX >= mWidth)
                continue;
            unsigned int aSrcPixel = SampleSource(aSrc, theSourceRect.mX + aCol, theSourceRect.mY + aRow);
            if (!aWhite)
                aSrcPixel = ColorizeSource(aSrcPixel, theColor);
            unsigned int aAdd = PremultiplyAdd(aSrcPixel, aSrc->mHasAlpha);
            unsigned int * aDest = aDestBits + aDestX + aDestY * aDestPitch;
            *aDest = AdditivePixel(aTable, *aDest, aAdd);
        }
    }
    BitsChanged();
}

void Sexy::MemoryImage::AdditiveBltMirror(Sexy::Image * theSource, int theX, int theY,
                                          const TRect<int> & theSourceRect, const Color & theColor)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    MemoryImage * aSrc = dynamic_cast<MemoryImage *>(theSource);
    if (aSrc == nullptr || mApp == nullptr)
        return;

    const unsigned char * aTable = mApp->mAdd8BitMaxTable;
    unsigned int * aDestBits = GetBits();
    if (aDestBits == nullptr)
        return;
    int aDestPitch = GetPitch();
    bool aWhite = theColor == Color::White;
    int aDestX0 = theX + theSourceRect.mWidth - 1;

    for (int aRow = 0; aRow < theSourceRect.mHeight; ++aRow)
    {
        int aDestY = theY + aRow;
        if (aDestY < 0 || aDestY >= mHeight)
            continue;
        for (int aCol = 0; aCol < theSourceRect.mWidth; ++aCol)
        {
            int aDestX = aDestX0 - aCol;
            if (aDestX < 0 || aDestX >= mWidth)
                continue;
            unsigned int aSrcPixel = SampleSource(aSrc, theSourceRect.mX + aCol, theSourceRect.mY + aRow);
            if (!aWhite)
                aSrcPixel = ColorizeSource(aSrcPixel, theColor);
            unsigned int aAdd = PremultiplyAdd(aSrcPixel, aSrc->mHasAlpha);
            unsigned int * aDest = aDestBits + aDestX + aDestY * aDestPitch;
            *aDest = AdditivePixel(aTable, *aDest, aAdd);
        }
    }
    BitsChanged();
}

void Sexy::MemoryImage::NormalBlt(Sexy::Image * theSource, int theX, int theY,
                                  const TRect<int> & theSourceRect, const Color & theColor)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    MemoryImage * aSrc = dynamic_cast<MemoryImage *>(theSource);
    if (aSrc == nullptr)
        return;

    unsigned int * aDestBits = GetBits();
    if (aDestBits == nullptr)
        return;
    int aDestPitch = GetPitch();
    bool aOpaqueCopy = theColor == Color::White && !aSrc->mHasAlpha && !aSrc->mHasTrans;

    for (int aRow = 0; aRow < theSourceRect.mHeight; ++aRow)
    {
        int aDestY = theY + aRow;
        if (aDestY < 0 || aDestY >= mHeight)
            continue;
        for (int aCol = 0; aCol < theSourceRect.mWidth; ++aCol)
        {
            int aDestX = theX + aCol;
            if (aDestX < 0 || aDestX >= mWidth)
                continue;
            unsigned int aSrcPixel = SampleSource(aSrc, theSourceRect.mX + aCol, theSourceRect.mY + aRow);
            aSrcPixel = ColorizeSource(aSrcPixel, theColor);
            unsigned int * aDest = aDestBits + aDestX + aDestY * aDestPitch;
            if (aOpaqueCopy)
                *aDest = aSrcPixel | 0xFF000000u;
            else
                *aDest = BlendOver(*aDest, aSrcPixel, (int)(aSrcPixel >> 24));
        }
    }
    BitsChanged();
}

void Sexy::MemoryImage::Blt(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect,
                            const Color & theColor, int theDrawMode, bool theColorize)
{
    (void)theColorize;
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    if (theDrawMode == kDrawNormal)
        NormalBlt(theSource, theX, theY, theSourceRect, theColor);
    else if (theDrawMode == kDrawAdditive)
        AdditiveBlt(theSource, theX, theY, theSourceRect, theColor);
}

void Sexy::MemoryImage::NormalBltMirror(Sexy::Image * theSource, int theX, int theY,
                                        const TRect<int> & theSourceRect, const Color & theColor)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    MemoryImage * aSrc = dynamic_cast<MemoryImage *>(theSource);
    if (aSrc == nullptr)
        return;

    unsigned int * aDestBits = GetBits();
    if (aDestBits == nullptr)
        return;
    int aDestPitch = GetPitch();
    int aDestX0 = theX + theSourceRect.mWidth - 1;

    for (int aRow = 0; aRow < theSourceRect.mHeight; ++aRow)
    {
        int aDestY = theY + aRow;
        if (aDestY < 0 || aDestY >= mHeight)
            continue;
        for (int aCol = 0; aCol < theSourceRect.mWidth; ++aCol)
        {
            int aDestX = aDestX0 - aCol;
            if (aDestX < 0 || aDestX >= mWidth)
                continue;
            unsigned int aSrcPixel = SampleSource(aSrc, theSourceRect.mX + aCol, theSourceRect.mY + aRow);
            aSrcPixel = ColorizeSource(aSrcPixel, theColor);
            unsigned int * aDest = aDestBits + aDestX + aDestY * aDestPitch;
            *aDest = BlendOver(*aDest, aSrcPixel, (int)(aSrcPixel >> 24));
        }
    }
    BitsChanged();
}

void Sexy::MemoryImage::BltMirror(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect,
                                  const Color & theColor, int theDrawMode)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    if (theDrawMode == kDrawNormal)
        NormalBltMirror(theSource, theX, theY, theSourceRect, theColor);
    else if (theDrawMode == kDrawAdditive)
        AdditiveBltMirror(theSource, theX, theY, theSourceRect, theColor);
}

bool Sexy::MemoryImage::BltRotatedClipHelper(float & theX, float & theY, const TRect<int> & theSourceRect,
                                             const TRect<int> & theClipRect, double theRotate,
                                             TRect<float> & theRotatedRect, float theRotateCenterX,
                                             float theRotateCenterY)
{
    float aCos = std::cos((float)theRotate);
    float aSin = std::sin((float)theRotate);
    float aCornerX[4] = { 0, (float)theSourceRect.mWidth, (float)theSourceRect.mWidth, 0 };
    float aCornerY[4] = { 0, 0, (float)theSourceRect.mHeight, (float)theSourceRect.mHeight };
    float aMinX = 10000000.0f;
    float aMaxX = -10000000.0f;
    float aMinY = 10000000.0f;
    float aMaxY = -10000000.0f;

    for (int i = 0; i < 4; ++i)
    {
        float aOffsetX = aCornerX[i] - theRotateCenterX;
        float aOffsetY = aCornerY[i] - theRotateCenterY;
        aCornerX[i] = (theRotateCenterX + aOffsetX * aCos + aOffsetY * aSin) + theX;
        aCornerY[i] = (theRotateCenterY + aOffsetY * aCos - aOffsetX * aSin) + theY;
        if (aCornerX[i] < aMinX)
            aMinX = aCornerX[i];
        if (aCornerX[i] > aMaxX)
            aMaxX = aCornerX[i];
        if (aCornerY[i] < aMinY)
            aMinY = aCornerY[i];
        if (aCornerY[i] > aMaxY)
            aMaxY = aCornerY[i];
    }

    TRect<float> aClip((float)theClipRect.mX, (float)theClipRect.mY,
                       (float)theClipRect.mWidth, (float)theClipRect.mHeight);
    TRect<float> aBounds(aMinX, aMinY, aMaxX - aMinX, aMaxY - aMinY);
    TRect<float> aDest = aBounds.Intersection(aClip);
    if (aDest.mWidth <= 0.0f || aDest.mHeight <= 0.0f)
        return false;
    theRotatedRect = aDest;
    (void)theY;
    return true;
}

void Sexy::MemoryImage::BltRotated(Sexy::Image * theSource, float theX, float theY, const TRect<int> & theSourceRect,
                                   const TRect<int> & theClipRect, const Color & theColor, int theDrawMode,
                                   double theRotate, float theRotateCenterX, float theRotateCenterY)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    TRect<float> aDestRect;
    float aX = theX;
    float aY = theY;
    if (!BltRotatedClipHelper(aX, aY, theSourceRect, theClipRect, theRotate, aDestRect, theRotateCenterX, theRotateCenterY))
        return;

    MemoryImage * aSrc = dynamic_cast<MemoryImage *>(theSource);
    if (aSrc == nullptr)
        return;
    unsigned int * aDestBits = GetBits();
    if (aDestBits == nullptr)
        return;

    double aCos = std::cos(theRotate);
    double aSin = std::sin(theRotate);
    int aDestPitch = GetPitch();
    int aLeft = (int)aDestRect.mX;
    int aTop = (int)aDestRect.mY;
    int aRight = aLeft + (int)aDestRect.mWidth;
    int aBottom = aTop + (int)aDestRect.mHeight;
    bool aAdditive = theDrawMode == kDrawAdditive;
    const unsigned char * aTable = (aAdditive && mApp != nullptr) ? mApp->mAdd8BitMaxTable : nullptr;

    for (int aRow = aTop; aRow < aBottom; ++aRow)
    {
        if (aRow < 0 || aRow >= mHeight)
            continue;
        for (int aCol = aLeft; aCol < aRight; ++aCol)
        {
            if (aCol < 0 || aCol >= mWidth)
                continue;
            float aBaseX = (float)aCol - theRotateCenterX - theX;
            float aBaseY = (float)aRow - theRotateCenterY - theY;
            float aU = (float)(aCos * aBaseX - aSin * aBaseY) + theRotateCenterX;
            float aV = (float)(aCos * aBaseY + aSin * aBaseX) + theRotateCenterY;
            int aSrcX = theSourceRect.mX + (int)aU;
            int aSrcY = theSourceRect.mY + (int)aV;
            if (aU < 0.0f || aV < 0.0f || aU >= (float)theSourceRect.mWidth || aV >= (float)theSourceRect.mHeight)
                continue;
            unsigned int aSrcPixel = SampleSource(aSrc, aSrcX, aSrcY);
            aSrcPixel = ColorizeSource(aSrcPixel, theColor);
            unsigned int * aDest = aDestBits + aCol + aRow * aDestPitch;
            if (aAdditive && aTable != nullptr)
                *aDest = AdditivePixel(aTable, *aDest, PremultiplyAdd(aSrcPixel, aSrc->mHasAlpha));
            else
                *aDest = BlendOver(*aDest, aSrcPixel, (int)(aSrcPixel >> 24));
        }
    }
    BitsChanged();
}

bool Sexy::MemoryImage::StretchBltClipHelper(const TRect<int> & theDestRect, const TRect<int> & theSrcRect,
                                             const TRect<int> & theClipRect, TRect<float> & theStretchedRect,
                                             TRect<int> & theClippedDestRect)
{
    theClippedDestRect = theDestRect.Intersection(theClipRect);
    if (theDestRect.mWidth == 0 || theDestRect.mHeight == 0)
        return false;

    double aXFactor = (double)theSrcRect.mWidth / (double)theDestRect.mWidth;
    double aYFactor = (double)theSrcRect.mHeight / (double)theDestRect.mHeight;
    theStretchedRect = TRect<float>(
        (float)((double)theSrcRect.mX + (double)(theClippedDestRect.mX - theDestRect.mX) * aXFactor),
        (float)((double)theSrcRect.mY + (double)(theClippedDestRect.mY - theDestRect.mY) * aYFactor),
        (float)((double)theSrcRect.mWidth + (double)(theClippedDestRect.mWidth - theDestRect.mWidth) * aXFactor),
        (float)((double)theSrcRect.mHeight + (double)(theClippedDestRect.mHeight - theDestRect.mHeight) * aYFactor));
    return theStretchedRect.mWidth > 0.0f && theStretchedRect.mHeight > 0.0f;
}

bool Sexy::MemoryImage::StretchBltMirrorClipHelper(const TRect<int> & theDestRect, const TRect<int> & theSrcRect,
                                                   const TRect<int> & theClipRect, TRect<float> & theStretchedRect,
                                                   TRect<int> & theClippedDestRect)
{
    theClippedDestRect = theDestRect.Intersection(theClipRect);
    if (theDestRect.mWidth == 0 || theDestRect.mHeight == 0)
        return false;

    double aXFactor = (double)theSrcRect.mWidth / (double)theDestRect.mWidth;
    double aYFactor = (double)theSrcRect.mHeight / (double)theDestRect.mHeight;
    int aTotalClip = theDestRect.mWidth - theClippedDestRect.mWidth;
    int aLeftClip = theClippedDestRect.mX - theDestRect.mX;
    int aRightClip = aTotalClip - aLeftClip;
    theStretchedRect = TRect<float>(
        (float)((double)theSrcRect.mX + (double)aRightClip * aXFactor),
        (float)((double)theSrcRect.mY + (double)(theClippedDestRect.mY - theDestRect.mY) * aYFactor),
        (float)((double)theSrcRect.mWidth + (double)(theClippedDestRect.mWidth - theDestRect.mWidth) * aXFactor),
        (float)((double)theSrcRect.mHeight + (double)(theClippedDestRect.mHeight - theDestRect.mHeight) * aYFactor));
    return theStretchedRect.mWidth > 0.0f && theStretchedRect.mHeight > 0.0f;
}

void Sexy::MemoryImage::SlowStretchBlt(Sexy::Image * theSource, const TRect<int> & theDestRect,
                                       const TRect<float> & theSrcRect, const Color & theColor,
                                       int theDrawMode, bool theMirror)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    MemoryImage * aSrc = dynamic_cast<MemoryImage *>(theSource);
    if (aSrc == nullptr || theDestRect.mWidth <= 0 || theDestRect.mHeight <= 0)
        return;
    unsigned int * aDestBits = GetBits();
    if (aDestBits == nullptr)
        return;

    int aDestPitch = GetPitch();
    float aStepX = theSrcRect.mWidth / (float)theDestRect.mWidth;
    float aStepY = theSrcRect.mHeight / (float)theDestRect.mHeight;
    bool aAdditive = theDrawMode == kDrawAdditive;
    const unsigned char * aTable = (aAdditive && mApp != nullptr) ? mApp->mAdd8BitMaxTable : nullptr;

    for (int aRow = 0; aRow < theDestRect.mHeight; ++aRow)
    {
        int aDestY = theDestRect.mY + aRow;
        if (aDestY < 0 || aDestY >= mHeight)
            continue;
        float aSrcY = theSrcRect.mY + aStepY * ((float)aRow + 0.5f);
        for (int aCol = 0; aCol < theDestRect.mWidth; ++aCol)
        {
            int aDestX = theMirror ? theDestRect.mX + theDestRect.mWidth - 1 - aCol
                                   : theDestRect.mX + aCol;
            if (aDestX < 0 || aDestX >= mWidth)
                continue;
            float aSrcX = theSrcRect.mX + aStepX * ((float)aCol + 0.5f);
            unsigned int aSrcPixel = SampleSource(aSrc, (int)aSrcX, (int)aSrcY);
            aSrcPixel = ColorizeSource(aSrcPixel, theColor);
            unsigned int * aDest = aDestBits + aDestX + aDestY * aDestPitch;
            if (aAdditive && aTable != nullptr)
                *aDest = AdditivePixel(aTable, *aDest, PremultiplyAdd(aSrcPixel, aSrc->mHasAlpha));
            else
                *aDest = BlendOver(*aDest, aSrcPixel, (int)(aSrcPixel >> 24));
        }
    }
    BitsChanged();
}

void Sexy::MemoryImage::FastStretchBlt(Sexy::Image * theSource, const TRect<int> & theDestRect,
                                       const TRect<float> & theSrcRect, const Color & theColor,
                                       int theDrawMode, bool theMirror)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    MemoryImage * aSrc = dynamic_cast<MemoryImage *>(theSource);
    if (aSrc == nullptr || theDestRect.mWidth <= 0 || theDestRect.mHeight <= 0)
        return;
    unsigned int * aDestBits = GetBits();
    unsigned int * aSrcBits = aSrc->GetBits();
    if (aDestBits == nullptr || aSrcBits == nullptr)
        return;

    int aDestPitch = GetPitch();
    int aSrcPitch = aSrc->GetPitch();
    double aAddX = (double)theSrcRect.mWidth / (double)theDestRect.mWidth;
    double aAddY = (double)theSrcRect.mHeight / (double)theDestRect.mHeight;
    double aSrcY = theSrcRect.mY;
    bool aAdditive = theDrawMode == kDrawAdditive;
    const unsigned char * aTable = (aAdditive && mApp != nullptr) ? mApp->mAdd8BitMaxTable : nullptr;

    for (int aRow = 0; aRow < theDestRect.mHeight; ++aRow)
    {
        int aDestY = theDestRect.mY + aRow;
        double aSrcX = theSrcRect.mX;
        if (aDestY >= 0 && aDestY < mHeight)
        {
            for (int aCol = 0; aCol < theDestRect.mWidth; ++aCol)
            {
                int aSampleX = (int)aSrcX;
                int aSampleY = (int)aSrcY;
                int aDestX = theMirror ? theDestRect.mX + theDestRect.mWidth - 1 - aCol
                                       : theDestRect.mX + aCol;
                if (aDestX >= 0 && aDestX < mWidth
                    && aSampleX >= 0 && aSampleY >= 0
                    && aSampleX < aSrc->mWidth && aSampleY < aSrc->mHeight)
                {
                    unsigned int aSrcPixel = aSrcBits[aSampleX + aSampleY * aSrcPitch];
                    aSrcPixel = ColorizeSource(aSrcPixel, theColor);
                    unsigned int * aDest = aDestBits + aDestX + aDestY * aDestPitch;
                    if (aAdditive && aTable != nullptr)
                        *aDest = AdditivePixel(aTable, *aDest, PremultiplyAdd(aSrcPixel, true));
                    else
                        *aDest = BlendOver(*aDest, aSrcPixel, (int)(aSrcPixel >> 24));
                }
                aSrcX += aAddX;
            }
        }
        aSrcY += aAddY;
    }
    BitsChanged();
}

void Sexy::MemoryImage::StretchBlt(Sexy::Image * theSource, const TRect<int> & theDestRect,
                                   const TRect<int> & theSrcRect, const TRect<int> & theClipRect,
                                   const Color & theColor, int theDrawMode, bool theFastStretch)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    TRect<float> aSrcOut;
    TRect<int> aDestOut;
    if (!StretchBltClipHelper(theDestRect, theSrcRect, theClipRect, aSrcOut, aDestOut))
        return;

    if (theFastStretch && theSource != nullptr && theSource->GetPixelFormat() == PIXEL_FORMAT_A8R8G8B8
        && theColor == Color::White)
        FastStretchBlt(theSource, aDestOut, aSrcOut, theColor, theDrawMode, false);
    else
        SlowStretchBlt(theSource, aDestOut, aSrcOut, theColor, theDrawMode, false);
}

void Sexy::MemoryImage::StretchBltMirror(Sexy::Image * theSource, const TRect<int> & theDestRect,
                                         const TRect<int> & theSrcRect, const TRect<int> & theClipRect,
                                         const Color & theColor, int theDrawMode, bool theFastStretch)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    TRect<float> aSrcOut;
    TRect<int> aDestOut;
    if (!StretchBltMirrorClipHelper(theDestRect, theSrcRect, theClipRect, aSrcOut, aDestOut))
        return;

    if (theFastStretch)
        FastStretchBlt(theSource, aDestOut, aSrcOut, theColor, theDrawMode, true);
    else
        SlowStretchBlt(theSource, aDestOut, aSrcOut, theColor, theDrawMode, true);
}

void Sexy::MemoryImage::BltMatrixHelper(Sexy::Image * theSource, float theX, float theY,
                                        const SexyMatrix3 & theMatrix, const TRect<int> & theSourceRect,
                                        const Color & theColor, int theDrawMode, const TRect<int> & theClipRect,
                                        void * theBits, int theStride, int theBlendMode, bool theMirror)
{
    (void)theMirror;
    MemoryImage * aSrc = dynamic_cast<MemoryImage *>(theSource);
    if (aSrc == nullptr)
        return;

    float aHalfW = (float)theSourceRect.mWidth * 0.5f;
    float aHalfH = (float)theSourceRect.mHeight * 0.5f;
    float aWidth = (float)theSource->mWidth;
    float aHeight = (float)theSource->mHeight;
    if (aWidth == 0.0f || aHeight == 0.0f)
        return;
    float aU0 = (float)theSourceRect.mX / aWidth;
    float aU1 = (float)(theSourceRect.mX + theSourceRect.mWidth) / aWidth;
    float aV0 = (float)theSourceRect.mY / aHeight;
    float aV1 = (float)(theSourceRect.mY + theSourceRect.mHeight) / aHeight;

    // MemoryImage::BltMatrixHelper @0x327c64 writes the corners in this order:
    // (+half, -half), (-half, -half), (+half, +half), (-half, +half).
    SWShapeVertex aCorners[4] = {
        { aHalfW, -aHalfH, aU1, aV0, 0xFFFFFFFFu },
        { -aHalfW, -aHalfH, aU0, aV0, 0xFFFFFFFFu },
        { aHalfW, aHalfH, aU1, aV1, 0xFFFFFFFFu },
        { -aHalfW, aHalfH, aU0, aV1, 0xFFFFFFFFu }
    };
    for (int i = 0; i < 4; ++i)
    {
        // Homogeneous row (x, y, 1). TV's matrix*vector reads translation from
        // m02/m12, so a 2-vector multiply drops the roll's position.
        SexyVector3 aPoint(aCorners[i].mX, aCorners[i].mY, 1.0f);
        SexyVector3 aMapped = theMatrix * aPoint;
        aCorners[i].mX = aMapped.mX + theX - 0.5f;
        aCorners[i].mY = aMapped.mY + theY - 0.5f;
    }

    SWHelper aHelper;
    aHelper.SWDrawShape(aCorners, 4, aSrc, theColor, theDrawMode, theClipRect,
                        theBits, theStride, theBlendMode, false, false);
}

void Sexy::MemoryImage::BltMatrix(Sexy::Image * theSource, float theX, float theY, const SexyMatrix3 & theMatrix,
                                  const TRect<int> & theSourceRect, const Color & theColor, int theDrawMode,
                                  const TRect<int> & theClipRect, bool theLinearBlend, bool theMirror)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    void * aBits = GetBits();
    int aStride = GetStride();
    int aBlendMode = (mForcedMode || mHasAlpha || mHasTrans) ? 0x8888 : 0x888;
    BltMatrixHelper(theSource, theX, theY, theMatrix, theSourceRect, theColor, theDrawMode,
                    theClipRect, aBits, aStride, aBlendMode, theMirror);
    (void)theLinearBlend;
    BitsChanged();
}

void Sexy::MemoryImage::BltTrianglesTexHelper(Sexy::Image * theSource, const SexyVertex2D (*theTriangles)[3],
                                              int theTriangleCount, const TRect<int> & theClipRect,
                                              const Color & theColor, int theDrawMode, void * theBits,
                                              int theStride, int theBlendMode, float theTx, float theTy,
                                              bool theAdditive)
{
    MemoryImage * aSrc = dynamic_cast<MemoryImage *>(theSource);
    if (theTriangleCount <= 0 || theTriangles == nullptr)
        return;

    SWHelper aHelper;
    for (int aTri = 0; aTri < theTriangleCount; ++aTri)
    {
        SWShapeVertex aVerts[3];
        bool aColored = false;
        for (int aCorner = 0; aCorner < 3; ++aCorner)
        {
            const SexyVertex2D & aIn = theTriangles[aTri][aCorner];
            aVerts[aCorner].mX = aIn.mX + theTx;
            aVerts[aCorner].mY = aIn.mY + theTy;
            aVerts[aCorner].mU = aIn.mU;
            aVerts[aCorner].mV = aIn.mV;
            aVerts[aCorner].mColor = (unsigned int)aIn.mColor;
            if (aIn.mColor != 0)
                aColored = true;
        }
        aHelper.SWDrawShape(aVerts, 3, aSrc, theColor, theDrawMode, theClipRect,
                            theBits, theStride, theBlendMode, theAdditive, aColored);
    }
}

void Sexy::MemoryImage::BltTrianglesTex(Image * theSource, const SexyVertex2D (*theTriangles)[3],
                                        int theTriangleCount, const TRect<int> & theClipRect,
                                        const Color & theColor, int theDrawMode, float theTx,
                                        float theTy, bool theBlend)
{
    if (theSource != nullptr)
        theSource->mAboutToSample = true;
    void * aBits = GetBits();
    int aStride = GetStride();
    int aBlendMode = (mForcedMode || mHasAlpha || mHasTrans) ? 0x8888 : 0x888;
    BltTrianglesTexHelper(theSource, theTriangles, theTriangleCount, theClipRect, theColor,
                          theDrawMode, aBits, aStride, aBlendMode, theTx, theTy, theBlend);
    BitsChanged();
}

Sexy::MemoryImage::MemoryImage()
{
    mParentX = 0;
    mParentY = 0;
    mTag = ((int)(unsigned char)'m' << 24) | ((int)(unsigned char)'e' << 16)
         | ((int)(unsigned char)'m' << 8) | (int)(unsigned char)'i';
    mApp = (LawnApp *)gSexyAppBase;
    Init();
}

Sexy::MemoryImage::MemoryImage(Sexy::SexyAppBase * theApp)
{
    mParentX = 0;
    mParentY = 0;
    mTag = ((int)(unsigned char)'m' << 24) | ((int)(unsigned char)'e' << 16)
         | ((int)(unsigned char)'m' << 8) | (int)(unsigned char)'i';
    mApp = (LawnApp *)theApp;
    Init();
}

Sexy::MemoryImage * Sexy::MemoryImage::CreateSubImage(int theX, int theY, int theWidth, int theHeight)
{
    MemoryImage * aParent = mParent;
    int aX = theX;
    int aY = theY;
    if (aParent != nullptr)
    {
        aX = theX + mParentX;
        aY = theY + mParentY;
    }
    else
    {
        aParent = this;
    }

    MemoryImage * aSubImage = new MemoryImage((SexyAppBase *)mApp);
    aParent->Attach(aSubImage, aX, aY, theWidth, theHeight);
    return aSubImage;
}

Sexy::MemoryImage::MemoryImage(const Sexy::MemoryImage & theOther)
    : Image(theOther)
{
    MemoryImage * aSource = const_cast<MemoryImage *>(&theOther);

    mApp = aSource->mApp;
    mBitsChangedCount = aSource->mBitsChangedCount;
    mD3DFlags = aSource->mD3DFlags;
    mHasTrans = aSource->mHasTrans;
    mHasAlpha = aSource->mHasAlpha;
    mIsAlphaOnly = aSource->mIsAlphaOnly;
    mIsVolatile = aSource->mIsVolatile;
    mPurgeBits = aSource->mPurgeBits;
    mWantPal = aSource->mWantPal;
    mBitsChanged = aSource->mBitsChanged;
    mD3DData = nullptr;
    mParent = nullptr;
    mParentX = 0;
    mParentY = 0;
    mPixels = nullptr;
    mFileName = aSource->mFileName;

    bool aMaterialised = false;
    if (aSource->mBits != nullptr
        || (aSource->mColorTable == nullptr && (aSource->GetBits(), aMaterialised = true, aSource->mBits != nullptr))
        || (aSource->mParent != nullptr && aSource->mParent->mBits != nullptr))
    {
        int aCount = mHeight * mWidth;
        unsigned int * aBits = new unsigned int[(size_t)aCount + 1];
        mBits = (unsigned long *)aBits;
        mPitch = mWidth;
        mStride = 4 * mWidth;

        unsigned int * aSourceBits = aSource->GetBits();
        int aSourcePitch = aSource->GetPitch();
        if (aSourceBits != nullptr)
        {
            for (int aRow = 0; aRow < mHeight; ++aRow)
                std::memcpy((char *)mBits + 4 * mPitch * aRow,
                            aSourceBits + aSourcePitch * aRow, (size_t)mWidth * 4);
        }
        aBits[aCount] = kMemoryCheckId;
    }
    else
    {
        mBits = nullptr;
    }

    if (aMaterialised)
    {
        delete[] aSource->mBits;
        aSource->mBits = nullptr;
    }

    if (aSource->mColorTable != nullptr)
    {
        mColorTable = new unsigned int[256];
        std::memcpy(mColorTable, aSource->mColorTable, 256 * sizeof(unsigned int));
    }
    else
    {
        mColorTable = nullptr;
    }

    if (aSource->mColorIndices != nullptr && mWidth > 0 && mHeight > 0)
    {
        mColorIndices = new unsigned char[(size_t)mHeight * (size_t)mWidth];
        std::memcpy(mColorIndices, aSource->mColorIndices, (size_t)mHeight * (size_t)mWidth);
        if (mBits == nullptr)
        {
            mPitch = mWidth;
            mStride = mWidth;
        }
    }
    else
    {
        mColorIndices = nullptr;
    }

    if (aSource->mNativeAlphaData != nullptr)
    {
        if (aSource->mColorTable != nullptr)
        {
            mNativeAlphaData = new unsigned int[256];
            std::memcpy(mNativeAlphaData, aSource->mNativeAlphaData, 256 * sizeof(unsigned int));
        }
        else
        {
            mNativeAlphaData = new unsigned int[(size_t)mHeight * (size_t)mWidth];
            std::memcpy(mNativeAlphaData, aSource->mNativeAlphaData,
                        (size_t)mHeight * (size_t)mWidth * sizeof(unsigned int));
        }
    }
    else
    {
        mNativeAlphaData = nullptr;
    }

    if (aSource->mRLAlphaData != nullptr && mWidth > 0 && mHeight > 0)
    {
        mRLAlphaData = new unsigned char[(size_t)mHeight * (size_t)mWidth];
        std::memcpy(mRLAlphaData, aSource->mRLAlphaData, (size_t)mHeight * (size_t)mWidth);
    }
    else
    {
        mRLAlphaData = nullptr;
    }

    if (aSource->mRLAdditiveData != nullptr && mWidth > 0 && mHeight > 0)
    {
        mRLAdditiveData = new unsigned char[(size_t)mHeight * (size_t)mWidth];
        std::memcpy(mRLAdditiveData, aSource->mRLAdditiveData, (size_t)mHeight * (size_t)mWidth);
    }
    else
    {
        mRLAdditiveData = nullptr;
    }

    if (mApp != nullptr)
        mApp->AddMemoryImage(this);
}

void Sexy::MemoryImage::RemoveSubImage(Sexy::MemoryImage * theSubImage)
{
    mSubImagesLock.Lock();
    mSubImages.erase(theSubImage);
    mSubImagesLock.Unlock();
}

void Sexy::MemoryImage::Detach(bool theCopyBits)
{
    if (mParent == nullptr)
        return;

    if (theCopyBits && mWidth > 0 && mHeight > 0)
    {
        delete[] mBits;
        delete[] mColorTable;
        mColorTable = nullptr;
        delete[] mColorIndices;
        mColorIndices = nullptr;

        int aCount = mHeight * mWidth;
        unsigned int * aBits = new unsigned int[(size_t)aCount + 1];
        mBits = (unsigned long *)aBits;
        mPitch = mWidth;
        mStride = 4 * mWidth;

        unsigned int * aParentBits = mParent->GetBits();
        int aParentPitch = mParent->GetPitch();
        if (aParentBits != nullptr)
        {
            const char * aSrc = (const char *)(aParentBits + mParentX + mParentY * aParentPitch);
            int aSrcStride = 4 * aParentPitch;
            for (int aRow = 0; aRow < mHeight; ++aRow)
            {
                std::memcpy((char *)mBits + 4 * mPitch * aRow, aSrc, (size_t)mWidth * 4);
                aSrc += aSrcStride;
            }
        }
        aBits[aCount] = kMemoryCheckId;
    }

    mParent->RemoveSubImage(this);
    mParent = nullptr;
    mParentX = 0;
    mParentY = 0;
}

void Sexy::MemoryImage::AddSubImage(Sexy::MemoryImage * theSubImage)
{
    if (theSubImage == nullptr)
        return;
    mSubImagesLock.Lock();
    mSubImages.insert(theSubImage);
    mSubImagesLock.Unlock();
}

void Sexy::MemoryImage::Attach(Sexy::MemoryImage * theSubImage, int theX, int theY, int theWidth, int theHeight)
{
    if (theSubImage == nullptr)
        return;

    bool aBitsChanged = mBitsChanged;
    bool aHasTrans = mHasTrans;
    bool aHasAlpha = mHasAlpha;
    bool aIsAlphaOnly = mIsAlphaOnly;

    theSubImage->mParent = this;
    theSubImage->mAttachedParent = this;
    theSubImage->mFormat = mFormat;
    theSubImage->mSubRectX = theX;
    theSubImage->mSubRectY = theY;
    theSubImage->mSubRectRight = theX + theWidth;
    theSubImage->mSubRectBottom = theY + theHeight;
    theSubImage->mAttachedPad = 0;

    theSubImage->mParentX = theX;
    theSubImage->mParentY = theY;
    theSubImage->mWidth = theWidth;
    theSubImage->mHeight = theHeight;
    theSubImage->mBitsChanged = aBitsChanged;
    theSubImage->mHasTrans = aHasTrans;
    theSubImage->mHasAlpha = aHasAlpha;
    theSubImage->mIsAlphaOnly = aIsAlphaOnly;

    AddSubImage(theSubImage);
}

int Sexy::MemoryImage::GetPitch()
{
    if (mParent != nullptr)
        return mParent->GetPitch();
    return Image::GetPitch();
}

int Sexy::MemoryImage::GetStride()
{
    if (mParent != nullptr)
        return mParent->GetStride();
    return Image::GetStride();
}

void Sexy::MemoryImage::WriteToPng(std::string theFileName)
{
    ImageLib::Image aImage;
    aImage.mWidth = mWidth;
    aImage.mHeight = mHeight;
    aImage.mBits = GetBits();
    Sexy::String aPath;
    aPath = theFileName.c_str();
    ImageLib::WritePNGImage(aPath, &aImage);
}

void Sexy::MemoryImage::WriteToJPEG(std::string theFileName)
{
    ImageLib::Image aImage;
    aImage.mWidth = mWidth;
    aImage.mHeight = mHeight;
    aImage.mBits = GetBits();
    Sexy::String aPath;
    aPath = theFileName.c_str();
    ImageLib::WriteJPEGImage(aPath, &aImage);
}

Sexy::MemoryImage::~MemoryImage()
{
    if (Sexy::BgfxRenderDevice::sCurrent != nullptr)
        Sexy::BgfxRenderDevice::sCurrent->RemoveTexture(this);
    // Cleanup clears gSexyAppBase before the app object is freed. A capture
    // image in Run outlives that delete and must not lock the dead critsect.
    if (mApp != nullptr && gSexyAppBase != nullptr)
        mApp->RemoveMemoryImage(this);
    DetachSubImages(false);
    Detach(false);

    delete[] mPixels;
    delete[] mBits;
    delete[] mNativeAlphaData;
    delete[] mRLAlphaData;
    delete[] mRLAdditiveData;
    delete[] mColorIndices;
    delete[] mColorTable;
}

// Graphics is the software draw context. Coordinates and the scale formulas
// come from the TV draw bodies; the destination image does the pixel work.

#include "Graphics.h"
#include "Font.h"
#include "Graphics3D.h"
#include "IGraphicsDriver.h"
#include "Image.h"
#include "MemoryImage.h"
#include "ReconExterns.h"
#include "SexyAppBase.h"
#include "SexyTransform2D.h"
#include "Transform.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <vector>

namespace {

const int kMaxSpans = 8192;
unsigned char gCoverageTable[256 * 256];

int ClampByte(int theValue)
{
    if (theValue < 0)
        return 0;
    if (theValue > 255)
        return 255;
    return theValue;
}

int HexDigit(char theChar)
{
    if ((unsigned char)(theChar - '0') <= 9u)
        return theChar - '0';
    if ((unsigned char)(theChar - 'A') <= 5u)
        return theChar - 'A' + 10;
    if ((unsigned char)(theChar - 'a') <= 5u)
        return theChar - 'a' + 10;
    return 0;
}

} // namespace

bool Sexy::Graphics::mDefaultGlobalScale = false;
bool Sexy::Graphics::mDefaultTrackDeviceState = false;
Sexy::Image Sexy::Graphics::mStaticImage;
Sexy::String Sexy::Graphics::mPreferedEncoding;

const Sexy::TPoint<int> *Sexy::Graphics::mPolyPoints = nullptr;

int Sexy::Graphics::ComparePointY(void const* theLeft, void const* theRight)
{
    int aLeft = *(int const *)theLeft;
    int aRight = *(int const *)theRight;
    if (mPolyPoints[aLeft].mY <= mPolyPoints[aRight].mY)
        return -1;
    return 1;
}

int Sexy::Graphics::CompareEdgeX(void const* theLeft, void const* theRight)
{
    if (*(double const *)theLeft > *(double const *)theRight)
        return 1;
    return -1;
}

void Sexy::Graphics::SetPreferedEncoding(Sexy::String const& theEncoding)
{
    mPreferedEncoding = theEncoding;
}

Sexy::String Sexy::Graphics::GetPreferedEncoding()
{
    return mPreferedEncoding;
}

Sexy::Graphics3D * Sexy::Graphics::Get3D()
{
    return mGraphics3D;
}

void Sexy::Graphics::SetAsCurrentContext()
{
}

void Sexy::Graphics::PushTransform(SexyMatrix3 const& theTransform, bool theConcatenate)
{
    (void)theTransform;
    (void)theConcatenate;
}

void Sexy::Graphics::PopTransform()
{
}

void Sexy::Graphics::PopTransform(SexyMatrix3 & theTransform)
{
    (void)theTransform;
}

Sexy::Color const& Sexy::Graphics::DrawColor() const
{
    if (mColorStack.empty())
        return mColor;
    return mFinalColor;
}

void Sexy::Graphics::CalcFinalColor()
{
    if (mColorStack.empty())
    {
        mFinalColor = mColor;
        return;
    }

    Color const& aMult = mColorStack.back();
    mFinalColor.mRed = ClampByte(aMult.mRed * mColor.mRed / 255);
    mFinalColor.mGreen = ClampByte(aMult.mGreen * mColor.mGreen / 255);
    mFinalColor.mBlue = ClampByte(aMult.mBlue * mColor.mBlue / 255);
    mFinalColor.mAlpha = ClampByte(aMult.mAlpha * mColor.mAlpha / 255);
}

Color const& Sexy::Graphics::GetImageColor() const
{
    if (!mColorStack.empty())
    {
        if (mColorizeImages)
            return mFinalColor;
        return mColorStack.back();
    }
    if (mColorizeImages)
        return mColor;
    return Color::White;
}

Sexy::Graphics* Sexy::Graphics::SetTrackingDeviceState(bool theTrack)
{
    mTrackDeviceState = theTrack;
    return this;
}

Sexy::Font * Sexy::Graphics::GetFont()
{
    return mFont;
}

Sexy::Graphics* Sexy::Graphics::SetFont(Sexy::Font * theFont)
{
    mFont = theFont;
    return this;
}

Sexy::Graphics* Sexy::Graphics::SetColor(Color const& theColor)
{
    mColor = theColor;
    CalcFinalColor();
    return this;
}

const Color & Sexy::Graphics::GetColor()
{
    return mColor;
}

void Sexy::Graphics::PopColorMult()
{
    if (!mColorStack.empty())
        mColorStack.pop_back();
    CalcFinalColor();
}

Sexy::Graphics* Sexy::Graphics::SetDrawMode(int theDrawMode)
{
    mDrawMode = theDrawMode;
    return this;
}

int Sexy::Graphics::GetDrawMode()
{
    return mDrawMode;
}

Sexy::Graphics* Sexy::Graphics::SetColorizeImages(bool theColorize)
{
    mColorizeImages = theColorize;
    return this;
}

bool Sexy::Graphics::GetColorizeImages()
{
    return mColorizeImages;
}

Sexy::Graphics* Sexy::Graphics::SetFastStretch(bool theFastStretch)
{
    mFastStretch = theFastStretch;
    return this;
}

bool Sexy::Graphics::GetFastStretch()
{
    return mFastStretch;
}

Sexy::Graphics* Sexy::Graphics::SetLinearBlend(bool theLinear)
{
    mLinearBlend = theLinear;
    return this;
}

bool Sexy::Graphics::GetLinearBlend()
{
    return mLinearBlend;
}

Sexy::Graphics* Sexy::Graphics::SetDefaultGlobalScale(bool theScale)
{
    mDefaultGlobalScale = theScale;
    return this;
}

Sexy::Graphics* Sexy::Graphics::SetGlobalScale(bool theScale)
{
    mApplyGlobalScale = theScale;
    return this;
}

bool Sexy::Graphics::GetGlobalScale()
{
    return mApplyGlobalScale;
}

bool Sexy::Graphics::ScaleIsIdentity() const
{
    return !mApplyGlobalScale || (mScaleX == 1.0f && mScaleY == 1.0f);
}

void Sexy::Graphics::ScalePoint(int & theX, int & theY, int & theWidth, int & theHeight) const
{
    if (ScaleIsIdentity())
        return;
    theX = (int)(floor((float)((float)theX - mScaleOrigX) * mScaleX) + mScaleOrigX);
    theY = (int)(floor((float)mScaleY * (float)((float)theY - mScaleOrigY)) + mScaleOrigY);
    theWidth = (int)ceil((float)theWidth * mScaleX);
    theHeight = (int)ceil((float)mScaleY * (float)theHeight);
}

Sexy::Graphics* Sexy::Graphics::ClearRect(int theX, int theY, int theWidth, int theHeight)
{
    if (mDestImage == nullptr)
        return this;
    int aX = (int)(float)((float)theX + mTransX);
    int aY = (int)(float)((float)theY + mTransY);
    int aWidth = theWidth;
    int aHeight = theHeight;
    ScalePoint(aX, aY, aWidth, aHeight);
    mDestImage->ClearRect(TRect<int>(aX, aY, aWidth, aHeight).Intersection(mClipRect));
    return this;
}

Sexy::Graphics* Sexy::Graphics::ClearRect(TRect<int> const& theRect)
{
    return ClearRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

Sexy::Graphics* Sexy::Graphics::FillRect(int theX, int theY, int theWidth, int theHeight)
{
    if (mDestImage == nullptr)
        return this;
    Color const& aColor = DrawColor();
    if (aColor.mAlpha == 0)
        return this;

    int aX = (int)(float)((float)theX + mTransX);
    int aY = (int)(float)((float)theY + mTransY);
    int aWidth = theWidth;
    int aHeight = theHeight;
    ScalePoint(aX, aY, aWidth, aHeight);
    mDestImage->FillRect(TRect<int>(aX, aY, aWidth, aHeight).Intersection(mClipRect),
                         aColor, mDrawMode);
    return this;
}

Sexy::Graphics* Sexy::Graphics::FillRect(TRect<int> const& theRect)
{
    return FillRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

Sexy::Graphics* Sexy::Graphics::DrawRect(int theX, int theY, int theWidth, int theHeight)
{
    if (mDestImage == nullptr)
        return this;
    Color const& aColor = DrawColor();
    if (aColor.mAlpha == 0)
        return this;

    int aX = (int)(float)((float)theX + mTransX);
    int aY = (int)(float)((float)theY + mTransY);
    int aOutlineWidth = theWidth + 1;
    int aOutlineHeight = theHeight + 1;

    int aClipX = mClipRect.mX;
    if (aX >= aClipX)
        aClipX = aX;
    int aClippedWidth = (aX + aOutlineWidth > mClipRect.mX + mClipRect.mWidth)
                            ? mClipRect.mX + mClipRect.mWidth - aClipX
                            : aX + aOutlineWidth - aClipX;
    int aClipY = mClipRect.mY;
    if (aClipY < aY)
        aClipY = aY;
    int aBottom = aY + aOutlineHeight;
    if (aBottom >= mClipRect.mY + mClipRect.mHeight)
        aBottom = mClipRect.mY + mClipRect.mHeight;
    int aClippedHeight = aBottom - aClipY;
    if (aClippedWidth < 0 || aClippedHeight < 0)
        return this;

    if (aX == aClipX && aY == aClipY && aOutlineWidth == aClippedWidth && aOutlineHeight == aClippedHeight)
    {
        int aDeviceX = aX;
        int aDeviceY = aY;
        int aDeviceWidth = theWidth;
        int aDeviceHeight = theHeight;
        if (!ScaleIsIdentity())
        {
            aDeviceX = (int)(floor((float)((float)aX - mScaleOrigX) * mScaleX) + mScaleOrigX);
            aDeviceY = (int)(floor((float)mScaleY * (float)((float)aY - mScaleOrigY)) + mScaleOrigY);
            aDeviceWidth = (int)ceil((float)theWidth * mScaleX);
            aDeviceHeight = (int)ceil((float)mScaleY * (float)theHeight);
        }
        mDestImage->DrawRect(TRect<int>(aDeviceX, aDeviceY, aDeviceWidth, aDeviceHeight),
                             aColor, mDrawMode);
        return this;
    }

    FillRect(theX, theY, theWidth + 1, 1);
    FillRect(theX, theHeight + theY, theWidth + 1, 1);
    FillRect(theX, theY + 1, 1, theHeight - 1);
    FillRect(theX + theWidth, theY + 1, 1, theHeight - 1);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawRect(TRect<int> const& theRect)
{
    return DrawRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void Sexy::Graphics::DeleteEdge(int theIndex)
{
    for (int anEdge = 0; anEdge < mActiveEdgeCount; ++anEdge)
    {
        if (mActiveEdges[anEdge].mIndex == theIndex)
        {
            --mActiveEdgeCount;
            memmove(&mActiveEdges[anEdge], &mActiveEdges[anEdge + 1],
                    sizeof(ActiveEdge) * (mActiveEdgeCount - anEdge));
            break;
        }
    }
}

void Sexy::Graphics::InsertEdge(int theIndex, int theScanY)
{
    int aNext = (mPointCount - 1 <= theIndex) ? 0 : theIndex + 1;
    TPoint<int> const& aCur = mPolyPoints[theIndex];
    TPoint<int> const& aFollowing = mPolyPoints[aNext];
    bool aSwapped = aCur.mY >= aFollowing.mY;
    int aUpperX = aSwapped ? aFollowing.mX : aCur.mX;
    int aUpperY = aSwapped ? aFollowing.mY : aCur.mY;
    int aLowerX = aSwapped ? aCur.mX : aFollowing.mX;
    int aLowerY = aSwapped ? aCur.mY : aFollowing.mY;
    double aSlope = (double)(aLowerX - aUpperX) / (double)(aLowerY - aUpperY);

    ActiveEdge &anEdge = mActiveEdges[mActiveEdgeCount];
    anEdge.mIndex = theIndex;
    anEdge.mSlope = aSlope;
    anEdge.mX = mTransX + aUpperX + (0.5 - mTransY + theScanY - aUpperY) * aSlope;
    anEdge.mY = aUpperY - aUpperX / aSlope;
    ++mActiveEdgeCount;
}

void Sexy::Graphics::PolyFill(TPoint<int> const* thePoints, int theCount, bool theConvex)
{
    if (mDestImage == nullptr || theCount <= 0)
        return;

    if (theConvex)
    {
        mDestImage->PolyFill3D(thePoints, theCount, &mClipRect, mColor, mDrawMode,
                               (int)mTransX, (int)mTransY, true);
    }

    mPointCount = theCount;
    mPolyPoints = thePoints;

    int *anIndices = new int[theCount];
    mActiveEdges = new ActiveEdge[theCount];
    mActiveEdgeCount = 0;
    for (int anIndex = 0; anIndex < theCount; ++anIndex)
        anIndices[anIndex] = anIndex;
    qsort(anIndices, theCount, sizeof(int), ComparePointY);

    int aFirstY = (int)ceil((double)thePoints[anIndices[0]].mY + mTransY - 0.5);
    int aStartY = (aFirstY < mClipRect.mY) ? mClipRect.mY : aFirstY;
    int aLastY = (int)floor((double)thePoints[anIndices[theCount - 1]].mY + mTransY - 0.5);
    if (aLastY >= mClipRect.mY + mClipRect.mHeight - 1)
        aLastY = mClipRect.mY + mClipRect.mHeight - 1;

    int aSpans[3 * kMaxSpans];
    int aSpanCount = 0;

    for (int aY = aStartY; aY <= aLastY; ++aY)
    {
        for (int aCursor = 0; aCursor < mPointCount; )
        {
            int aPoint = anIndices[aCursor];
            double aScanY = (double)aY + 0.5;
            if ((float)((float)thePoints[aPoint].mY + mTransY) > aScanY)
                break;

            do
            {
                int aPrev = (aPoint <= 0) ? mPointCount - 1 : aPoint - 1;
                double aPrevY = (float)(mTransY + (float)thePoints[aPrev].mY);
                if (aPrevY > (double)aY - 0.5)
                {
                    if (aPrevY > aScanY)
                        InsertEdge(aPrev, aY);
                }
                else
                {
                    DeleteEdge(aPrev);
                }

                int aNext = (mPointCount - 1 <= aPoint) ? 0 : aPoint + 1;
                double aNextY = (float)((float)thePoints[aNext].mY + mTransY);
                if (aNextY <= (double)aY - 0.5)
                    DeleteEdge(aPoint);
                else if (aNextY > aScanY)
                    InsertEdge(aPoint, aY);

                if (++aCursor >= mPointCount)
                    break;
                aPoint = anIndices[aCursor];
            }
            while ((float)((float)thePoints[aPoint].mY + mTransY) <= aScanY);
        }

        qsort(mActiveEdges, mActiveEdgeCount, sizeof(ActiveEdge), CompareEdgeX);
        if (mActiveEdgeCount > 0)
        {
            for (int anEdge = 0; anEdge + 1 < mActiveEdgeCount; anEdge += 2)
            {
                int aSpanStart = (int)ceil(mActiveEdges[anEdge].mX - 0.5);
                if (aSpanStart < mClipRect.mX)
                    aSpanStart = mClipRect.mX;
                int aSpanEnd = (int)floor(mActiveEdges[anEdge + 1].mX - 0.5);
                if (aSpanEnd >= mClipRect.mX + mClipRect.mWidth - 1)
                    aSpanEnd = mClipRect.mX + mClipRect.mWidth - 1;
                if (aSpanEnd >= aSpanStart && aSpanCount < kMaxSpans)
                {
                    aSpans[3 * aSpanCount] = aY;
                    aSpans[3 * aSpanCount + 1] = aSpanStart;
                    aSpans[3 * aSpanCount + 2] = aSpanEnd - aSpanStart + 1;
                    ++aSpanCount;
                }
                mActiveEdges[anEdge].mX += mActiveEdges[anEdge].mSlope;
                mActiveEdges[anEdge + 1].mX += mActiveEdges[anEdge + 1].mSlope;
            }
        }
    }

    mDestImage->FillScanLines(aSpans, aSpanCount, mColor, mDrawMode);
    delete[] anIndices;
    delete[] mActiveEdges;
    mActiveEdges = nullptr;
}

void Sexy::Graphics::PolyFillAA(TPoint<int> const* thePoints, int theCount, bool theConvex)
{
    if (mDestImage == nullptr || theCount <= 0)
        return;

    Color const& aColor = DrawColor();
    if (theConvex)
    {
        mDestImage->PolyFill3D(thePoints, theCount, &mClipRect, aColor, mDrawMode,
                               (int)mTransX, (int)mTransY, true);
    }

    int aMinX = thePoints[0].mX;
    int aMaxX = thePoints[0].mX;
    int aMinY = thePoints[0].mY;
    int aMaxY = thePoints[0].mY;
    for (int anIndex = 1; anIndex < theCount; ++anIndex)
    {
        int aX = thePoints[anIndex].mX;
        int aY = thePoints[anIndex].mY;
        if (aMinX > aX)
            aMinX = aX;
        if (aMaxX < aX)
            aMaxX = aX;
        if (aMinY > aY)
            aMinY = aY;
        if (aMaxY < aY)
            aMaxY = aY;
    }

    int aBoxWidth = aMaxX - aMinX;
    int aBoxHeight = aMaxY - aMinY;
    unsigned char *aCoverage;
    int aStride;
    int aCoverageHeight;
    if (aBoxWidth <= 255 && aBoxHeight <= 255)
    {
        aStride = 256;
        aCoverageHeight = 256;
        aCoverage = gCoverageTable;
        memset(aCoverage, 0, 0x10000);
    }
    else
    {
        aStride = aBoxWidth + 1;
        aCoverageHeight = aBoxHeight + 1;
        int aTableSize = aStride * aCoverageHeight;
        aCoverage = new unsigned char[aTableSize];
        memset(aCoverage, 0, aTableSize);
    }

    mPointCount = theCount;
    mPolyPoints = thePoints;
    int *aSorted = new int[theCount];
    mActiveEdges = new ActiveEdge[theCount];
    for (int anIndex = 0; anIndex < theCount; ++anIndex)
        aSorted[anIndex] = anIndex;
    qsort(aSorted, theCount, sizeof(int), ComparePointY);
    mActiveEdgeCount = 0;

    int aFirstY = (int)ceil((double)thePoints[aSorted[0]].mY + mTransY - 0.5);
    int aYStart = (aFirstY < mClipRect.mY) ? mClipRect.mY : aFirstY;
    int aLastY = (int)floor(mTransY + (double)thePoints[aSorted[theCount - 1]].mY - 0.5);
    int aYEnd = mClipRect.mY + mClipRect.mHeight - 1;
    if (aLastY < aYEnd)
        aYEnd = aLastY;

    int aSpans[3 * kMaxSpans];
    int aSpanCount = 0;
    if (aYStart <= aYEnd)
    {
        int aClipRight = mClipRect.mX + mClipRect.mWidth - 1;
        int aSortedIndex = 0;
        int aRowOffset = (aYStart - aMinY) * aStride - aMinX;

        for (int aY = aYStart; aY <= aYEnd; ++aY)
        {
            if (aSortedIndex < mPointCount)
            {
                int aPoint = aSorted[aSortedIndex];
                double aLower = (double)aY + 0.5;
                if ((float)((float)thePoints[aPoint].mY + mTransY) <= aLower)
                {
                    double aUpper = (double)aY - 0.5;
                    for (;;)
                    {
                        int aPrev = (aPoint <= 0) ? mPointCount - 1 : aPoint - 1;
                        float aPrevY = mTransY + (float)thePoints[aPrev].mY;
                        if (aPrevY > aUpper)
                        {
                            if (aPrevY > aLower)
                                InsertEdge(aPrev, aY);
                        }
                        else
                        {
                            DeleteEdge(aPrev);
                        }

                        int aNext = (mPointCount - 1 <= aPoint) ? 0 : aPoint + 1;
                        float aNextY = (float)thePoints[aNext].mY + mTransY;
                        if (aNextY <= aUpper)
                            DeleteEdge(aPoint);
                        else if (aNextY > aLower)
                            InsertEdge(aPoint, aY);

                        if (++aSortedIndex >= mPointCount)
                            break;
                        aPoint = aSorted[aSortedIndex];
                        if (!((float)((float)thePoints[aPoint].mY + mTransY) <= aLower))
                            break;
                    }
                }
            }

            qsort(mActiveEdges, mActiveEdgeCount, sizeof(ActiveEdge), CompareEdgeX);
            int anEdgeCount = mActiveEdgeCount;
            for (int anEdge = 0; anEdge + 1 < anEdgeCount && aSpanCount <= 0x1FFF; anEdge += 2)
            {
                ActiveEdge &aLeft = mActiveEdges[anEdge];
                ActiveEdge &aRight = mActiveEdges[anEdge + 1];

                int aStartX = (int)ceil(aLeft.mX - 0.5);
                int aLeftCoverage;
                if (mClipRect.mX > aStartX)
                {
                    aStartX = mClipRect.mX;
                    aLeftCoverage = 255;
                }
                else
                {
                    aLeftCoverage = (int)(fabs(aLeft.mX - (double)aStartX - 0.5) * 255.0);
                }

                int anEndX = (int)floor(aRight.mX - 0.5);
                int aRightCoverage;
                if (aClipRight >= anEndX)
                    aRightCoverage = (int)(fabs(aRight.mX - (double)anEndX - 0.5) * 255.0);
                else
                {
                    anEndX = aClipRight;
                    aRightCoverage = 255;
                }
                if (aStartX > anEndX)
                    continue;

                aSpans[3 * aSpanCount] = aY;
                aSpans[3 * aSpanCount + 1] = aStartX;
                aSpans[3 * aSpanCount + 2] = anEndX - aStartX + 1;
                ++aSpanCount;

                if (anEndX == aStartX)
                {
                    int aNewCoverage = aCoverage[aRowOffset + anEndX]
                                     + ((aRightCoverage * aLeftCoverage) >> 8);
                    if (aNewCoverage > 254)
                        aNewCoverage = 255;
                    aCoverage[aRowOffset + anEndX] = (unsigned char)aNewCoverage;
                }
                else
                {
                    double aLeftSlope = fabs(aLeft.mSlope);
                    if (aLeftSlope <= 1.0)
                    {
                        int aNewCoverage = aLeftCoverage + aCoverage[aRowOffset + aStartX];
                        if (aNewCoverage > 254)
                            aNewCoverage = 255;
                        aCoverage[aRowOffset + aStartX] = (unsigned char)aNewCoverage;
                        ++aStartX;
                    }
                    else
                    {
                        double anInvSlope = 1.0 / aLeft.mSlope;
                        double aRamp = aLeft.mY - 0.5 - (double)aY;
                        do
                        {
                            int aValue = (int)(fabs(aRamp + (double)aStartX * anInvSlope) * 255.0);
                            int aNewCoverage = aCoverage[aRowOffset + aStartX] + ((aValue > 255) ? 255 : aValue);
                            aCoverage[aRowOffset + aStartX] = (unsigned char)((aNewCoverage > 254) ? 255 : aNewCoverage);
                            aLeftSlope -= 1.0;
                            ++aStartX;
                        }
                        while (aStartX <= anEndX && aLeftSlope > 0.0);
                    }

                    double aRightSlope = fabs(aRight.mSlope);
                    if (aRightSlope <= 1.0)
                    {
                        --anEndX;
                        int aNewCoverage = aRightCoverage + aCoverage[aRowOffset + anEndX];
                        if (aNewCoverage > 254)
                            aNewCoverage = 255;
                        aCoverage[aRowOffset + anEndX] = (unsigned char)aNewCoverage;
                    }
                    else
                    {
                        double anInvSlope = 1.0 / aRight.mSlope;
                        double aRamp = aRight.mY - 0.5 - (double)aY;
                        do
                        {
                            int aValue = (int)(fabs(aRamp + (double)anEndX * anInvSlope) * 255.0);
                            int aNewCoverage = aCoverage[aRowOffset + anEndX] + ((aValue > 255) ? 255 : aValue);
                            aCoverage[aRowOffset + anEndX] = (unsigned char)((aNewCoverage > 254) ? 255 : aNewCoverage);
                            aRightSlope -= 1.0;
                            --anEndX;
                        }
                        while (anEndX >= aStartX && aRightSlope > 0.0);
                    }

                    if (aStartX <= anEndX)
                        memset(aCoverage + aRowOffset + aStartX, 255, anEndX - aStartX + 1);
                }
            }

            for (int anEdge = 0; anEdge + 1 < anEdgeCount; anEdge += 2)
            {
                mActiveEdges[anEdge].mX += mActiveEdges[anEdge].mSlope;
                mActiveEdges[anEdge + 1].mX += mActiveEdges[anEdge + 1].mSlope;
            }

            if (aY < aYEnd)
                aRowOffset += aStride;
        }
    }

    mDestImage->FillScanLinesWithCoverage(aSpans, aSpanCount, aColor, mDrawMode,
                                          aCoverage, aMinX, aMinY, aStride, aCoverageHeight);
    if (aCoverage != gCoverageTable)
        delete[] aCoverage;
    delete[] aSorted;
    delete[] mActiveEdges;
    mActiveEdges = nullptr;
}

bool Sexy::Graphics::ClipLine(double *theStartX, double *theStartY, double *theEndX, double *theEndY)
{
    double aX2 = *theStartX;
    double aX1 = *theEndX;
    double aY2 = *theStartY;
    double aY1 = *theEndY;
    if (*theStartX <= *theEndX)
    {
        aY1 = *theStartY;
        aX1 = *theStartX;
        aY2 = *theEndY;
        aX2 = *theEndX;
    }

    double aClipX = mClipRect.mX;
    if (aClipX > aX1)
    {
        if (aClipX > aX2)
            return false;
        aY1 += (aClipX - aX1) * ((aY2 - aY1) / (aX2 - aX1));
        aX1 = mClipRect.mX;
    }

    int aClipRight = mClipRect.mX + mClipRect.mWidth;
    if ((double)aClipRight <= aX2)
    {
        if ((double)aClipRight <= aX1)
            return false;
        aY2 += (aY2 - aY1) / (aX2 - aX1) * ((double)(aClipRight - 1) - aX2);
        aX2 = (double)(aClipRight - 1);
    }

    if (aY2 >= aY1)
    {
        double aSwapY = aY2;
        double aSwapX = aX2;
        aY2 = aY1;
        aX2 = aX1;
        aY1 = aSwapY;
        aX1 = aSwapX;
    }

    double aClipY = mClipRect.mY;
    if (aClipY > aY2)
    {
        if (aY1 < aClipY)
            return false;
        aX2 += (aClipY - aY2) * ((aX1 - aX2) / (aY1 - aY2));
        aY2 = mClipRect.mY;
    }

    int aClipBottom = mClipRect.mY + mClipRect.mHeight;
    if (aY1 < (double)aClipBottom)
    {
        *theStartX = aX2;
        *theStartY = aY2;
        *theEndX = aX1;
        *theEndY = aY1;
        return true;
    }
    if (aY2 < (double)aClipBottom)
    {
        aX1 += (aX1 - aX2) / (aY1 - aY2) * ((double)(aClipBottom - 1) - aY1);
        aY1 = (double)(aClipBottom - 1);
        *theStartX = aX2;
        *theStartY = aY2;
        *theEndX = aX1;
        *theEndY = aY1;
        return true;
    }
    return false;
}

Sexy::Graphics* Sexy::Graphics::DrawLine(int theStartX, int theStartY, int theEndX, int theEndY)
{
    if (mDestImage == nullptr)
        return this;
    Color const& aColor = DrawColor();
    double aX1 = (float)((float)theStartX + mTransX);
    double aX2 = (float)((float)theEndX + mTransX);
    double aY1 = (float)((float)theStartY + mTransY);
    double aY2 = (float)((float)theEndY + mTransY);
    if (!ClipLine(&aX1, &aY1, &aX2, &aY2))
        return this;

    if (mApplyGlobalScale && (mScaleX != 1.0f || mScaleY != 1.0f))
    {
        double aScaledWidth = (double)(int)ceil((float)((float)(theEndX - theStartX) * mScaleX));
        double aScaledHeight = (double)(int)ceil((float)((float)(theEndY - theStartY) * mScaleX));
        aX1 = floor((aX1 - mScaleOrigX) * (double)mScaleX) + mScaleOrigX;
        aY1 = floor((double)mScaleY * (aY1 - mScaleOrigY)) + mScaleOrigY;
        aX2 = aScaledWidth + aX1;
        aY2 = aScaledHeight + aY1;
    }
    mDestImage->DrawLine(aX1, aY1, aX2, aY2, aColor, mDrawMode);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawLineAA(int theStartX, int theStartY, int theEndX, int theEndY)
{
    if (mDestImage == nullptr)
        return this;
    Color const& aColor = DrawColor();
    double aX1 = (float)((float)theStartX + mTransX);
    double aX2 = (float)((float)theEndX + mTransX);
    double aY1 = (float)((float)theStartY + mTransY);
    double aY2 = (float)((float)theEndY + mTransY);
    if (!ClipLine(&aX1, &aY1, &aX2, &aY2))
        return this;

    if (mApplyGlobalScale && (mScaleX != 1.0f || mScaleY != 1.0f))
    {
        double aScaledWidth = (double)(int)ceil((float)((float)(theEndX - theStartX) * mScaleX));
        double aScaledHeight = (double)(int)ceil((float)((float)(theEndY - theStartY) * mScaleX));
        aX1 = floor((aX1 - mScaleOrigX) * (double)mScaleX) + mScaleOrigX;
        aY1 = floor((double)mScaleY * (aY1 - mScaleOrigY)) + mScaleOrigY;
        aX2 = aScaledWidth + aX1;
        aY2 = aScaledHeight + aY1;
    }
    mDestImage->DrawLineAA(aX1, aY1, aX2, aY2, aColor, mDrawMode);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawImage(Sexy::Image * theImage, int theX, int theY, TRect<int> const& theSrcRect)
{
    if (theImage == nullptr || mDestImage == nullptr)
        return this;
    if (theSrcRect.mX + theSrcRect.mWidth > theImage->GetWidth())
        return this;
    if (theSrcRect.mY + theSrcRect.mHeight > theImage->GetHeight())
        return this;

    int aX = theX + (int)mTransX;
    int aY = theY + (int)mTransY;
    if (mScaleX == 1.0f && mScaleY == 1.0f)
    {
        int aClippedX = (mClipRect.mX < aX) ? aX : mClipRect.mX;
        int aClippedWidth = (mClipRect.mX + mClipRect.mWidth > aX + theSrcRect.mWidth)
                              ? aX + theSrcRect.mWidth - aClippedX
                              : mClipRect.mX + mClipRect.mWidth - aClippedX;
        int aClippedY = (mClipRect.mY < aY) ? aY : mClipRect.mY;
        int aClippedBottom = theSrcRect.mHeight + aY;
        if (mClipRect.mHeight + mClipRect.mY < aClippedBottom)
            aClippedBottom = mClipRect.mHeight + mClipRect.mY;
        int aClippedHeight = aClippedBottom - aClippedY;
        if (aClippedWidth <= 0 || aClippedHeight <= 0)
            return this;
        TRect<int> aSrc(theSrcRect.mX + aClippedX - aX, theSrcRect.mY + aClippedY - aY,
                        aClippedWidth, aClippedHeight);
        mDestImage->Blt(theImage, aClippedX, aClippedY, aSrc, GetImageColor(), mDrawMode);
        return this;
    }

    TRect<int> aDest((int)(floor((float)((float)aX - mScaleOrigX) * mScaleX) + mScaleOrigX),
                     (int)(floor((float)mScaleY * (float)((float)aY - mScaleOrigY)) + mScaleOrigY),
                     (int)ceil((float)theSrcRect.mWidth * mScaleX),
                     (int)ceil((float)mScaleY * (float)theSrcRect.mHeight));
    mDestImage->StretchBlt(theImage, aDest, theSrcRect, mClipRect, GetImageColor(), mDrawMode, mFastStretch);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawImage(Sexy::Image * theImage, int theX, int theY)
{
    if (theImage == nullptr || mDestImage == nullptr)
        return this;
    if (mScaleX == 1.0f && mScaleY == 1.0f)
    {
        int aX = (int)mTransX + theX;
        int aY = (int)mTransY + theY;
        int aClippedX = (mClipRect.mX < aX) ? aX : mClipRect.mX;
        int aClippedWidth = (aX + theImage->GetWidth() > mClipRect.mX + mClipRect.mWidth)
                              ? mClipRect.mX + mClipRect.mWidth - aClippedX
                              : aX + theImage->GetWidth() - aClippedX;
        int aClippedY = (mClipRect.mY < aY) ? aY : mClipRect.mY;
        int aClippedBottom = aY + theImage->GetHeight();
        if (aClippedBottom > mClipRect.mY + mClipRect.mHeight)
            aClippedBottom = mClipRect.mY + mClipRect.mHeight;
        int aClippedHeight = aClippedBottom - aClippedY;
        if (aClippedWidth <= 0 || aClippedHeight <= 0)
            return this;
        TRect<int> aSrc(aClippedX - aX, aClippedY - aY, aClippedWidth, aClippedHeight);
        mDestImage->Blt(theImage, aClippedX, aClippedY, aSrc, GetImageColor(), mDrawMode);
        return this;
    }
    return DrawImage(theImage, theX, theY, TRect<int>(0, 0, theImage->GetWidth(), theImage->GetHeight()));
}

Sexy::Graphics* Sexy::Graphics::DrawImageMirror(Sexy::Image * theImage, int theX, int theY, TRect<int> const& theSrcRect, bool theMirror)
{
    if (!theMirror)
        return DrawImage(theImage, theX, theY, theSrcRect);
    if (theImage == nullptr || mDestImage == nullptr)
        return this;
    if (theSrcRect.mX + theSrcRect.mWidth > theImage->GetWidth())
        return this;
    if (theSrcRect.mY + theSrcRect.mHeight > theImage->GetHeight())
        return this;

    int aX = (int)mTransX + theX;
    int aY = theY + (int)mTransY;
    int aClippedX = (mClipRect.mX < aX) ? aX : mClipRect.mX;
    int aClippedWidth = (mClipRect.mX + mClipRect.mWidth > aX + theSrcRect.mWidth)
                          ? aX + theSrcRect.mWidth - aClippedX
                          : mClipRect.mX + mClipRect.mWidth - aClippedX;
    int aClippedY = (mClipRect.mY < aY) ? aY : mClipRect.mY;
    int aClippedBottom = mClipRect.mY + mClipRect.mHeight;
    if (aClippedBottom >= theSrcRect.mHeight + aY)
        aClippedBottom = theSrcRect.mHeight + aY;
    int aClippedHeight = aClippedBottom - aClippedY;
    if (aClippedWidth <= 0 || aClippedHeight <= 0)
        return this;

    TRect<int> aSrc(theSrcRect.mX + theSrcRect.mWidth - aClippedWidth - (aClippedX - aX),
                    theSrcRect.mY + aClippedY - aY,
                    aClippedWidth, aClippedHeight);
    if (mApplyGlobalScale && (mScaleX != 1.0f || mScaleY != 1.0f))
    {
        aClippedX = (int)(floor((float)((float)aClippedX - mScaleOrigX) * mScaleX) + mScaleOrigX);
        aClippedY = (int)(floor((float)mScaleY * (float)((float)aClippedY - mScaleOrigY)) + mScaleOrigY);
    }
    mDestImage->BltMirror(theImage, aClippedX, aClippedY, aSrc, GetImageColor(), mDrawMode);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawImageMirror(Sexy::Image * theImage, int theX, int theY, bool theMirror)
{
    if (theImage == nullptr)
        return this;
    return DrawImageMirror(theImage, theX, theY, TRect<int>(0, 0, theImage->GetWidth(), theImage->GetHeight()), theMirror);
}

Sexy::Graphics* Sexy::Graphics::DrawImage(Sexy::Image * theImage, int theX, int theY, int theWidth, int theHeight)
{
    if (theImage == nullptr || mDestImage == nullptr)
        return this;
    int aX = (int)((float)theX + mTransX);
    int aY = (int)((float)theY + mTransY);
    int aWidth = theWidth;
    int aHeight = theHeight;
    if (mApplyGlobalScale && (mScaleX != 1.0f || mScaleY != 1.0f))
    {
        float aScaleX = mScaleX;
        float aScaleY = mScaleY;
        aX = (int)(floor((float)((float)aX - mScaleOrigX) * aScaleX) + mScaleOrigX);
        aY = (int)(floor((float)aScaleY * (float)((float)aY - mScaleOrigY)) + mScaleOrigY);
        aWidth = (int)ceil((float)((float)theWidth * aScaleX));
        aHeight = (int)ceil((float)(aScaleY * (float)theHeight));
    }
    TRect<int> aDest(aX, aY, aWidth, aHeight);
    TRect<int> aSrc(0, 0, theImage->mWidth, theImage->mHeight);
    mDestImage->StretchBlt(theImage, aDest, aSrc, mClipRect, GetImageColor(), mDrawMode, mFastStretch);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawImage(Sexy::Image * theImage, TRect<int> const& theDestRect, TRect<int> const& theSrcRect)
{
    if (theImage == nullptr || mDestImage == nullptr)
        return this;

    int aDestX = (int)((float)theDestRect.mX + mTransX);
    int aDestY = (int)((float)theDestRect.mY + mTransY);
    int aDestWidth = theDestRect.mWidth;
    int aDestHeight = theDestRect.mHeight;
    if (mApplyGlobalScale && (mScaleX != 1.0f || mScaleY != 1.0f))
    {
        aDestX = (int)(floor((float)((float)aDestX - mScaleOrigX) * mScaleX) + mScaleOrigX);
        aDestY = (int)(floor((float)mScaleY * (float)((float)aDestY - mScaleOrigY)) + mScaleOrigY);
        aDestWidth = (int)ceil((float)((float)theDestRect.mWidth * mScaleX));
        aDestHeight = (int)ceil((float)(mScaleY * (float)theDestRect.mHeight));
    }

    int anImageWidth = theImage->GetWidth();
    int anImageHeight = theImage->GetHeight();
    int aClipX = theSrcRect.mX < 0 ? 0 : theSrcRect.mX;
    int aSrcRight = theSrcRect.mX + theSrcRect.mWidth;
    int aClipWidth = (anImageWidth > aSrcRight) ? aSrcRight - aClipX : anImageWidth - aClipX;
    int aSrcBottom = theSrcRect.mY + theSrcRect.mHeight;
    if (anImageHeight >= aSrcBottom)
        anImageHeight = aSrcBottom;
    int aClipY = theSrcRect.mY < 0 ? 0 : theSrcRect.mY;
    int aClipHeight = anImageHeight - aClipY;
    if (aClipWidth < 0 || aClipHeight < 0)
    {
        aClipX = 0;
        aClipY = 0;
        aClipWidth = 0;
        aClipHeight = 0;
    }
    TRect<int> aClippedSrc(aClipX, aClipY, aClipWidth, aClipHeight);

    bool aNeedsScale = theSrcRect.mX != aClipX
                    || aClipY != theSrcRect.mY
                    || theSrcRect.mWidth != aClipWidth
                    || theSrcRect.mHeight != aClipHeight;
    if (aNeedsScale && theSrcRect.mWidth != 0 && theSrcRect.mHeight != 0)
    {
        float aRatioX = (float)aDestWidth / (float)theSrcRect.mWidth;
        float aRatioY = (float)aDestHeight / (float)theSrcRect.mHeight;
        TRect<int> aScaled((int)(float)((float)aDestX + (float)((float)(aClipX - theSrcRect.mX) * aRatioX)),
                           (int)(float)((float)aDestY + (float)((float)(aClipY - theSrcRect.mY) * aRatioY)),
                           (int)ceilf((float)aClipWidth * aRatioX),
                           (int)ceilf((float)aClipHeight * aRatioY));
        mDestImage->StretchBlt(theImage, aScaled, aClippedSrc, mClipRect,
                               GetImageColor(), mDrawMode, mFastStretch);
        return this;
    }
    TRect<int> aDest(aDestX, aDestY, aDestWidth, aDestHeight);
    mDestImage->StretchBlt(theImage, aDest, theSrcRect, mClipRect,
                           GetImageColor(), mDrawMode, mFastStretch);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawImageMirror(Sexy::Image * theImage, TRect<int> const& theDestRect, TRect<int> const& theSrcRect, bool theMirror)
{
    if (!theMirror)
        return DrawImage(theImage, theDestRect, theSrcRect);
    if (theImage == nullptr || mDestImage == nullptr)
        return this;

    int aX = (int)((float)theDestRect.mX + mTransX);
    int aY = (int)((float)theDestRect.mY + mTransY);
    int aWidth = theDestRect.mWidth;
    int aHeight = theDestRect.mHeight;
    if (mApplyGlobalScale && (mScaleX != 1.0f || mScaleY != 1.0f))
    {
        aX = (int)(floor((float)((float)aX - mScaleOrigX) * mScaleX) + mScaleOrigX);
        aY = (int)(floor((float)mScaleY * (float)((float)aY - mScaleOrigY)) + mScaleOrigY);
        aWidth = (int)ceil((float)((float)theDestRect.mWidth * mScaleX));
        aHeight = (int)ceil((float)(mScaleY * (float)theDestRect.mHeight));
    }
    TRect<int> aDest(aX, aY, aWidth, aHeight);
    mDestImage->StretchBltMirror(theImage, aDest, theSrcRect, mClipRect,
                                 GetImageColor(), mDrawMode, mFastStretch);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawImageF(Sexy::Image * theImage, float theX, float theY, TRect<int> const& theSrcRect)
{
    if (theImage == nullptr || mDestImage == nullptr)
        return this;
    float aX = theX + mTransX;
    float aY = theY + mTransY;
    if (mScaleX == 1.0f && mScaleY == 1.0f)
    {
        mDestImage->BltF(theImage, aX, aY, theSrcRect, mClipRect, GetImageColor(), mDrawMode);
        return this;
    }
    TRect<int> aDest((int)(mScaleOrigX + (float)((float)(aX - mScaleOrigX) * mScaleX)),
                     (int)(mScaleOrigY + (float)(mScaleY * (float)(aY - mScaleOrigY))),
                     (int)(float)((float)theSrcRect.mWidth * mScaleX),
                     (int)(float)(mScaleY * (float)theSrcRect.mHeight));
    mDestImage->StretchBlt(theImage, aDest, theSrcRect, mClipRect, GetImageColor(), mDrawMode, mFastStretch);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawImageF(Sexy::Image * theImage, float theX, float theY)
{
    if (theImage == nullptr || mDestImage == nullptr)
        return this;
    if (mScaleX == 1.0f && mScaleY == 1.0f)
    {
        TRect<int> aSrc(0, 0, theImage->mWidth, theImage->mHeight);
        mDestImage->BltF(theImage, theX + mTransX, theY + mTransY, aSrc, mClipRect, GetImageColor(), mDrawMode);
        return this;
    }
    return DrawImageF(theImage, theX, theY, TRect<int>(0, 0, theImage->mWidth, theImage->mHeight));
}

void Sexy::Graphics::DrawImageRotatedF(Sexy::Image * theImage, float theX, float theY, double theRot, float theCenterX, float theCenterY, TRect<int> const* theSrcRect)
{
    if (theImage == nullptr || mDestImage == nullptr)
        return;
    float aX = theX + mTransX;
    float aY = theY + mTransY;
    if (theSrcRect != nullptr)
    {
        mDestImage->BltRotated(theImage, aX, aY, *theSrcRect, mClipRect, GetImageColor(),
                               mDrawMode, theRot, theCenterX, theCenterY);
        return;
    }
    TRect<int> aSrc(0, 0, theImage->mWidth, theImage->mHeight);
    mDestImage->BltRotated(theImage, aX, aY, aSrc, mClipRect, GetImageColor(),
                           mDrawMode, theRot, theCenterX, theCenterY);
}

void Sexy::Graphics::DrawImageRotated(Sexy::Image * theImage, int theX, int theY, double theRot, TRect<int> const* theSrcRect)
{
    if (theImage == nullptr)
        return;
    if (theSrcRect != nullptr)
    {
        DrawImageRotatedF(theImage, (float)theX, (float)theY, theRot,
                          (float)(theSrcRect->mWidth / 2), (float)(theSrcRect->mHeight / 2), theSrcRect);
        return;
    }
    DrawImageRotatedF(theImage, (float)theX, (float)theY, theRot,
                      (float)(theImage->GetWidth() / 2), (float)(theImage->GetHeight() / 2), nullptr);
}

void Sexy::Graphics::DrawImageRotatedF(Sexy::Image * theImage, float theX, float theY, double theRot, TRect<int> const* theSrcRect)
{
    if (theImage == nullptr)
        return;
    if (theSrcRect != nullptr)
    {
        DrawImageRotatedF(theImage, theX, theY, theRot,
                          (float)theSrcRect->mWidth * 0.5f, (float)theSrcRect->mHeight * 0.5f, theSrcRect);
        return;
    }
    DrawImageRotatedF(theImage, theX, theY, theRot,
                      (float)theImage->GetWidth() * 0.5f, (float)theImage->GetHeight() * 0.5f, nullptr);
}

void Sexy::Graphics::DrawImageRotated(Sexy::Image * theImage, int theX, int theY, double theRot, int theCenterX, int theCenterY, TRect<int> const* theSrcRect)
{
    DrawImageRotatedF(theImage, (float)theX, (float)theY, theRot, (float)theCenterX, (float)theCenterY, theSrcRect);
}

Sexy::Graphics* Sexy::Graphics::DrawImageMatrix(Sexy::Image * theImage, SexyMatrix3 const& theMatrix, float theX, float theY, bool theMirror)
{
    if (theImage == nullptr || mDestImage == nullptr)
        return this;
    TRect<int> aSrc(0, 0, theImage->mWidth, theImage->mHeight);
    if (ScaleIsIdentity())
    {
        mDestImage->BltMatrix(theImage, theX + mTransX, theY + mTransY, theMatrix, aSrc,
                              GetImageColor(), mDrawMode, mClipRect, mLinearBlend, theMirror);
        return this;
    }
    float aOrigX = mScaleOrigX;
    float aOrigY = mScaleOrigY;
    double aScaledX = floor((float)((float)((float)(mTransX - aOrigX) + theX) * mScaleX));
    double aScaledY = floor((float)(mScaleY * (float)((float)(mTransY - aOrigY) + theY)));
    SexyTransform2D aTransform(theMatrix);
    aTransform.Scale(mScaleX, mScaleY);
    Color const& aColor = mColorizeImages ? mColor : Color::White;
    mDestImage->BltMatrix(theImage, (float)aScaledX + aOrigX, (float)aScaledY + aOrigY,
                          aTransform, aSrc, aColor, mDrawMode, mClipRect, mLinearBlend, theMirror);
    return this;
}

Sexy::Graphics* Sexy::Graphics::DrawImageMatrix(Sexy::Image * theImage, SexyMatrix3 const& theMatrix, TRect<int> const& theSrcRect, float theX, float theY, bool theMirror)
{
    if (theImage == nullptr || mDestImage == nullptr)
        return this;
    if (ScaleIsIdentity())
    {
        mDestImage->BltMatrix(theImage, theX + mTransX, theY + mTransY, theMatrix, theSrcRect,
                              GetImageColor(), mDrawMode, mClipRect, mLinearBlend, theMirror);
        return this;
    }
    float aOrigX = mScaleOrigX;
    float aOrigY = mScaleOrigY;
    double aScaledX = floor((float)((float)((float)(mTransX - aOrigX) + theX) * mScaleX));
    double aScaledY = floor((float)(mScaleY * (float)((float)(mTransY - aOrigY) + theY)));
    SexyTransform2D aTransform(theMatrix);
    aTransform.Scale(mScaleX, mScaleY);
    mDestImage->BltMatrix(theImage, (float)aScaledX + aOrigX, (float)aScaledY + aOrigY,
                          aTransform, theSrcRect, GetImageColor(), mDrawMode, mClipRect, mLinearBlend, theMirror);
    return this;
}

void Sexy::Graphics::DrawImageTransformHelper(Sexy::Image * theImage, Sexy::Transform const& theTransform, TRect<int> const& theSrcRect, float theX, float theY, bool theUseFloat)
{
    if (theTransform.mNeedCalc)
    {
        Sexy::Transform aLocal = theTransform;
        SexyMatrix3 *aMatrix = aLocal.CalcMatrix();
        if (aMatrix != nullptr)
            DrawImageMatrix(theImage, *aMatrix, theSrcRect, theX, theY, true);
        return;
    }

    float aHalfWidth = (float)theSrcRect.mWidth * 0.5f;
    float aHalfHeight = (float)theSrcRect.mHeight * 0.5f;
    if (theTransform.mRotated)
    {
        float aCenterX = aHalfWidth - theTransform.mTransX;
        float aCenterY = aHalfHeight - theTransform.mTransY;
        float aX = (float)((float)(theTransform.mRotateCenterX + 0.5f) - aCenterX) + theX;
        float aY = (float)((float)(theTransform.mRotateCenterY + 0.5f) + theY) - aCenterY;
        if (theUseFloat)
            DrawImageRotatedF(theImage, aX, aY, theTransform.mRotation, aCenterX, aCenterY, &theSrcRect);
        else
            DrawImageRotated(theImage, (int)aX, (int)aY, theTransform.mRotation, (int)aCenterX, (int)aCenterY, &theSrcRect);
        return;
    }
    if (theTransform.mScaled)
    {
        float aScaleX = theTransform.mScaleX;
        float aScaleY = theTransform.mScaleY;
        bool aMirror = false;
        if (aScaleX == -1.0f)
        {
            if (aScaleY == 1.0f)
            {
                DrawImageMirror(theImage,
                    (int)(float)((float)((float)(theTransform.mTransX + theTransform.mRotateCenterX) + 0.5f) - aHalfWidth + theX),
                    (int)(float)((float)((float)(theTransform.mTransY + theTransform.mRotateCenterY) + 0.5f) - aHalfHeight + theY),
                    theSrcRect, true);
                return;
            }
            aMirror = true;
        }
        TRect<int> aDest((int)(float)((float)(theTransform.mRotateCenterX - aHalfWidth * aScaleX) + theX),
                         (int)(float)((float)(theY + theTransform.mRotateCenterY) - aHalfHeight * aScaleY),
                         (int)(float)(aHalfWidth * aScaleX * 2.0f),
                         (int)(float)(aHalfHeight * aScaleY * 2.0f));
        DrawImageMirror(theImage, aDest, theSrcRect, aMirror);
        return;
    }

    float aX = (float)((float)((float)(theTransform.mTransX + theTransform.mRotateCenterX) + 0.5f) - aHalfWidth) + theX;
    float aY = (float)((float)((float)(theTransform.mTransY + theTransform.mRotateCenterY) + 0.5f) - aHalfHeight) + theY;
    if (theUseFloat)
        DrawImageF(theImage, aX, aY, theSrcRect);
    else
        DrawImage(theImage, (int)aX, (int)aY, theSrcRect);
}

void Sexy::Graphics::DrawImageTransform(Sexy::Image * theImage, Sexy::Transform const& theTransform, float theX, float theY)
{
    if (theImage == nullptr)
        return;
    DrawImageTransformHelper(theImage, theTransform, TRect<int>(0, 0, theImage->mWidth, theImage->mHeight), theX, theY, false);
}

void Sexy::Graphics::DrawImageTransformF(Sexy::Image * theImage, Sexy::Transform const& theTransform, float theX, float theY)
{
    if (theImage == nullptr)
        return;
    DrawImageTransformHelper(theImage, theTransform, TRect<int>(0, 0, theImage->mWidth, theImage->mHeight), theX, theY, true);
}

void Sexy::Graphics::DrawImageTransform(Sexy::Image * theImage, Sexy::Transform const& theTransform, TRect<int> const& theSrcRect, float theX, float theY)
{
    DrawImageTransformHelper(theImage, theTransform, theSrcRect, theX, theY, false);
}

void Sexy::Graphics::DrawImageTransformF(Sexy::Image * theImage, Sexy::Transform const& theTransform, TRect<int> const& theSrcRect, float theX, float theY)
{
    DrawImageTransformHelper(theImage, theTransform, theSrcRect, theX, theY, true);
}

void Sexy::Graphics::DrawTriangleTex(Sexy::Image * theImage, SexyVertex2D const& theA, SexyVertex2D const& theB, SexyVertex2D const& theC)
{
    SexyVertex2D aTriangle[1][3];
    aTriangle[0][0] = theA;
    aTriangle[0][1] = theB;
    aTriangle[0][2] = theC;
    DrawTrianglesTex(theImage, aTriangle, 1);
}

void Sexy::Graphics::DrawTrianglesTex(Sexy::Image * theImage, SexyVertex2D const(*theTriangles)[3], int theCount, Color const& theColor, int theDrawMode, float theTx, float theTy, bool theBlend, TRect<int> const* theClipRect)
{
    if (theImage == nullptr || mDestImage == nullptr || theTriangles == nullptr || theCount <= 0)
        return;
    TRect<int> aClip = theClipRect != nullptr ? *theClipRect : mClipRect;
    mDestImage->BltTrianglesTex(theImage, theTriangles, theCount, aClip, theColor, theDrawMode, theTx, theTy, theBlend);
}

void Sexy::Graphics::DrawTrianglesTexStrip(Sexy::Image * theImage, SexyVertex2D const* theVertices, int theCount, Color const& theColor, int theDrawMode, float theTx, float theTy, bool theBlend)
{
    if (theImage == nullptr || theVertices == nullptr || theCount <= 0)
        return;
    SexyVertex2D aChunk[100][3];
    for (int aStart = 0; aStart < theCount; )
    {
        int aCount = theCount - aStart;
        if (aCount > 100)
            aCount = 100;
        for (int aTriangle = 0; aTriangle < aCount; ++aTriangle)
        {
            for (int aVertex = 0; aVertex < 3; ++aVertex)
                aChunk[aTriangle][aVertex] = theVertices[(aStart + aTriangle) * 3 + aVertex];
        }
        DrawTrianglesTex(theImage, aChunk, aCount, theColor, theDrawMode, theTx, theTy, theBlend, nullptr);
        aStart += aCount;
    }
}

void Sexy::Graphics::DrawTrianglesTexStrip(Sexy::Image * theImage, SexyVertex2D const* theVertices, int theCount)
{
    Color const& aColor = mColorizeImages ? mColor : Color::White;
    DrawTrianglesTexStrip(theImage, theVertices, theCount, aColor, mDrawMode, mTransX, mTransY, mLinearBlend);
}

Sexy::Graphics* Sexy::Graphics::ClearClipRect()
{
    if (mDestImage == nullptr)
    {
        mClipRect = TRect<int>(0, 0, 0, 0);
        return this;
    }
    mClipRect = TRect<int>(0, 0, mDestImage->GetWidth(), mDestImage->GetHeight());
    return this;
}

void Sexy::Graphics::SetClipRect(int theX, int theY, int theWidth, int theHeight)
{
    if (mDestImage == nullptr)
    {
        mClipRect = TRect<int>(0, 0, 0, 0);
        return;
    }
    int anImageWidth = mDestImage->GetWidth();
    int anImageHeight = mDestImage->GetHeight();
    int aX = (int)(float)((float)theX + mTransX);
    int aY = (int)(float)((float)theY + mTransY);
    int aClipX = aX < 0 ? 0 : aX;
    int aClipWidth = (anImageWidth > aX + theWidth) ? aX + theWidth - aClipX : anImageWidth - aClipX;
    int aClipY = aY < 0 ? 0 : aY;
    if (anImageHeight >= theHeight + aY)
        anImageHeight = theHeight + aY;
    int aClipHeight = anImageHeight - aClipY;
    if (aClipWidth < 0 || aClipHeight < 0)
        mClipRect = TRect<int>(0, 0, 0, 0);
    else
        mClipRect = TRect<int>(aClipX, aClipY, aClipWidth, aClipHeight);
}

void Sexy::Graphics::SetClipRect(TRect<int> const& theRect)
{
    SetClipRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void Sexy::Graphics::ClipRect(int theX, int theY, int theWidth, int theHeight)
{
    int aX = (int)(float)((float)theX + mTransX);
    int aY = (int)(float)((float)theY + mTransY);
    mClipRect = mClipRect.Intersection(TRect<int>(aX, aY, theWidth, theHeight));
}

void Sexy::Graphics::ClipRect(TRect<int> const& theRect)
{
    ClipRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

Sexy::Graphics* Sexy::Graphics::Translate(int theX, int theY)
{
    mTransX += (float)theX;
    mTransY += (float)theY;
    return this;
}

Sexy::Graphics* Sexy::Graphics::TranslateF(float theX, float theY)
{
    mTransX += theX;
    mTransY += theY;
    return this;
}

Sexy::Graphics* Sexy::Graphics::SetScale(float theScaleX, float theScaleY, float theOrigX, float theOrigY)
{
    mScaleX = theScaleX;
    mScaleY = theScaleY;
    mScaleOrigX = theOrigX + mTransX;
    mScaleOrigY = theOrigY + mTransY;
    return this;
}

int Sexy::Graphics::StringWidth(Sexy::String const& theString)
{
    if (mFont == nullptr)
        return 0;
    return mFont->StringWidth(theString);
}

Sexy::Graphics* Sexy::Graphics::DrawImageCel(Sexy::Image * theImage, int theX, int theY, int theCol, int theRow)
{
    if (theImage == nullptr)
        return this;
    if (theCol < 0 || theRow < 0 || theRow >= theImage->mNumRows || theCol >= theImage->mNumCols)
        return this;
    int aCelWidth = theImage->mWidth / theImage->mNumCols;
    int aCelHeight = theImage->mHeight / theImage->mNumRows;
    return DrawImage(theImage, theX, theY, TRect<int>(theCol * aCelWidth, theRow * aCelHeight, aCelWidth, aCelHeight));
}

Sexy::Graphics* Sexy::Graphics::DrawImageCel(Sexy::Image * theImage, int theX, int theY, int theCel)
{
    if (theImage == nullptr || theImage->mNumCols == 0)
        return this;
    return DrawImageCel(theImage, theX, theY, theCel % theImage->mNumCols, theCel / theImage->mNumCols);
}

Sexy::Graphics* Sexy::Graphics::DrawImageAnim(Sexy::Image * theImage, int theX, int theY, int theTime)
{
    if (theImage == nullptr || theImage->mAnimInfo == nullptr)
        return this;
    return DrawImageCel(theImage, theX, theY, theImage->mAnimInfo->GetCel(theTime));
}

Sexy::Graphics* Sexy::Graphics::DrawImageCel(Sexy::Image * theImage, TRect<int> const& theDestRect, int theCol, int theRow)
{
    if (theImage == nullptr)
        return this;
    if (theCol < 0 || theRow < 0 || theRow >= theImage->mNumRows || theCol >= theImage->mNumCols)
        return this;
    int aCelWidth = theImage->mWidth / theImage->mNumCols;
    int aCelHeight = theImage->mHeight / theImage->mNumRows;
    return DrawImage(theImage, theDestRect, TRect<int>(theCol * aCelWidth, theRow * aCelHeight, aCelWidth, aCelHeight));
}

Sexy::Graphics* Sexy::Graphics::DrawImageCel(Sexy::Image * theImage, TRect<int> const& theDestRect, int theCel)
{
    if (theImage == nullptr || theImage->mNumCols == 0)
        return this;
    return DrawImageCel(theImage, theDestRect, theCel % theImage->mNumCols, theCel / theImage->mNumCols);
}

Sexy::Graphics* Sexy::Graphics::DrawString(Sexy::String const& theString, int theX, int theY)
{
    if (mFont == nullptr)
        return this;

    Color const& aColor = DrawColor();
    // Sexy::String already holds the label. Widening it and calling the base
    // Font::DrawStringUnicode drops the glyphs, so the image font draws the
    // bytes it was given.
    mFont->DrawString(this, theX, theY, theString, aColor, mClipRect);

    if (!DebugDrawStringEnabled())
        return this;
    int aWidth = mFont->StringWidth(theString);
    DrawRect(theX, theY - mFont->GetAscent(), aWidth, mFont->GetHeight());
    return this;
}

void Sexy::Graphics::BindDestination(Sexy::Graphics const* theOther)
{
    mGraphics3D = nullptr;
    mIs3D = false;
    if (gSexyAppBase == nullptr || gSexyAppBase->mDDInterface == nullptr)
        return;

    IGraphicsDriver *aDriver = gSexyAppBase->mDDInterface;
    Image *aDest = theOther != nullptr ? theOther->mDestImage : mDestImage;
    RenderDevice3D *aDevice3D = aDriver->GetRenderDevice3D();
    if (aDevice3D != nullptr)
    {
        MemoryImage *aMemory = nullptr;
        if (aDest != nullptr)
            aMemory = aDest->AsMemoryImage();
        void *aRenderData = aDevice3D->CreateImageRenderData(aMemory);
        if (aRenderData != nullptr)
        {
            HRenderContext aContext;
            aContext.mValue = 0;
            mGraphics3D = new Graphics3D(this, aDevice3D, aContext);
            mIs3D = true;
            return;
        }
    }
    if (mDestImage != nullptr)
        mIs3D = mDestImage->Is3D();
}

Sexy::Graphics::Graphics(Sexy::Image * theDestImage)
    : mGraphics3D(nullptr)
    , mActiveEdges(nullptr)
    , mActiveEdgeCount(0)
    , mPointCount(0)
{
    mClipRect = TRect<int>();
    mFinalColor = Color();
    mColor = Color();
    mTransX = 0.0f;
    mTransY = 0.0f;
    mScaleOrigX = 0.0f;
    mScaleOrigY = 0.0f;
    mDestImage = theDestImage;
    mScaleX = 1.0f;
    mScaleY = 1.0f;
    mFont = nullptr;
    mDrawMode = DRAWMODE_NORMAL;
    mColorizeImages = false;
    mFastStretch = false;
    mWriteColoredString = true;
    mLinearBlend = true;
    mApplyGlobalScale = mDefaultGlobalScale;
    mTrackDeviceState = mDefaultTrackDeviceState;

    if (theDestImage != nullptr)
        mIs3D = theDestImage->Is3D();
    else
    {
        mIs3D = false;
        mDestImage = &mStaticImage;
    }
    mClipRect = TRect<int>(0, 0, mDestImage->GetWidth(), mDestImage->GetHeight());
    BindDestination(nullptr);
}

Sexy::Graphics::Graphics(Sexy::Graphics const& theOther)
    : mGraphics3D(nullptr)
    , mActiveEdges(nullptr)
    , mActiveEdgeCount(0)
    , mPointCount(0)
{
    mClipRect = TRect<int>();
    mFinalColor = Color();
    mColor = Color();
    CopyStateFrom(&theOther);
    BindDestination(&theOther);
}

Sexy::Graphics * Sexy::Graphics::Create()
{
    return new Graphics(*this);
}

void Sexy::Graphics::DrawImageBox(TRect<int> const& theSrc, TRect<int> const& theDest, Sexy::Image * theImage)
{
    if (theImage == nullptr || theSrc.mWidth <= 0 || theSrc.mHeight <= 0)
        return;

    int aCornerWidth = theSrc.mWidth / 3;
    int aCornerHeight = theSrc.mHeight / 3;
    int aMidWidth = theSrc.mWidth - 2 * aCornerWidth;
    int aMidHeight = theSrc.mHeight - 2 * aCornerHeight;
    int aSrcX = theSrc.mX;
    int aSrcY = theSrc.mY;
    if (aCornerWidth <= 0 || aCornerHeight <= 0 || aMidWidth <= 0 || aMidHeight <= 0)
        return;

    DrawImage(theImage, theDest.mX, theDest.mY, TRect<int>(aSrcX, aSrcY, aCornerWidth, aCornerHeight));
    DrawImage(theImage, theDest.mX + theDest.mWidth - aCornerWidth, theDest.mY,
              TRect<int>(aSrcX + aCornerWidth + aMidWidth, aSrcY, aCornerWidth, aCornerHeight));
    DrawImage(theImage, theDest.mX, theDest.mY + theDest.mHeight - aCornerHeight,
              TRect<int>(aSrcX, aSrcY + aCornerHeight + aMidHeight, aCornerWidth, aCornerHeight));
    DrawImage(theImage, theDest.mX + theDest.mWidth - aCornerWidth, theDest.mY + theDest.mHeight - aCornerHeight,
              TRect<int>(aSrcX + aCornerWidth + aMidWidth, aSrcY + aCornerHeight + aMidHeight, aCornerWidth, aCornerHeight));

    Graphics aVertClip(*this);
    aVertClip.ClipRect(theDest.mX + aCornerWidth, theDest.mY, theDest.mWidth - 2 * aCornerWidth, theDest.mHeight);
    int aColumns = (aMidWidth + theDest.mWidth - 2 * aCornerWidth - 1) / aMidWidth;
    for (int aColumn = 0; aColumn < aColumns; ++aColumn)
    {
        int aX = theDest.mX + aCornerWidth + aColumn * aMidWidth;
        aVertClip.DrawImage(theImage, aX, theDest.mY, TRect<int>(aSrcX + aCornerWidth, aSrcY, aMidWidth, aCornerHeight));
        aVertClip.DrawImage(theImage, aX, theDest.mY + theDest.mHeight - aCornerHeight,
                            TRect<int>(aSrcX + aCornerWidth, aSrcY + aCornerHeight + aMidHeight, aMidWidth, aCornerHeight));
    }

    Graphics aHorzClip(*this);
    aHorzClip.ClipRect(theDest.mX, theDest.mY + aCornerHeight, theDest.mWidth, theDest.mHeight - 2 * aCornerHeight);
    int aRows = (theDest.mHeight - 2 * aCornerHeight + aMidHeight - 1) / aMidHeight;
    for (int aRow = 0; aRow < aRows; ++aRow)
    {
        int aY = theDest.mY + aCornerHeight + aRow * aMidHeight;
        aHorzClip.DrawImage(theImage, theDest.mX, aY, TRect<int>(aSrcX, aSrcY + aCornerHeight, aCornerWidth, aMidHeight));
        aHorzClip.DrawImage(theImage, theDest.mX + theDest.mWidth - aCornerWidth, aY,
                            TRect<int>(aSrcX + aCornerWidth + aMidWidth, aSrcY + aCornerHeight, aCornerWidth, aMidHeight));
    }

    Graphics aMidClip(*this);
    aMidClip.ClipRect(theDest.mX + aCornerWidth, theDest.mY + aCornerHeight,
                      theDest.mWidth - 2 * aCornerWidth, theDest.mHeight - 2 * aCornerHeight);
    for (int aColumn = 0; aColumn < aColumns; ++aColumn)
    {
        for (int aRow = 0; aRow < aRows; ++aRow)
        {
            aMidClip.DrawImage(theImage,
                               theDest.mX + aCornerWidth + aColumn * aMidWidth,
                               theDest.mY + aCornerHeight + aRow * aMidHeight,
                               TRect<int>(aSrcX + aCornerWidth, aSrcY + aCornerHeight, aMidWidth, aMidHeight));
        }
    }
}

void Sexy::Graphics::DrawImageBox(TRect<int> const& theDest, Sexy::Image * theImage)
{
    if (theImage == nullptr)
        return;
    DrawImageBox(TRect<int>(0, 0, theImage->mWidth, theImage->mHeight), theDest, theImage);
}

Sexy::Graphics* Sexy::Graphics::PushState()
{
    if (mStateStack.empty())
        mStateStack.reserve(8);
    GraphicsState aState;
    mStateStack.push_back(aState);
    mStateStack.back().CopyStateFrom(this);
    SetAsCurrentContext();
    return this;
}

Sexy::Graphics* Sexy::Graphics::PushState(bool theTrackDeviceState)
{
    if (mStateStack.empty())
        mStateStack.reserve(8);
    GraphicsState aState;
    mStateStack.push_back(aState);
    mStateStack.back().CopyStateFrom(this);
    SetAsCurrentContext();
    mTrackDeviceState = theTrackDeviceState;
    return this;
}

Sexy::Graphics* Sexy::Graphics::PopState()
{
    if (!mStateStack.empty())
    {
        CopyStateFrom(&mStateStack.back());
        mStateStack.pop_back();
    }
    SetAsCurrentContext();
    return this;
}

void Sexy::Graphics::PushColorMult()
{
    if (mColorStack.empty())
        mColorStack.push_back(mColor);
    else
        mColorStack.push_back(mFinalColor);
    CalcFinalColor();
}

void Sexy::Graphics::DrawTrianglesTex(Sexy::Image * theImage, SexyVertex2D const(*theTriangles)[3], int theCount)
{
    if (theImage == nullptr || mDestImage == nullptr || theTriangles == nullptr || theCount <= 0)
        return;
    if (ScaleIsIdentity())
    {
        mDestImage->BltTrianglesTex(theImage, theTriangles, theCount, mClipRect, GetImageColor(),
                                    mDrawMode, mTransX, mTransY, mLinearBlend);
        return;
    }

    SexyVertex2D (*aScaled)[3] = new SexyVertex2D[theCount][3];
    for (int aTriangle = 0; aTriangle < theCount; ++aTriangle)
    {
        for (int aVertex = 0; aVertex < 3; ++aVertex)
        {
            SexyVertex2D aPoint = theTriangles[aTriangle][aVertex];
            aPoint.mX = (float)(floor((aPoint.mX - mScaleOrigX) * mScaleX) + mScaleOrigX);
            aPoint.mY = (float)(floor((aPoint.mY - mScaleOrigY) * mScaleY) + mScaleOrigY);
            aScaled[aTriangle][aVertex] = aPoint;
        }
    }
    Color const& aColor = mColorizeImages ? mColor : Color::White;
    mDestImage->BltTrianglesTex(theImage, aScaled, theCount, mClipRect,
                                aColor, mDrawMode, mTransX, mTransY, mLinearBlend);
    delete[] aScaled;
}

int Sexy::Graphics::WriteString(Sexy::String const& theString, int theX, int theY, int theWidth, int theJustification, bool theDraw, int theOffset, int theLength, int theOldColor)
{
    if (mFont == nullptr)
        return 0;

    int aStart = theOffset;
    int anEnd = (theLength < 0) ? (int)theString.length() : theOffset + theLength;
    if (anEnd > (int)theString.length())
        anEnd = (int)theString.length();
    if (aStart < 0)
        aStart = 0;
    if (aStart > anEnd)
        return 0;

    int aColorWord = theOldColor;
    if (aColorWord == -1)
        aColorWord = (int)DrawColor().ToInt();

    if (theDraw && theJustification <= 1 && theWidth >= 0)
    {
        int aMeasured = WriteString(theString, theX, theY, theWidth, -1, false, theOffset, theLength, aColorWord);
        if (theJustification == 0)
            theX += (theWidth - aMeasured) / 2;
        else if (theJustification == 1)
            theX += theWidth - aMeasured;
    }

    Sexy::String aLine;
    int aXOffset = 0;
    while (aStart < anEnd)
    {
        char aChar = theString[(unsigned int)aStart];
        if (aChar != '^' || !mWriteColoredString)
        {
            aLine = aLine + Sexy::String(1, aChar);
            ++aStart;
            continue;
        }

        int aNext = aStart + 1;
        if (anEnd > aStart + 1 && theString[(unsigned int)aNext] == '^')
        {
            aLine = aLine + Sexy::String("^");
            aStart += 2;
            continue;
        }
        if (anEnd - 7 <= aStart)
            break;

        int aColor = 0;
        if (theString[(unsigned int)aNext] != 'o')
        {
            for (int aDigit = 0; aDigit < 6; ++aDigit)
                aColor = (aColor << 4) | HexDigit(theString[(unsigned int)(aStart + 1 + aDigit)]);
        }
        else if (strncmp(theString.c_str() + aNext, "oldclr", 6u) == 0)
        {
            aColor = aColorWord;
        }

        if (theDraw)
        {
            DrawString(aLine, theX + aXOffset, theY);
            SetColor(Color((unsigned char)(aColor >> 16), (unsigned char)(aColor >> 8),
                           (unsigned char)aColor, GetColor().mAlpha));
        }
        aXOffset += mFont->StringWidth(aLine);
        aLine = Sexy::String();
        aStart += 8;
    }

    if (theDraw)
        DrawString(aLine, theX + aXOffset, theY);
    return aXOffset + mFont->StringWidth(aLine);
}

int Sexy::Graphics::DrawStringColor(Sexy::String const& theString, int theX, int theY, int theOldColor)
{
    return WriteString(theString, theX, theY, -1, -1, true, 0, -1, theOldColor);
}

int Sexy::Graphics::WriteWordWrapped(TRect<int> const& theRect, Sexy::String const& theLine, int theLineSpacing, int theJustification, int *theMaxWidth, int theMaxChars, int *theLastWidth)
{
    if (mFont == nullptr)
        return 0;

    int aYOffset = 0;
    int aMaxWidth = 0;
    Color aSavedColor = GetColor();
    int aColorWord = (int)aSavedColor.ToInt();
    if ((aColorWord & 0xFF000000) == 0xFF000000)
        aColorWord &= 0xFFFFFF;

    int aLimit = theMaxChars;
    if (aLimit < 0)
        aLimit = (int)theLine.length();
    int aFontHeight = mFont->GetAscent() - mFont->GetDescent();
    int aSpacing = theLineSpacing;
    if (aSpacing == -1)
        aSpacing = mFont->GetLineSpacing();

    int aLength = (int)theLine.length();
    int aWidth = (theLastWidth != nullptr) ? *theLastWidth : 0;
    int aPos = 0;
    int aLineStart = 0;
    int aPrevChar = 0;
    int aLastWordEnd = -1;
    char aChar = 0;
    bool aStopped = false;

    while (aPos < aLength && !aStopped)
    {
        aChar = theLine[(unsigned int)aPos];
        char aNext = (aLength - 1 <= aPos) ? 0 : theLine[(unsigned int)(aPos + 1)];
        if (aChar == '^' && mWriteColoredString && aLength > aPos + 1)
        {
            if (aNext != '^')
            {
                aPos += 8;
                if (aLength <= aPos)
                    break;
                continue;
            }
            ++aPos;
        }
        else if (aChar == ' ')
        {
            aLastWordEnd = aPos;
        }
        else if (aChar == '\n')
        {
            aLastWordEnd = aPos++;
            aWidth = theRect.mWidth + 1;
        }

        aWidth += mFont->CharWidthKern(aChar, aPrevChar);
        if (aWidth <= theRect.mWidth)
        {
            ++aPos;
            aPrevChar = aChar;
            continue;
        }

        int aWritten = aWidth;
        if (aLastWordEnd == -1)
        {
            if (aPos <= aLineStart)
                ++aPos;
            int aCount = (aLimit >= aPos) ? aPos - aLineStart : aLimit - aLineStart;
            aWritten = 0;
            if (aCount > 0)
            {
                aWritten = WriteString(theLine, theRect.mX, theRect.mY + aYOffset, theRect.mWidth,
                                       theJustification, true, aLineStart, aCount, aColorWord);
                if (aWritten < 0)
                {
                    aStopped = true;
                    break;
                }
            }
            if (theMaxWidth != nullptr && *theMaxWidth < aWritten)
                *theMaxWidth = aWritten;
            aLineStart = aPos;
            if (theLastWidth != nullptr)
                *theLastWidth = aWritten;
        }
        else
        {
            int aClipTop = mClipRect.mY;
            int aClipY = (int)(float)((float)(theRect.mY + aYOffset) + mTransY);
            if (aClipY >= aClipTop && aClipY < aClipTop + mClipRect.mHeight + aSpacing)
            {
                int aBreak = (aLimit < aLastWordEnd) ? aLimit : aLastWordEnd;
                if (aBreak - aLineStart > 0)
                    WriteString(theLine, theRect.mX, theRect.mY + aYOffset, theRect.mWidth, theJustification, true,
                                aLineStart, aBreak - aLineStart, aColorWord);
            }
            aWritten = aWidth;
            if (aWritten < 0)
            {
                aStopped = true;
                break;
            }
            aLineStart = aLastWordEnd + 1;
            if (aLimit < aLineStart)
            {
                aStopped = true;
                break;
            }
            if (aChar == ' ')
            {
                while (aLength != aLineStart && theLine[(unsigned int)aLineStart] == ' ')
                    ++aLineStart;
            }
        }

        if (aMaxWidth < aWritten)
            aMaxWidth = aWritten;
        int aBottom = aYOffset + aSpacing;
        aYOffset += aSpacing;
        if (theRect.mHeight != 0 && theRect.mHeight < aBottom + aSpacing - aFontHeight)
        {
            aStopped = true;
            break;
        }
        aPrevChar = 0;
        aWidth = 0;
        aPos = aLineStart;
        aLastWordEnd = -1;
    }

    if (!aStopped && (theRect.mHeight == 0 || aSpacing + aYOffset - aFontHeight <= theRect.mHeight))
    {
        if (aPos >= aLength)
        {
            if (aChar == '\n')
            {
                aYOffset += aSpacing;
                if (theLastWidth != nullptr)
                    *theLastWidth = 0;
            }
        }
        else
        {
            int aRemaining = (int)theLine.length() - aPos;
            if ((int)theLine.length() > aLimit)
                aRemaining = aLimit - aPos;
            int aWritten = 0;
            if (aRemaining > 0)
            {
                aWritten = WriteString(theLine, theRect.mX, theRect.mY + aYOffset, theRect.mWidth, theJustification, true,
                                       aPos, aRemaining, aColorWord);
            }
            if (aWritten >= 0)
            {
                if (aMaxWidth < aWritten)
                    aMaxWidth = aWritten;
                if (theMaxWidth != nullptr && *theMaxWidth < aWritten)
                    *theMaxWidth = aWritten;
                if (theLastWidth != nullptr)
                    *theLastWidth = aWritten;
                aYOffset += aSpacing;
            }
        }
    }

    SetColor(aSavedColor);
    if (theMaxWidth != nullptr)
        *theMaxWidth = aMaxWidth;
    return mFont->GetHeight() + aYOffset - aSpacing;
}

int Sexy::Graphics::DrawStringWordWrapped(Sexy::String const& theLine, int theX, int theY, int theWrapWidth, int theLineSpacing, int theJustification, int *theMaxWidth)
{
    if (mFont == nullptr)
        return 0;
    int anAscent = mFont->GetAscent();
    int aDescent = mFont->GetDescent();
    TRect<int> aRect(theX, theY - (anAscent - aDescent), theWrapWidth, 0);
    return WriteWordWrapped(aRect, theLine, theLineSpacing, theJustification, theMaxWidth, -1, nullptr);
}

int Sexy::Graphics::GetWordWrappedHeight(int theWidth, Sexy::String const& theLine, int theLineSpacing, int *theMaxWidth)
{
    Graphics aMeasure(nullptr);
    aMeasure.SetFont(mFont);
    return aMeasure.WriteWordWrapped(TRect<int>(0, 0, theWidth, 0), theLine, theLineSpacing, -1, theMaxWidth, -1, nullptr);
}

Sexy::Graphics::~Graphics()
{
    delete mGraphics3D;
    mGraphics3D = nullptr;
}

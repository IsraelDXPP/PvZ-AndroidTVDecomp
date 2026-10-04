// ImageFont. Measurement walks the active layers. Drawing buckets glyphs
// by order and issues DrawImage. The cache stream is a moving byte cursor:
// each read copies the requested count and advances.

#include "ImageFont.h"
#include "ActiveFontLayer.h"
#include "Buffer.h"
#include "CharData.h"
#include "CritSect.h"
#include "DeviceImage.h"
#include "Font.h"
#include "FontData.h"
#include "FontLayer.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "NativeDisplay.h"
#include "ReconData.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"
#include "ResourceManager.h"
#include "SexyAppBase.h"
#include "SharedImageRef.h"

#include <cstring>
#include <list>
#include <map>
#include <string>
#include <vector>

extern Sexy::CritSect gRenderCritSec;

namespace {

const Sexy::String& AsString(Sexy::WString const& theWide)
{
    const void* aWide = &theWide;
    return *static_cast<const Sexy::String*>(aWide);
}

const Sexy::WString& AsWide(Sexy::String const& theText)
{
    const void* aText = &theText;
    return *static_cast<const Sexy::WString*>(aText);
}

const int* WideChars(Sexy::String const& theWide)
{
    const void* aChars = theWide.c_str();
    return static_cast<const int*>(aChars);
}

void ReadBytes(const unsigned char*& theCursor, void* theDest, int theCount)
{
    if (theCount <= 0)
        return;
    std::memcpy(theDest, theCursor, (size_t)theCount);
    theCursor += theCount;
}

void WriteBytes(unsigned char*& theCursor, const void* theSrc, int theCount)
{
    if (theCount <= 0 || theCursor == nullptr)
        return;
    std::memcpy(theCursor, theSrc, (size_t)theCount);
    theCursor += theCount;
}

void WriteString(unsigned char*& theCursor, Sexy::String const& theText)
{
    int aLength = (int)theText.length();
    WriteBytes(theCursor, &aLength, 4);
    WriteBytes(theCursor, theText.c_str(), aLength);
}

int StringBytes(Sexy::String const& theText)
{
    return (int)theText.length() + 4;
}

Sexy::String ReadString(const unsigned char*& theCursor, bool theSwap)
{
    int aLength = 0;
    ReadBytes(theCursor, &aLength, 4);
    if (theSwap)
        aLength = EndianInt(aLength);
    if (aLength < 0)
        aLength = 0;
    Sexy::String aText;
    if (aLength == 0)
        return aText;
    aText.reserve((unsigned int)aLength);
    const void* aTextBytes = theCursor;
    aText.Assign(static_cast<const char*>(aTextBytes), (unsigned int)aLength);
    theCursor += aLength;
    return aText;
}

static int MulDiv255(int aLeft, int aRight)
{
    int aProduct = (int)((unsigned int)aLeft * (unsigned int)aRight);
    long long aWide = (long long)aProduct * (int)0x80808081;
    int aHi = (int)(aWide >> 32);
    int aSum = (int)((unsigned int)aHi + (unsigned int)aProduct);
    return (aSum >> 7) - (aProduct >> 31);
}

static int ClampChannel(int aValue)
{
    return aValue >= 255 ? 255 : aValue;
}

int SignedHalf(unsigned short aHalf)
{
    return (int)(short)aHalf;
}

bool NameEndsWithMod(Sexy::String const& theName)
{
    return theName.length() > 5
        && theName.compare(theName.length() - 5, 5, "__MOD") == 0;
}

bool VectorHas(std::vector<Sexy::String> const& theTags, Sexy::String const& theNeedle)
{
    for (unsigned int i = 0; i < theTags.size(); ++i)
    {
        if (theTags[i] == theNeedle)
            return true;
    }
    return false;
}

Sexy::TRect<int> SourceRect(Sexy::ActiveFontLayer* theLayer, int theChar)
{
    auto found = theLayer->mCharRects.find(theChar);
    if (found == theLayer->mCharRects.end())
        found = theLayer->mCharRects.emplace(theChar, Sexy::TRect<int>()).first;
    return found->second;
}

struct DrawGlyphRecord
{
    Sexy::Image* mImage;
    int mX;
    int mY;
    int mSrcX;
    int mSrcY;
    int mSrcWidth;
    int mSrcHeight;
    int mDrawMode;
    Sexy::Color mColor;
    DrawGlyphRecord* mNext;
};

DrawGlyphRecord* sDrawHeads[256];
DrawGlyphRecord* sDrawTails[256];
DrawGlyphRecord sDrawRecords[0x1000];

struct Extents
{
    float mMinX;
    float mMinY;
    float mLastSpanX;
    float mLastSpanY;
    float mWidth;
    float mHeight;
};

struct GlyphListNode
{
    GlyphListNode* mNext;
    GlyphListNode* mPrev;
    Sexy::Glyph const* mGlyph;
};

void ApplyFormatHint(Sexy::FontLayer* theLayer, Sexy::Image* theImage, int theHint, bool theIsNew)
{
    if (!theIsNew || theImage == nullptr)
        return;
    theImage->Palletize();
    if (theHint == 0 || theImage->GetPixelFormatHint() != 0)
        return;
    Sexy::NativeDisplay* aDisplay = theLayer->mFontData->mApp != nullptr
        ? theLayer->mFontData->mApp->mPlatformDriverOrQueue
        : nullptr;
    if (aDisplay != nullptr && aDisplay->IsPixelFormatSupported(Sexy::PixelFormat(theHint)))
    {
        theImage->SetPixelFormatHint(Sexy::PixelFormat(theHint));
        theImage->Finalize();
    }
}

void LoadLayerImage(Sexy::FontData* theFont, Sexy::FontLayer* theLayer, int theHint, int& theFailed)
{
    bool aIsNew = false;
    bool aGotImage = false;
    Sexy::SharedImageRef aLoaded;
    Sexy::SexyAppBase* aApp = gSexyAppBase;
    if (aApp->mResourceManager != nullptr && theFont->mImageGroup.empty())
    {
        Sexy::String aId = aApp->mResourceManager->GetIdByPath(theLayer->mImageFileName);
        if (!aId.empty())
        {
            aLoaded = aApp->mResourceManager->GetImage(aId);
            Sexy::DeviceImage* aDevice = aLoaded;
            if (aDevice == nullptr)
                aLoaded = aApp->mResourceManager->LoadImage(aId);
            aDevice = aLoaded;
            aGotImage = aDevice != nullptr;
        }
    }
    if (!aGotImage)
    {
        Sexy::String aFile = theFont->mImageGroup;
        aFile += theLayer->mImageFileName;
        Sexy::String aVariant;
        aLoaded = aApp->GetSharedImage(aFile, aVariant, &aIsNew, true, false);
    }
    theLayer->mImage = aLoaded;
    Sexy::Image* aImage = theLayer->mImage;
    if (aImage == nullptr)
        theFailed = 1;
    else
        ApplyFormatHint(theLayer, aImage, theHint, aIsNew);
}

int MeasureLayerSize(Sexy::FontData* theFont)
{
    int aSize = 8 * (int)theFont->mCharMap.size() + 37;
    for (auto aLayer = theFont->mLayerList.begin(); aLayer != theFont->mLayerList.end(); ++aLayer)
    {
        int aLayerBytes = aSize + StringBytes(aLayer->mLayerName) + 4;
        for (unsigned int i = 0; i < aLayer->mRequiredTags.size(); ++i)
            aLayerBytes += StringBytes(aLayer->mRequiredTags[i]);
        aLayerBytes += 4;
        for (unsigned int i = 0; i < aLayer->mExcludedTags.size(); ++i)
            aLayerBytes += StringBytes(aLayer->mExcludedTags[i]);
        int aKernSpan = (int)aLayer->mKerningValues.size() * 8;
        int aCharBytes = aLayerBytes + (aKernSpan & ~7) + 8;
        aCharBytes += 24 * (int)aLayer->mCharDataMap.size();
        aSize = aCharBytes + StringBytes(aLayer->mImageFileName) + 88;
    }
    return aSize;
}

void WriteCharData(unsigned char*& theCursor, int theChar, Sexy::CharData const& theData)
{
    WriteBytes(theCursor, &theChar, 4);
    WriteBytes(theCursor, &theData.mPackedOrigin, 4);
    WriteBytes(theCursor, &theData.mPackedSize, 4);
    WriteBytes(theCursor, &theData.mPackedOffset, 4);
    WriteBytes(theCursor, &theData.mKernIndex, 2);
    WriteBytes(theCursor, &theData.mKernCount, 2);
    WriteBytes(theCursor, &theData.mAdvance, 2);
    WriteBytes(theCursor, &theData.mPageHeight, 2);
}

void ReadCharData(const unsigned char*& theCursor, Sexy::CharData* theData, bool theSwap)
{
    ReadBytes(theCursor, &theData->mPackedOrigin, 4);
    ReadBytes(theCursor, &theData->mPackedSize, 4);
    if (theSwap)
    {
        theData->mX = (unsigned short)EndianShort((short)theData->mX);
        theData->mY = (unsigned short)EndianShort((short)theData->mY);
        theData->mWidth = (unsigned short)EndianShort((short)theData->mWidth);
        theData->mHeight = (unsigned short)EndianShort((short)theData->mHeight);
    }
    ReadBytes(theCursor, &theData->mPackedOffset, 4);
    if (theSwap)
    {
        theData->mOffsetX = (unsigned short)EndianShort((short)theData->mOffsetX);
        theData->mOffsetY = (unsigned short)EndianShort((short)theData->mOffsetY);
    }
    ReadBytes(theCursor, &theData->mKernIndex, 2);
    if (theSwap)
        theData->mKernIndex = (unsigned short)EndianUINT16(theData->mKernIndex);
    ReadBytes(theCursor, &theData->mKernCount, 2);
    if (theSwap)
        theData->mKernCount = (unsigned short)EndianUINT16(theData->mKernCount);
    ReadBytes(theCursor, &theData->mAdvance, 2);
    if (theSwap)
        theData->mAdvance = (unsigned short)EndianShort((short)theData->mAdvance);
    ReadBytes(theCursor, &theData->mPageHeight, 2);
    if (theSwap)
        theData->mPageHeight = (unsigned short)EndianShort((short)theData->mPageHeight);
}

int ReadFont(Sexy::ImageFont* theFont, const unsigned char* theCursor, bool theSwap)
{
    int aMagic = 0;
    ReadBytes(theCursor, &aMagic, 4);
    if (theSwap)
        aMagic = EndianInt(aMagic);
    if (aMagic != kFontFileMagic)
        return 0;
    ReadBytes(theCursor, &aMagic, 4);
    if (theSwap)
        aMagic = EndianInt(aMagic);
    if (aMagic != kFontFileVersion)
        return 0;

    ReadBytes(theCursor, &theFont->mAscent, 4);
    ReadBytes(theCursor, &theFont->mAscentPadding, 4);
    ReadBytes(theCursor, &theFont->mHeight, 4);
    ReadBytes(theCursor, &theFont->mLineSpacingOffset, 4);
    if (theSwap)
    {
        theFont->mAscent = EndianInt(theFont->mAscent);
        theFont->mAscentPadding = EndianInt(theFont->mAscentPadding);
        theFont->mHeight = EndianInt(theFont->mHeight);
        theFont->mLineSpacingOffset = EndianInt(theFont->mLineSpacingOffset);
    }

    theFont->mFontData->mApp = gSexyAppBase;
    ReadBytes(theCursor, &theFont->mFontData->mIsListFont, 1);
    ReadBytes(theCursor, &theFont->mFontData->mDefaultPointSize, 4);
    if (theSwap)
        theFont->mFontData->mDefaultPointSize = EndianInt(theFont->mFontData->mDefaultPointSize);

    int aCharMapCount = 0;
    ReadBytes(theCursor, &aCharMapCount, 4);
    aCharMapCount = EndianInt(aCharMapCount);
    for (int aIndex = 0; aIndex < aCharMapCount; ++aIndex)
    {
        int aFrom = 0;
        int aTo = 0;
        ReadBytes(theCursor, &aFrom, 4);
        ReadBytes(theCursor, &aTo, 4);
        aFrom = (int)EndianUINT32((unsigned int)aFrom);
        aTo = (int)EndianUINT32((unsigned int)aTo);
        theFont->mFontData->mCharMap[aFrom] = aTo;
    }

    int aLayerCount = 0;
    ReadBytes(theCursor, &aLayerCount, 4);
    if (theSwap)
        aLayerCount = EndianInt(aLayerCount);
    int aFailed = 0;
    for (int aLayerIndex = 0; aLayerIndex < aLayerCount; ++aLayerIndex)
    {
        theFont->mFontData->mLayerList.push_back(Sexy::FontLayer(theFont->mFontData));
        Sexy::FontLayer* aFontLayer = &theFont->mFontData->mLayerList.back();

        aFontLayer->mLayerName = ReadString(theCursor, theSwap);
        Sexy::String aWideName;
        ToWString(&aWideName, &aFontLayer->mLayerName);
        aFontLayer->mLayerName = aWideName;
        theFont->mFontData->mLayerMap[aFontLayer->mLayerName] = aFontLayer;

        int aTagCount = 0;
        ReadBytes(theCursor, &aTagCount, 4);
        if (theSwap)
            aTagCount = EndianInt(aTagCount);
        for (int aTag = 0; aTag < aTagCount; ++aTag)
        {
            Sexy::String aNarrow = ReadString(theCursor, theSwap);
            Sexy::String aWide;
            ToWString(&aWide, &aNarrow);
            aFontLayer->mRequiredTags.push_back(aWide);
        }
        ReadBytes(theCursor, &aTagCount, 4);
        if (theSwap)
            aTagCount = EndianInt(aTagCount);
        for (int aTag = 0; aTag < aTagCount; ++aTag)
        {
            Sexy::String aNarrow = ReadString(theCursor, theSwap);
            Sexy::String aWide;
            ToWString(&aWide, &aNarrow);
            aFontLayer->mExcludedTags.push_back(aWide);
        }

        int aKerningCount = 0;
        ReadBytes(theCursor, &aKerningCount, 4);
        if (theSwap)
            aKerningCount = EndianInt(aKerningCount);
        if (aKerningCount > 0)
        {
            aFontLayer->mKerningValues.resize((unsigned int)aKerningCount);
            ReadBytes(theCursor, aFontLayer->mKerningValues.data(), 8 * aKerningCount);
            if (theSwap)
            {
                for (int aKern = 0; aKern < aKerningCount; ++aKern)
                {
                    aFontLayer->mKerningValues[aKern].mSecondChar =
                        EndianInt(aFontLayer->mKerningValues[aKern].mSecondChar);
                    aFontLayer->mKerningValues[aKern].mAmount =
                        EndianInt(aFontLayer->mKerningValues[aKern].mAmount);
                }
            }
        }

        int aCharCount = 0;
        ReadBytes(theCursor, &aCharCount, 4);
        if (theSwap)
            aCharCount = EndianInt(aCharCount);
        for (int aChar = 0; aChar < aCharCount; ++aChar)
        {
            int aKey = 0;
            ReadBytes(theCursor, &aKey, 4);
            if (theSwap)
                aKey = (int)EndianUINT32((unsigned int)aKey);
            Sexy::CharData* aData = aFontLayer->SetCharData(aKey);
            ReadCharData(theCursor, aData, theSwap);
        }

        ReadBytes(theCursor, &aFontLayer->mColorMult, 16);
        ReadBytes(theCursor, &aFontLayer->mColorAdd, 16);
        if (theSwap)
        {
            aFontLayer->mColorMult.mRed = EndianInt(aFontLayer->mColorMult.mRed);
            aFontLayer->mColorMult.mGreen = EndianInt(aFontLayer->mColorMult.mGreen);
            aFontLayer->mColorMult.mBlue = EndianInt(aFontLayer->mColorMult.mBlue);
            aFontLayer->mColorMult.mAlpha = EndianInt(aFontLayer->mColorMult.mAlpha);
            aFontLayer->mColorAdd.mRed = EndianInt(aFontLayer->mColorAdd.mRed);
            aFontLayer->mColorAdd.mGreen = EndianInt(aFontLayer->mColorAdd.mGreen);
            aFontLayer->mColorAdd.mBlue = EndianInt(aFontLayer->mColorAdd.mBlue);
            aFontLayer->mColorAdd.mAlpha = EndianInt(aFontLayer->mColorAdd.mAlpha);
        }
        aFontLayer->mImageFileName = ReadString(theCursor, theSwap);

        int aFormatHint = 0;
        ReadBytes(theCursor, &aFormatHint, 4);
        if (theSwap)
            (void)EndianInt(aFormatHint);
        aFontLayer->mUseAlphaCorrection = aFormatHint;
        LoadLayerImage(theFont->mFontData, aFontLayer, aFormatHint, aFailed);

        ReadBytes(theCursor, &aFontLayer->mDrawMode, 4);
        ReadBytes(theCursor, &aFontLayer->mOffset, 8);
        ReadBytes(theCursor, &aFontLayer->mSpacing, 4);
        ReadBytes(theCursor, &aFontLayer->mMinPointSize, 4);
        ReadBytes(theCursor, &aFontLayer->mMaxPointSize, 4);
        ReadBytes(theCursor, &aFontLayer->mPointSize, 4);
        ReadBytes(theCursor, &aFontLayer->mAscent, 4);
        ReadBytes(theCursor, &aFontLayer->mAscentPadding, 4);
        ReadBytes(theCursor, &aFontLayer->mHeight, 4);
        ReadBytes(theCursor, &aFontLayer->mDefaultHeight, 4);
        ReadBytes(theCursor, &aFontLayer->mLineSpacingOffset, 4);
        ReadBytes(theCursor, &aFontLayer->mBaseOrder, 4);
        if (theSwap)
        {
            aFontLayer->mDrawMode = EndianInt(aFontLayer->mDrawMode);
            aFontLayer->mOffset.mX = EndianInt(aFontLayer->mOffset.mX);
            aFontLayer->mOffset.mY = EndianInt(aFontLayer->mOffset.mY);
            aFontLayer->mSpacing = EndianInt(aFontLayer->mSpacing);
            aFontLayer->mMinPointSize = EndianInt(aFontLayer->mMinPointSize);
            aFontLayer->mMaxPointSize = EndianInt(aFontLayer->mMaxPointSize);
            aFontLayer->mPointSize = EndianInt(aFontLayer->mPointSize);
            aFontLayer->mAscent = EndianInt(aFontLayer->mAscent);
            aFontLayer->mAscentPadding = EndianInt(aFontLayer->mAscentPadding);
            aFontLayer->mHeight = EndianInt(aFontLayer->mHeight);
            aFontLayer->mDefaultHeight = EndianInt(aFontLayer->mDefaultHeight);
            aFontLayer->mLineSpacingOffset = EndianInt(aFontLayer->mLineSpacingOffset);
            aFontLayer->mBaseOrder = EndianInt(aFontLayer->mBaseOrder);
        }
    }

    theFont->mFontData->mFileName = ReadString(theCursor, theSwap);
    Sexy::String aErrorNarrow = ReadString(theCursor, theSwap);
    Sexy::String aErrorHeader;
    ToSexyString(&aErrorHeader, &aErrorNarrow);
    theFont->mFontData->mErrorHeader = aErrorHeader;

    ReadBytes(theCursor, &theFont->mPointSize, 4);
    if (theSwap)
        theFont->mPointSize = EndianInt(theFont->mPointSize);
    int aTagCount = 0;
    ReadBytes(theCursor, &aTagCount, 4);
    if (theSwap)
        aTagCount = EndianInt(aTagCount);
    for (int aTag = 0; aTag < aTagCount; ++aTag)
    {
        Sexy::String aNarrow = ReadString(theCursor, theSwap);
        Sexy::String aWide;
        ToWString(&aWide, &aNarrow);
        theFont->mTags.push_back(aWide);
    }
    ReadBytes(theCursor, &theFont->mScale, 8);
    if (theSwap)
        theFont->mScale = EndianDouble(theFont->mScale);
    ReadBytes(theCursor, &theFont->mForceScaledImagesWhite, 1);
    theFont->mRecalcFontSize = false;
    return aFailed ^ 1;
}

} // namespace

int Sexy::ImageFont::CharWidth(int theChar)
{
    return CharWidthKern(theChar, 0);
}

void Sexy::ImageFont::DrawString(Sexy::Graphics* theGraphics, int theX, int theY,
                                 Sexy::String const& theString, Color const& theColor,
                                 TRect<int> const& theClipRect)
{
    DrawStringEx(theGraphics, theX, theY, theString, theColor, &theClipRect, nullptr, nullptr);
}

void Sexy::ImageFont::DrawString(Sexy::Graphics* theGraphics, int theX, int theY,
                                 Sexy::WString const& theString, Color const& theColor,
                                 TRect<int> const& theClipRect)
{
    DrawStringEx(theGraphics, theX, theY, theString, theColor, &theClipRect, nullptr, nullptr);
}

int Sexy::ImageFont::SetPointSize(int thePointSize)
{
    mPointSize = thePointSize;
    mRecalcFontSize = false;
    return mPointSize;
}

int Sexy::ImageFont::SetScale(double theScale)
{
    mScale = theScale;
    mRecalcFontSize = false;
    return 1;
}

int Sexy::ImageFont::GetPointSize()
{
    return mPointSize;
}

int Sexy::ImageFont::GetDefaultPointSize()
{
    return mFontData->mDefaultPointSize;
}

bool Sexy::ImageFont::IsComposited()
{
    Prepare();
    if (mActiveLayerList.empty())
        return false;
    if (mActiveLayerList.size() <= 1)
        return mActiveLayerList[0].mLayer->mColorAdd.ToInt() != 0;
    return true;
}

Sexy::ImageFont* Sexy::ImageFont::Prepare()
{
    if (mRecalcFontSize)
        return this;
    GenerateActiveFontLayers();
    mRecalcFontSize = true;
    return this;
}

int Sexy::ImageFont::StringWidth(Sexy::String const& theString)
{
    Sexy::String aWide;
    StringToWString(&aWide, theString.c_str());
    if (aWide.empty())
    {
        int aWidth = 0;
        int aPrevChar = 0;
        for (unsigned int i = 0; i < theString.length(); ++i)
        {
            int aChar = (unsigned char)theString.c_str()[i];
            aWidth += CharWidthKern(aChar, aPrevChar);
            aPrevChar = aChar;
        }
        return aWidth;
    }
    return StringWidth(AsWide(aWide));
}

int Sexy::ImageFont::AddTag(Sexy::String const& theTag)
{
    Sexy::String aWide;
    StringToWString(&aWide, theTag.c_str());
    return AddTag(AsWide(aWide));
}

int Sexy::ImageFont::RemoveTag(Sexy::String const& theTag)
{
    Sexy::String aWide;
    StringToWString(&aWide, theTag.c_str());
    return RemoveTag(AsWide(aWide));
}

int Sexy::ImageFont::HasTag(Sexy::String const& theTag)
{
    Sexy::String aWide;
    StringToWString(&aWide, theTag.c_str());
    return HasTag(AsWide(aWide));
}

int Sexy::ImageFont::AddTag(Sexy::WString const& theTag)
{
    if (HasTag(theTag) != 0)
        return 0;
    Sexy::String aUpper;
    StringToUpper(&aUpper, AsString(theTag).c_str());
    mTags.push_back(aUpper);
    mRecalcFontSize = false;
    return 1;
}

int Sexy::ImageFont::RemoveTag(Sexy::WString const& theTag)
{
    Sexy::String aUpper;
    StringToUpper(&aUpper, AsString(theTag).c_str());
    for (unsigned int i = 0; i < mTags.size(); ++i)
    {
        if (mTags[i] == aUpper)
        {
            mTags.erase(mTags.begin() + (int)i);
            mRecalcFontSize = false;
            return 1;
        }
    }
    return 0;
}

int Sexy::ImageFont::HasTag(Sexy::WString const& theTag)
{
    const Sexy::String& aTag = AsString(theTag);
    for (unsigned int i = 0; i < mTags.size(); ++i)
    {
        if (mTags[i] == aTag)
            return 1;
    }
    return 0;
}

int Sexy::ImageFont::GetDefine(Sexy::WString const& theName)
{
    Sexy::DataElement* aElement = mFontData->Dereference(AsString(theName));
    if (aElement == nullptr)
        return 0;
    Sexy::String aText = mFontData->DataElementToString(aElement, true);
    return (int)aText.ToInt();
}

int Sexy::ImageFont::StringWidth(Sexy::WString const& theString)
{
    const Sexy::String& aWide = AsString(theString);
    if (aWide.empty())
        return 0;
    const int* aChars = WideChars(aWide);
    int aWidth = 0;
    int aPrevMapped = 0;
    for (unsigned int i = 0; i < aWide.size(); ++i)
    {
        int aMapped = GetMappedChar(aChars[i]);
        aWidth += MappedCharWidthKern(aMapped, aPrevMapped);
        aPrevMapped = aMapped;
    }
    return aWidth;
}

void Sexy::ImageFont::StringToGlyphs(Sexy::WString const& theString, std::vector<Glyph>& theGlyphs)
{
    const Sexy::String& aWide = AsString(theString);
    if (aWide.empty())
        return;
    const int* aChars = WideChars(aWide);
    int aLayerCount = (int)mActiveLayerList.size();
    float aPenX = 0.0f;
    int aPrevMapped = 0;

    for (unsigned int i = 0; i < aWide.size(); ++i)
    {
        int aMapped = GetMappedChar(aChars[i]);
        Glyph aGlyph = {};
        aGlyph.mChar = aMapped;

        int aMaxWidth = 0;
        int aGlyphWidth = 0;
        int aLayerWidth = 0;
        if (aLayerCount != 0)
        {
            double aScale = mScale;
            double aScaledPointSize = (double)mPointSize * aScale;
            for (int aLayer = 0; aLayer < aLayerCount; ++aLayer)
            {
                FontLayer* aFontLayer = mActiveLayerList[(unsigned int)aLayer].mLayer;
                CharData* aCharData = aFontLayer->GetCharData(aMapped);
                int aLayerPointSize = aFontLayer->mPointSize;
                int aWidth = aLayerPointSize != 0
                    ? (int)(aScaledPointSize * (double)(short)aCharData->mAdvance / (double)aLayerPointSize)
                    : (int)((double)(short)aCharData->mAdvance * aScale);

                int aKern = 0;
                if (aPrevMapped != 0)
                {
                    aKern = aFontLayer->mSpacing;
                    if (aCharData->mKernCount != 0)
                    {
                        FontLayer::KerningValue* aPair =
                            aFontLayer->mKerningValues.data() + aCharData->mKernIndex;
                        for (unsigned int aPairIndex = 0; aPairIndex < aCharData->mKernCount; ++aPairIndex)
                        {
                            if (aMapped == aPair->mSecondChar)
                            {
                                aKern += aLayerPointSize != 0
                                    ? (int)(aScaledPointSize * (double)aPair->mAmount / (double)aLayerPointSize)
                                    : (int)((double)aPair->mAmount * aScale);
                                break;
                            }
                            ++aPair;
                        }
                    }
                }

                int aTotal = aKern + aWidth;
                if (aMaxWidth < aTotal)
                    aMaxWidth = aTotal;
                if (aLayer == 0)
                    aGlyphWidth = aTotal;
                if (aLayer <= 3)
                    (&aGlyph.mWidth0)[aLayer] = aTotal;
                if (aLayer == 0)
                    aGlyph.mCharData = aCharData;
                else if (aLayer == 1)
                    aGlyph.mCharData1 = aCharData;
                else if (aLayer == 2)
                    aGlyph.mCharData2 = aCharData;
                else if (aLayer == 3)
                    aGlyph.mCharData3 = aCharData;
                aLayerWidth = aMaxWidth;
            }
        }

        aGlyph.mX = aPenX;
        aGlyph.mWidth = aGlyphWidth;
        aGlyph.mMaxWidth = aMaxWidth;
        aGlyph.mY = 0.0f;
        theGlyphs.push_back(aGlyph);
        aPenX = aPenX + (float)aLayerWidth;
        aPrevMapped = aMapped;
    }
}

void Sexy::ImageFont::DrawGlyphs(Sexy::Graphics* theGraphics, int theX, int theY,
                                 std::vector<Glyph>& theGlyphs, unsigned int theStart,
                                 unsigned int theCount, Color const& theColor,
                                 TRect<int> const& theClipRect)
{
    (void)theClipRect;
    if (theGlyphs.empty() || theStart >= theGlyphs.size())
        return;
    unsigned int anEnd = theStart + theCount;
    if (anEnd > theGlyphs.size())
        anEnd = (unsigned int)theGlyphs.size();
    int aLayerCount = (int)mActiveLayerList.size();
    if (aLayerCount == 0)
        return;

    struct GlyphSprite
    {
        int mOrder;
        int mX;
        int mY;
        int mSrcX;
        int mSrcY;
        int mWidth;
        int mHeight;
        int mDrawMode;
        Color mColor;
        Sexy::Image* mImage;
        GlyphSprite* mNext;
    };

    gRenderCritSec.Lock();
    bool aSavedColorize = theGraphics->GetColorizeImages();
    theGraphics->SetColorizeImages(true);

    std::vector<Color> aLayerColors((unsigned int)aLayerCount);
    for (int aLayer = 0; aLayer < aLayerCount; ++aLayer)
    {
        FontLayer* aFontLayer = mActiveLayerList[(unsigned int)aLayer].mLayer;
        Color aColor;
        aColor.mAlpha = theColor.mAlpha + aFontLayer->mColorAdd.mAlpha * theColor.mAlpha / 255;
        aColor.mRed = theColor.mRed + aFontLayer->mColorAdd.mRed * theColor.mRed / 255;
        aColor.mGreen = theColor.mGreen + aFontLayer->mColorAdd.mGreen * theColor.mGreen / 255;
        aColor.mBlue = theColor.mBlue + aFontLayer->mColorAdd.mBlue * theColor.mBlue / 255;
        if (aColor.mAlpha >= 255) aColor.mAlpha = 255;
        if (aColor.mRed >= 255) aColor.mRed = 255;
        if (aColor.mGreen >= 255) aColor.mGreen = 255;
        if (aColor.mBlue >= 255) aColor.mBlue = 255;
        aLayerColors[(unsigned int)aLayer] = aColor;
    }

    GlyphSprite* aBuckets[256];
    GlyphSprite* aBucketTails[256];
    for (int i = 0; i < 256; ++i)
    {
        aBuckets[i] = nullptr;
        aBucketTails[i] = nullptr;
    }
    std::vector<GlyphSprite> aSprites;
    aSprites.reserve(256);

    for (unsigned int aGlyphIndex = theStart; aGlyphIndex < anEnd; ++aGlyphIndex)
    {
        Glyph& aGlyph = theGlyphs[aGlyphIndex];
        for (int aLayer = 0; aLayer < aLayerCount; ++aLayer)
        {
            FontLayer* aFontLayer = mActiveLayerList[(unsigned int)aLayer].mLayer;
            CharData* aCharData = nullptr;
            if (aLayer == 0)
                aCharData = static_cast<CharData*>(aGlyph.mCharData);
            else
                aCharData = aFontLayer->GetCharData(aGlyph.mChar);
            if (aCharData == nullptr)
                continue;

            double aScale = mScale;
            if (aFontLayer->mPointSize != 0)
                aScale = aScale * ((double)mPointSize / (double)aFontLayer->mPointSize);

            short aOffsetX = (short)aCharData->mOffsetX;
            short aOffsetY = (short)aCharData->mOffsetY;
            short aSrcWidth = (short)aCharData->mKernIndex;
            short aSrcHeight = (short)aCharData->mKernCount;
            short aOrderHalf = (short)aCharData->mPageHeight;
            int aSpriteX;
            int aSpriteY;
            if (aScale == 1.0)
            {
                aSpriteX = theX + (int)aGlyph.mX + aOffsetX;
                aSpriteY = theY + (int)aGlyph.mY + aOffsetY;
            }
            else
            {
                aSpriteX = (int)(aScale * (double)aOffsetX) + theX + (int)aGlyph.mX;
                aSpriteY = theY + (int)aGlyph.mY - (int)(aScale * (double)aOffsetY);
            }

            int anOrder = aOrderHalf + aFontLayer->mBaseOrder;
            if (anOrder < 0)
                anOrder = 0;
            if (anOrder >= 255)
                anOrder = 255;

            GlyphSprite aSprite;
            aSprite.mOrder = anOrder;
            aSprite.mX = aSpriteX;
            aSprite.mY = aSpriteY;
            aSprite.mSrcX = aOffsetX;
            aSprite.mSrcY = aOffsetY;
            aSprite.mWidth = aSrcWidth;
            aSprite.mHeight = aSrcHeight;
            aSprite.mDrawMode = aFontLayer->mDrawMode;
            aSprite.mColor = aLayerColors[(unsigned int)aLayer];
            aSprite.mImage = aFontLayer->mImage.mImage;
            aSprite.mNext = nullptr;
            aSprites.push_back(aSprite);
            GlyphSprite* aRecord = &aSprites.back();
            if (aBucketTails[anOrder] != nullptr)
                aBucketTails[anOrder]->mNext = aRecord;
            else
                aBuckets[anOrder] = aRecord;
            aBucketTails[anOrder] = aRecord;
        }
    }

    Color aSavedColor = theGraphics->GetColor();
    int aSavedDrawMode = theGraphics->GetDrawMode();
    for (int i = 0; i < 256; ++i)
    {
        for (GlyphSprite* aRecord = aBuckets[i]; aRecord != nullptr; aRecord = aRecord->mNext)
        {
            if (aRecord->mDrawMode != -1)
                theGraphics->SetDrawMode(aRecord->mDrawMode);
            theGraphics->SetColor(aRecord->mColor);
            if (aRecord->mImage != nullptr)
            {
                TRect<int> aSrcRect(aRecord->mSrcX, aRecord->mSrcY, aRecord->mWidth, aRecord->mHeight);
                theGraphics->DrawImage(aRecord->mImage, aRecord->mX, aRecord->mY, aSrcRect);
            }
            theGraphics->SetDrawMode(aSavedDrawMode);
        }
    }
    theGraphics->SetColor(aSavedColor);
    theGraphics->SetColorizeImages(aSavedColorize);
    gRenderCritSec.Unlock();
}

int Sexy::ImageFont::SerializeWrite(void* theStream, int theSize)
{
    int aSize = theSize;
    if (theSize <= 0)
    {
        int aLayerSize = MeasureLayerSize(mFontData);
        int aTagsBytes = StringBytes(mFontData->mFileName) + StringBytes(mFontData->mErrorHeader)
            + 8 + aLayerSize;
        for (unsigned int i = 0; i < mTags.size(); ++i)
            aTagsBytes += StringBytes(mTags[i]);
        aSize = aTagsBytes + 9;
    }
    if (theStream == nullptr)
        return aSize;

    unsigned char* aCursor = static_cast<unsigned char*>(theStream);
    int aMagic = kFontFileMagic;
    int aVersion = kFontFileVersion;
    WriteBytes(aCursor, &aMagic, 4);
    WriteBytes(aCursor, &aVersion, 4);
    WriteBytes(aCursor, &mAscent, 4);
    WriteBytes(aCursor, &mAscentPadding, 4);
    WriteBytes(aCursor, &mHeight, 4);
    WriteBytes(aCursor, &mLineSpacingOffset, 4);
    WriteBytes(aCursor, &mFontData->mIsListFont, 1);
    WriteBytes(aCursor, &mFontData->mDefaultPointSize, 4);

    int aCharMapCount = (int)mFontData->mCharMap.size();
    WriteBytes(aCursor, &aCharMapCount, 4);
    for (auto aPair = mFontData->mCharMap.begin(); aPair != mFontData->mCharMap.end(); ++aPair)
    {
        int aFrom = aPair->first;
        int aTo = aPair->second;
        WriteBytes(aCursor, &aFrom, 4);
        WriteBytes(aCursor, &aTo, 4);
    }

    int aLayerCount = (int)mFontData->mLayerList.size();
    WriteBytes(aCursor, &aLayerCount, 4);
    for (auto aLayer = mFontData->mLayerList.begin(); aLayer != mFontData->mLayerList.end(); ++aLayer)
    {
        WriteString(aCursor, aLayer->mLayerName);
        int aTagCount = (int)aLayer->mRequiredTags.size();
        WriteBytes(aCursor, &aTagCount, 4);
        for (int i = 0; i < aTagCount; ++i)
        {
            Sexy::String aTagText;
            ToString(&aTagText, &aLayer->mRequiredTags[(unsigned int)i]);
            WriteString(aCursor, aTagText);
        }
        aTagCount = (int)aLayer->mExcludedTags.size();
        WriteBytes(aCursor, &aTagCount, 4);
        for (int i = 0; i < aTagCount; ++i)
        {
            Sexy::String aTagText;
            ToString(&aTagText, &aLayer->mExcludedTags[(unsigned int)i]);
            WriteString(aCursor, aTagText);
        }

        int aKerningCount = (int)aLayer->mKerningValues.size();
        WriteBytes(aCursor, &aKerningCount, 4);
        if (aKerningCount != 0)
            WriteBytes(aCursor, aLayer->mKerningValues.data(), 8 * aKerningCount);

        int aCharCount = (int)aLayer->mCharDataMap.size();
        WriteBytes(aCursor, &aCharCount, 4);
        for (auto aChar = aLayer->mCharDataMap.begin(); aChar != aLayer->mCharDataMap.end(); ++aChar)
            WriteCharData(aCursor, aChar->first, aChar->second);

        WriteBytes(aCursor, &aLayer->mColorMult, 16);
        WriteBytes(aCursor, &aLayer->mColorAdd, 16);
        WriteString(aCursor, aLayer->mImageFileName);
        int aFormatHint = aLayer->mUseAlphaCorrection;
        WriteBytes(aCursor, &aFormatHint, 4);
        WriteBytes(aCursor, &aLayer->mDrawMode, 4);
        WriteBytes(aCursor, &aLayer->mOffset, 8);
        WriteBytes(aCursor, &aLayer->mSpacing, 4);
        WriteBytes(aCursor, &aLayer->mMinPointSize, 4);
        WriteBytes(aCursor, &aLayer->mMaxPointSize, 4);
        WriteBytes(aCursor, &aLayer->mPointSize, 4);
        WriteBytes(aCursor, &aLayer->mAscent, 4);
        WriteBytes(aCursor, &aLayer->mAscentPadding, 4);
        WriteBytes(aCursor, &aLayer->mHeight, 4);
        WriteBytes(aCursor, &aLayer->mDefaultHeight, 4);
        WriteBytes(aCursor, &aLayer->mLineSpacingOffset, 4);
        WriteBytes(aCursor, &aLayer->mBaseOrder, 4);
    }

    WriteString(aCursor, mFontData->mFileName);
    Sexy::String aErrorHeader;
    ToString(&aErrorHeader, &mFontData->mErrorHeader);
    WriteString(aCursor, aErrorHeader);
    WriteBytes(aCursor, &mPointSize, 4);
    int aTagCount = (int)mTags.size();
    WriteBytes(aCursor, &aTagCount, 4);
    for (int i = 0; i < aTagCount; ++i)
    {
        Sexy::String aTagText;
        ToString(&aTagText, &mTags[(unsigned int)i]);
        WriteString(aCursor, aTagText);
    }
    WriteBytes(aCursor, &mScale, 8);
    WriteBytes(aCursor, &mForceScaledImagesWhite, 1);
    return aSize;
}

int Sexy::ImageFont::GetMappedChar(int theChar)
{
    auto found = mFontData->mCharMap.find(theChar);
    if (found == mFontData->mCharMap.end())
        return theChar;
    return found->second;
}

int Sexy::ImageFont::CharWidthKern(int theChar, int thePrevChar)
{
    int aMappedChar = GetMappedChar(theChar);
    if (thePrevChar != 0)
        thePrevChar = GetMappedChar(thePrevChar);
    return MappedCharWidthKern(aMappedChar, thePrevChar);
}

int Sexy::ImageFont::GetLayerCount()
{
    int aCount = 0;
    for (auto aLayer = mFontData->mLayerList.begin(); aLayer != mFontData->mLayerList.end(); ++aLayer)
    {
        if (NameEndsWithMod(aLayer->mLayerName))
            continue;
        ++aCount;
    }
    return aCount;
}

void Sexy::ImageFont::PopLayerColor(Sexy::String const& theLayerName)
{
    Sexy::String aModName(theLayerName);
    aModName.Append("__MOD", 5);
    for (unsigned int i = 0; i < mActiveLayerList.size(); ++i)
    {
        FontLayer* aFontLayer = mActiveLayerList[i].mLayer;
        if (aFontLayer == nullptr)
            continue;
        const char* aName = aFontLayer->mLayerName.c_str();
        if (strcasecmp(aName, theLayerName.c_str()) == 0 || strcasecmp(aName, aModName.c_str()) == 0)
            mActiveLayerList[i].PopColor();
    }
}

void Sexy::ImageFont::PopLayerColor(int theLayerIndex)
{
    int aIndex = 0;
    for (auto aLayer = mFontData->mLayerList.begin(); aLayer != mFontData->mLayerList.end(); ++aLayer)
    {
        if (NameEndsWithMod(aLayer->mLayerName))
            continue;
        if (aIndex == theLayerIndex)
        {
            PopLayerColor(aLayer->mLayerName);
            return;
        }
        ++aIndex;
    }
}

int Sexy::ImageFont::MappedCharWidthKern(int theChar, int thePrevChar)
{
    Prepare();
    double aScale = (double)mPointSize * mScale;
    int aMaxWidth = 0;
    for (unsigned int i = 0; i < mActiveLayerList.size(); ++i)
    {
        FontLayer* aFontLayer = mActiveLayerList[i].mLayer;
        CharData* aCharData = aFontLayer->GetCharData(theChar);
        double aGlyphScale = aFontLayer->mPointSize != 0
            ? aScale / (double)aFontLayer->mPointSize
            : mScale;
        int aWidth = (int)((double)(short)aCharData->mAdvance * aGlyphScale);
        int aKern = 0;
        if (thePrevChar != 0)
        {
            aKern = aFontLayer->mSpacing;
            CharData* aPrevData = aFontLayer->GetCharData(thePrevChar);
            if (aPrevData->mKernCount != 0)
            {
                FontLayer::KerningValue* aKerning =
                    aFontLayer->mKerningValues.data() + aPrevData->mKernIndex;
                for (unsigned short aPair = 0; aPair < aPrevData->mKernCount; ++aPair)
                {
                    if (aKerning[aPair].mSecondChar == theChar)
                    {
                        aKern = (int)((double)aFontLayer->mSpacing
                            + (double)aKerning[aPair].mAmount * aGlyphScale);
                        break;
                    }
                }
            }
        }
        int aTotal = aKern + aWidth;
        if (aMaxWidth < aTotal)
            aMaxWidth = aTotal;
    }
    return aMaxWidth;
}

void Sexy::ImageFont::PushLayerColor(Sexy::String const& theLayerName, Color const& theColor)
{
    Prepare();
    Sexy::String aModName(theLayerName);
    aModName.Append("__MOD", 5);
    for (unsigned int i = 0; i < mActiveLayerList.size(); ++i)
    {
        FontLayer* aFontLayer = mActiveLayerList[i].mLayer;
        if (aFontLayer == nullptr)
            continue;
        const char* aName = aFontLayer->mLayerName.c_str();
        if (strcasecmp(aName, theLayerName.c_str()) == 0 || strcasecmp(aName, aModName.c_str()) == 0)
            mActiveLayerList[i].PushColor(theColor);
    }
}

void Sexy::ImageFont::PushLayerColor(int theLayerIndex, Color const& theColor)
{
    Prepare();
    int aIndex = 0;
    for (auto aLayer = mFontData->mLayerList.begin(); aLayer != mFontData->mLayerList.end(); ++aLayer)
    {
        if (NameEndsWithMod(aLayer->mLayerName))
            continue;
        if (aIndex == theLayerIndex)
        {
            PushLayerColor(aLayer->mLayerName, theColor);
            return;
        }
        ++aIndex;
    }
}

void Sexy::ImageFont::GlyphExtentsList(int const& theGlyphsList, int& theExtents)
{
    void* aListStorage = const_cast<int*>(&theGlyphsList);
    GlyphListNode* aSentinel = static_cast<GlyphListNode*>(aListStorage);
    void* aExtentsStorage = &theExtents;
    Extents* aExtents = static_cast<Extents*>(aExtentsStorage);
    aExtents->mMinX = 0.0f;
    aExtents->mMinY = 0.0f;
    aExtents->mLastSpanX = 0.0f;
    aExtents->mLastSpanY = 0.0f;
    aExtents->mWidth = 0.0f;
    aExtents->mHeight = 0.0f;
    if (aSentinel->mNext == aSentinel)
        return;

    Prepare();
    int aLayerCount = (int)mActiveLayerList.size();
    float aRunMinX = 0.0f;
    float aRunMinY = 0.0f;
    float aRunMaxX = 0.0f;
    float aRunMaxY = 0.0f;
    bool aFirst = true;

    for (GlyphListNode* aNode = aSentinel->mNext; aNode != aSentinel; aNode = aNode->mNext)
    {
        Sexy::Glyph const* aGlyph = aNode->mGlyph;
        if (aGlyph->mWidth0 == 0 || aGlyph->mWidth == 0)
            continue;

        float aBoxMinX = 0.0f;
        float aBoxMinY = 0.0f;
        float aBoxMaxX = 0.0f;
        float aBoxMaxY = 0.0f;
        if (aLayerCount != 0)
        {
            int aMinX = 0;
            int aMinY = 0;
            int aMaxX = 0;
            int aMaxY = 0;
            void* const* aCached = &aGlyph->mCharData;
            for (int aLayerIndex = 0; aLayerIndex < aLayerCount; ++aLayerIndex)
            {
                ActiveFontLayer* aLayer = &mActiveLayerList[(unsigned int)aLayerIndex];
                FontLayer* aFontLayer = aLayer->mLayer;
                CharData* aCharData = aLayerIndex <= 3
                    ? static_cast<CharData*>(aCached[aLayerIndex])
                    : aFontLayer->GetCharData(aGlyph->mChar);
                if (aCharData == nullptr)
                    continue;

                int aPenX = (int)aGlyph->mX;
                int aPenY = (int)aGlyph->mY;
                double aGlyphScale = mScale;
                if (aFontLayer->mPointSize != 0)
                    aGlyphScale = mScale * (double)(mPointSize / aFontLayer->mPointSize);

                short aOffsetX = (short)aCharData->mOffsetX;
                short aOffsetY = (short)aCharData->mOffsetY;
                int aX;
                int aY;
                if (aGlyphScale == 1.0)
                {
                    aX = aPenX + aFontLayer->mOffset.mX + aOffsetX;
                    aY = aPenY + aFontLayer->mOffset.mY - aFontLayer->mAscent + aOffsetY;
                }
                else
                {
                    aX = aPenX + (int)(aGlyphScale * (double)(aFontLayer->mOffset.mX + aOffsetX));
                    aY = aPenY - (int)(aGlyphScale * (double)(aFontLayer->mAscent
                        - aFontLayer->mOffset.mY - aOffsetY));
                }

                Sexy::TRect<int> aSrc = SourceRect(aLayer, aGlyph->mChar);
                if (aLayerIndex == 0)
                {
                    aMinX = aX;
                    aMinY = aY;
                    aMaxX = aX + aSrc.mWidth;
                    aMaxY = aY + aSrc.mHeight;
                }
                else
                {
                    if (aMinX > aX)
                        aMinX = aX;
                    if (aMinY > aY)
                        aMinY = aY;
                    int aRight = aX + aSrc.mWidth;
                    int aBottom = aY + aSrc.mHeight;
                    if (aMaxX < aRight)
                        aMaxX = aRight;
                    if (aMaxY < aBottom)
                        aMaxY = aBottom;
                }
            }
            aBoxMinX = (float)aMinX;
            aBoxMinY = (float)aMinY;
            aBoxMaxX = (float)aMaxX;
            aBoxMaxY = (float)aMaxY;
        }

        if (aFirst)
        {
            aRunMinX = aBoxMinX;
            aRunMinY = aBoxMinY;
            aRunMaxX = aBoxMaxX;
            aRunMaxY = aBoxMaxY;
            aFirst = false;
        }
        else
        {
            if (aRunMinX > aBoxMinX)
                aRunMinX = aBoxMinX;
            if (aRunMaxX < aBoxMaxX)
                aRunMaxX = aBoxMaxX;
            if (aRunMinY > aBoxMinY)
                aRunMinY = aBoxMinY;
            if (aRunMaxY < aBoxMaxY)
                aRunMaxY = aBoxMaxY;
        }
    }
    aRunMaxX -= aRunMinX;
    aRunMaxY -= aRunMinY;

    Sexy::Glyph const* aFirstGlyph = aSentinel->mNext->mGlyph;
    Sexy::Glyph const* aLastGlyph = aSentinel->mPrev->mGlyph;
    int aFirstPenX = (int)aFirstGlyph->mX;
    int aFirstPenY = (int)aFirstGlyph->mY;
    aExtents->mMinX = aRunMinX - aFirstGlyph->mX;
    aExtents->mMinY = aRunMinY - aFirstGlyph->mY;
    aExtents->mWidth = aRunMaxX;
    aExtents->mHeight = aRunMaxY;
    aExtents->mLastSpanX = (float)((int)((float)aLastGlyph->mMaxWidth + aLastGlyph->mX) - aFirstPenX);
    aExtents->mLastSpanY = (float)((int)(aLastGlyph->mOffsetY + aLastGlyph->mY) - aFirstPenY);
}

void Sexy::ImageFont::DrawStringEx(Sexy::Graphics* theGraphics, int theX, int theY,
                                   Sexy::String const& theString, Color const& theColor,
                                   TRect<int> const* theClipRect,
                                   std::list<TRect<int>>* theDrawnAreas, int* theWidth)
{
    (void)theClipRect;
    if (theString.empty())
    {
        if (theDrawnAreas != nullptr)
            theDrawnAreas->clear();
        if (theWidth != nullptr)
            *theWidth = 0;
        return;
    }

    gRenderCritSec.Lock();
    for (int i = 0; i < 256; ++i)
    {
        sDrawHeads[i] = nullptr;
        sDrawTails[i] = nullptr;
    }
    if (theDrawnAreas != nullptr)
        theDrawnAreas->clear();

    if (!mFontData->mIsListFont)
    {
        if (theWidth != nullptr)
            *theWidth = 0;
        gRenderCritSec.Unlock();
        return;
    }

    Prepare();
    int aSavedColorize = theGraphics->GetColorizeImages();
    theGraphics->SetColorizeImages(true);

    const char* aBytes = theString.c_str();
    unsigned aLength = (unsigned)theString.length();
    int aPen = theX;
    int aRecordCount = 0;
    int aChar = GetMappedChar((unsigned char)aBytes[0]);
    for (unsigned aIndex = 0; aIndex < aLength; ++aIndex)
    {
        int aNext = aIndex >= aLength - 1
            ? 0
            : GetMappedChar((unsigned char)aBytes[aIndex + 1]);
        int aPenMax = aPen;
        for (unsigned int aLayerIndex = 0; aLayerIndex < mActiveLayerList.size(); ++aLayerIndex)
        {
            ActiveFontLayer* aLayer = &mActiveLayerList[aLayerIndex];
            FontLayer* aFontLayer = aLayer->mLayer;
            CharData* aCharData = aFontLayer->GetCharData(aChar);

            double aGlyphScale = mScale;
            if (aFontLayer->mPointSize != 0)
                aGlyphScale = mScale * (double)(mPointSize / aFontLayer->mPointSize);

            short aOffsetX = (short)aCharData->mOffsetX;
            short aOffsetY = (short)aCharData->mOffsetY;
            int aAdvance;
            int aX;
            int aY;
            int aKern;
            if (aGlyphScale == 1.0)
            {
                aAdvance = (short)aCharData->mAdvance;
                aY = theY + aFontLayer->mOffset.mY - aFontLayer->mAscent + aOffsetY;
                aX = aFontLayer->mOffset.mX + aPen + aOffsetX;
                if (aNext != 0)
                {
                    aKern = aFontLayer->mSpacing;
                    if (aCharData->mKernCount != 0)
                    {
                        FontLayer::KerningValue* aKerning =
                            aFontLayer->mKerningValues.data() + aCharData->mKernIndex;
                        for (unsigned short i = 0; i < aCharData->mKernCount; ++i)
                        {
                            if (aKerning[i].mSecondChar == aNext)
                            {
                                aKern += aKerning[i].mAmount;
                                break;
                            }
                        }
                    }
                }
                else
                    aKern = 0;
            }
            else
            {
                aY = theY - (int)(aGlyphScale * (double)(aFontLayer->mAscent
                    - aFontLayer->mOffset.mY - aOffsetY));
                aX = aPen + (int)(aGlyphScale * (double)(aOffsetX + aFontLayer->mOffset.mX));
                aAdvance = (int)(aGlyphScale * (double)(short)aCharData->mAdvance);
                if (aNext != 0)
                {
                    aKern = aFontLayer->mSpacing;
                    if (aCharData->mKernCount != 0)
                    {
                        FontLayer::KerningValue* aKerning =
                            aFontLayer->mKerningValues.data() + aCharData->mKernIndex;
                        for (unsigned short i = 0; i < aCharData->mKernCount; ++i)
                        {
                            if (aKerning[i].mSecondChar == aNext)
                                aKern += (int)((double)aKerning[i].mAmount * aGlyphScale);
                        }
                    }
                }
                else
                    aKern = 0;
            }

            Color aDrawn;
            if (aLayer->mColorStack.empty())
            {
                Color const& aMult = aFontLayer->mColorMult;
                Color const& aAdd = aFontLayer->mColorAdd;
                aDrawn.mRed = ClampChannel(MulDiv255(aMult.mRed, theColor.mRed) + aAdd.mRed);
                aDrawn.mGreen = ClampChannel(MulDiv255(aMult.mGreen, theColor.mGreen) + aAdd.mGreen);
                aDrawn.mBlue = ClampChannel(MulDiv255(aMult.mBlue, theColor.mBlue) + aAdd.mBlue);
                aDrawn.mAlpha = ClampChannel(MulDiv255(aMult.mAlpha, theColor.mAlpha) + aAdd.mAlpha);
            }
            else
            {
                Color const& aStacked = aLayer->mColorStack.back();
                Color const& aMult = aFontLayer->mColorMult;
                Color aMixed;
                aMixed.mRed = ClampChannel(MulDiv255(aMult.mRed, aStacked.mRed));
                aMixed.mGreen = ClampChannel(MulDiv255(aMult.mGreen, aStacked.mGreen));
                aMixed.mBlue = ClampChannel(MulDiv255(aMult.mBlue, aStacked.mBlue));
                aMixed.mAlpha = ClampChannel(MulDiv255(aMult.mAlpha, aStacked.mAlpha));
                Color const& aAdd = aFontLayer->mColorAdd;
                aDrawn.mRed = ClampChannel(MulDiv255(aMixed.mRed, theColor.mRed) + aAdd.mRed);
                aDrawn.mGreen = ClampChannel(MulDiv255(aMixed.mGreen, theColor.mGreen) + aAdd.mGreen);
                aDrawn.mBlue = ClampChannel(MulDiv255(aMixed.mBlue, theColor.mBlue) + aAdd.mBlue);
                aDrawn.mAlpha = ClampChannel(MulDiv255(aMixed.mAlpha, theColor.mAlpha) + aAdd.mAlpha);
            }

            if (aRecordCount >= 0x1000)
                break;

            Sexy::TRect<int> aSrc = SourceRect(aLayer, aChar);
            int aBucket = (short)aCharData->mPageHeight + aFontLayer->mBaseOrder + 0x80;
            if (aBucket < 0)
                aBucket = 0;
            else if (aBucket >= 255)
                aBucket = 255;

            DrawGlyphRecord* aRecord = &sDrawRecords[aRecordCount];
            ++aRecordCount;
            aRecord->mImage = aLayer->mImage;
            aRecord->mX = aX;
            aRecord->mY = aY;
            aRecord->mSrcX = aSrc.mX;
            aRecord->mSrcY = aSrc.mY;
            aRecord->mSrcWidth = aSrc.mWidth;
            aRecord->mSrcHeight = aSrc.mHeight;
            aRecord->mDrawMode = aFontLayer->mDrawMode;
            aRecord->mColor = aDrawn;
            aRecord->mNext = nullptr;
            if (sDrawTails[aBucket] != nullptr)
                sDrawTails[aBucket]->mNext = aRecord;
            else
                sDrawHeads[aBucket] = aRecord;
            sDrawTails[aBucket] = aRecord;

            if (theDrawnAreas != nullptr)
            {
                Sexy::TRect<int> aWidthRect = SourceRect(aLayer, aChar);
                Sexy::TRect<int> aHeightRect = SourceRect(aLayer, aChar);
                theDrawnAreas->push_back(TRect<int>(aX, aY, aWidthRect.mWidth, aHeightRect.mHeight));
            }

            int aSpan = aAdvance + aKern + aPen;
            if (aPenMax < aSpan)
                aPenMax = aSpan;
        }
        aPen = aPenMax;
        aChar = aNext;
    }

    if (theWidth != nullptr)
        *theWidth = aPen - theX;

    Color aSavedColor = theGraphics->GetColor();
    for (int aBucket = 0; aBucket < 256; ++aBucket)
    {
        for (DrawGlyphRecord* aRecord = sDrawHeads[aBucket]; aRecord != nullptr; aRecord = aRecord->mNext)
        {
            int aSavedMode = theGraphics->GetDrawMode();
            if (aRecord->mDrawMode != -1)
                theGraphics->SetDrawMode(aRecord->mDrawMode);
            theGraphics->SetColor(aRecord->mColor);
            if (aRecord->mImage != nullptr)
            {
                TRect<int> aSrc(aRecord->mSrcX, aRecord->mSrcY, aRecord->mSrcWidth, aRecord->mSrcHeight);
                theGraphics->DrawImage(aRecord->mImage, aRecord->mX, aRecord->mY, aSrc);
            }
            theGraphics->SetDrawMode(aSavedMode);
        }
    }
    theGraphics->SetColor(aSavedColor);
    theGraphics->SetColorizeImages(aSavedColorize != 0);
    gRenderCritSec.Unlock();
}

void Sexy::ImageFont::DrawStringEx(Sexy::Graphics* theGraphics, int theX, int theY,
                                   Sexy::WString const& theString, Color const& theColor,
                                   TRect<int> const* theClipRect,
                                   std::list<TRect<int>>* theDrawnAreas, int* theWidth)
{
    DrawStringEx(theGraphics, theX, theY, AsString(theString), theColor,
                 theClipRect, theDrawnAreas, theWidth);
}

Sexy::ImageFont::ImageFont(Sexy::Image* theImage)
    : mFontData(nullptr)
    , mPointSize(0)
    , mRecalcFontSize(false)
    , mActiveListPad(0)
    , mScale(1.0)
    , mForceScaledImagesWhite(false)
    , mApp(nullptr)
    , mDigestComputed(false)
{
    std::memset(mDigest, 0, sizeof(mDigest));
    mFontData = new FontData();
    mFontData->Ref();
    mFontData->mIsListFont = true;
    mPointSize = mFontData->mDefaultPointSize;
    mRecalcFontSize = false;
    mForceScaledImagesWhite = false;

    FontLayer aLayer(mFontData);
    mFontData->mLayerList.push_back(aLayer);
    FontLayer* aFontLayer = &mFontData->mLayerList.back();
    mFontData->mLayerMap[aFontLayer->mLayerName] = aFontLayer;
    aFontLayer->mImage.Release();
    aFontLayer->mImage.mImage = theImage;
    int aHeight = aFontLayer->mImage.mImage->GetHeight();
    aFontLayer->mDefaultHeight = aHeight;
    aFontLayer->mAscent = aHeight;
}

int Sexy::ImageFont::SerializeRead(void* theStream, int theSize)
{
    (void)theSize;
    if (theStream == nullptr)
        return 0;
    return ReadFont(this, static_cast<const unsigned char*>(theStream), false);
}

int Sexy::ImageFont::SerializeReadEndian(void* theStream, int theSize)
{
    (void)theSize;
    if (theStream == nullptr)
        return 0;
    return ReadFont(this, static_cast<const unsigned char*>(theStream), true);
}

Sexy::ImageFont::ImageFont(Sexy::ImageFont const& theOther)
    : Font(theOther)
    , mFontData(theOther.mFontData)
    , mPointSize(theOther.mPointSize)
    , mTags(theOther.mTags)
    , mRecalcFontSize(theOther.mRecalcFontSize)
    , mActiveListPad(theOther.mActiveListPad)
    , mScale(theOther.mScale)
    , mForceScaledImagesWhite(theOther.mForceScaledImagesWhite)
    , mApp(theOther.mApp)
    , mDigestComputed(theOther.mDigestComputed)
{
    std::memcpy(mDigest, theOther.mDigest, sizeof(mDigest));
    mFontData->Ref();
    if (mRecalcFontSize)
        mActiveLayerList = theOther.mActiveLayerList;
}

Sexy::ImageFont* Sexy::ImageFont::Duplicate()
{
    return new ImageFont(*this);
}

int Sexy::ImageFont::UpdateDigest(Sexy::String const& theFileName)
{
    if (mDigestComputed)
        return mDigestComputed;
    Sexy::Buffer aBuffer;
    int aResult = mApp->ReadBufferFromFile(theFileName, &aBuffer, true);
    if (aResult == 0)
        return 0;

    MD5Context aState;
    MD5Init(&aState);
    const unsigned char* aData = aBuffer.mData.empty() ? nullptr : aBuffer.mData.data();
    size_t aByteCount = (size_t)((aBuffer.mDataBitSize + 7) / 8);
    MD5Update(&aState, aData, (unsigned int)aByteCount);
    MD5Final(mDigest, &aState);
    mDigestComputed = true;
    return aResult;
}

bool Sexy::ImageFont::CheckCachedBuffer(Sexy::Buffer& theBuffer, Sexy::String const& theFileName)
{
    const unsigned char* aData = theBuffer.mData.empty() ? nullptr : theBuffer.mData.data();
    if (UpdateDigest(theFileName) == 0)
        return false;
    if (aData == nullptr || std::memcmp(aData, mDigest, sizeof(mDigest)) != 0)
        return false;

    const unsigned char* aCursor = aData + sizeof(mDigest);
    unsigned int aWord = 0;
    ReadBytes(aCursor, &aWord, 4);
    if (aWord != (unsigned int)kFontFileMagic)
        return false;
    ReadBytes(aCursor, &aWord, 4);
    return aWord == (unsigned int)kFontFileVersion;
}

void Sexy::ImageFont::GenerateActiveFontLayers()
{
    if (!mFontData->mIsListFont)
        return;

    mActiveLayerList.clear();
    mAscent = 0;
    mAscentPadding = 0;
    mHeight = 0;
    mLineSpacingOffset = 0;
    if (mFontData->mLayerList.empty())
        return;

    bool aFirstMatch = true;
    for (auto aNode = mFontData->mLayerList.begin(); aNode != mFontData->mLayerList.end(); ++aNode)
    {
        FontLayer* aFontLayer = &*aNode;
        if (mPointSize < aFontLayer->mMinPointSize || mPointSize > aFontLayer->mMaxPointSize)
            continue;

        bool aMatch = true;
        for (unsigned int i = 0; i < aFontLayer->mRequiredTags.size(); ++i)
        {
            if (!VectorHas(mTags, aFontLayer->mRequiredTags[i]))
                aMatch = false;
        }
        if (aMatch && !mTags.empty())
        {
            for (unsigned int i = 0; i < mTags.size(); ++i)
            {
                if (VectorHas(aFontLayer->mExcludedTags, mTags[i]))
                    aMatch = false;
            }
        }
        if (!aMatch)
            continue;

        mActiveLayerList.push_back(ActiveFontLayer());
        ActiveFontLayer* aActive = &mActiveLayerList.back();
        aActive->mLayer = aFontLayer;

        double aMetricScale = mScale;
        double aMetricDiv = 1.0;
        bool aRescale = !(mScale == 1.0
            && (aFontLayer->mPointSize == 0 || aFontLayer->mPointSize == mPointSize));
        if (aRescale && aFontLayer->mPointSize != 0)
        {
            aMetricScale = mScale * (double)mPointSize;
            aMetricDiv = (double)aFontLayer->mPointSize;
        }

        if (aRescale)
        {
            Sexy::MemoryImage* aScaled = new Sexy::MemoryImage(mFontData->mApp);
            int aTotalWidth = 0;
            int aMaxHeight = 0;
            double aInvPoint = 1.0 / aMetricDiv;
            for (auto aChar = aFontLayer->mCharDataMap.begin();
                 aChar != aFontLayer->mCharDataMap.end(); ++aChar)
            {
                int aWidth = (int)((aMetricScale * (double)SignedHalf(aChar->second.mWidth)) * aInvPoint);
                int aHeight = (int)((aMetricScale * (double)SignedHalf(aChar->second.mHeight)) * aInvPoint);
                aActive->mCharRects[aChar->first] = TRect<int>(aTotalWidth, 0, aWidth, aHeight);
                aTotalWidth += aWidth;
                if (aMaxHeight < aHeight)
                    aMaxHeight = aHeight;
            }

            aActive->mImage = aScaled;
            aActive->mOwnsImage = true;
            aScaled->Create(aTotalWidth, aMaxHeight);
            {
                Sexy::Graphics aGraphics(aScaled);
                for (auto aChar = aFontLayer->mCharDataMap.begin();
                     aChar != aFontLayer->mCharDataMap.end(); ++aChar)
                {
                    Sexy::Image* aSrc = aFontLayer->mImage;
                    if (aSrc == nullptr)
                        continue;
                    TRect<int> aDestRect = aActive->mCharRects[aChar->first];
                    TRect<int> aSrcRect(SignedHalf(aChar->second.mX), SignedHalf(aChar->second.mY),
                                        SignedHalf(aChar->second.mWidth), SignedHalf(aChar->second.mHeight));
                    aGraphics.DrawImage(aSrc, aDestRect, aSrcRect);
                }
                if (mForceScaledImagesWhite)
                {
                    int aCount = aScaled->mWidth * aScaled->mHeight;
                    unsigned int* aBits = aScaled->GetBits();
                    for (int aPixel = 0; aPixel < aCount; ++aPixel)
                        aBits[aPixel] |= 0x00FFFFFFu;
                }
                aScaled->Palletize();
            }
        }
        else
        {
            aActive->mImage = aFontLayer->mImage;
            aActive->mOwnsImage = false;
            for (auto aChar = aFontLayer->mCharDataMap.begin();
                 aChar != aFontLayer->mCharDataMap.end(); ++aChar)
            {
                aActive->mCharRects[aChar->first] = TRect<int>(
                    SignedHalf(aChar->second.mX), SignedHalf(aChar->second.mY),
                    SignedHalf(aChar->second.mWidth), SignedHalf(aChar->second.mHeight));
            }
        }

        int aAscent = (int)((aMetricScale * (double)aFontLayer->mAscent) / aMetricDiv);
        if (aAscent > mAscent)
            mAscent = aAscent;
        int aHeightSrc = aFontLayer->mHeight != 0 ? aFontLayer->mHeight : aFontLayer->mDefaultHeight;
        int aLayerHeight = (int)((aMetricScale * (double)aHeightSrc) / aMetricDiv);
        if (aLayerHeight > mHeight)
            mHeight = aLayerHeight;
        int aPadding = (int)((aMetricScale * (double)aFontLayer->mAscentPadding) / aMetricDiv);
        int aSpacing = (int)((aMetricScale * (double)aFontLayer->mLineSpacingOffset) / aMetricDiv);
        if (aFirstMatch)
        {
            mAscentPadding = aPadding;
            mLineSpacingOffset = aSpacing;
            aFirstMatch = false;
        }
        else
        {
            if (aPadding < mAscentPadding)
                mAscentPadding = aPadding;
            if (mLineSpacingOffset < aSpacing)
                mLineSpacingOffset = aSpacing;
        }
    }
}

Sexy::ImageFont::ImageFont(Sexy::SexyAppBase* theApp, Sexy::String const& theFileName)
    : mFontData(nullptr)
    , mPointSize(0)
    , mRecalcFontSize(false)
    , mActiveListPad(0)
    , mScale(1.0)
    , mForceScaledImagesWhite(false)
    , mApp(theApp)
    , mDigestComputed(false)
{
    std::memset(mDigest, 0, sizeof(mDigest));
    mFontData = new FontData();
    mFontData->Ref();

    Sexy::String aFileName(theFileName);
    aFileName.Append(".cfu2", 5);
    Sexy::String aRelativeName = "cached\\";
    aRelativeName += aFileName;
    Sexy::String aAppDataFolder;
    GetAppDataFolder(&aAppDataFolder);
    Sexy::String aCacheName(aAppDataFolder);
    aCacheName += aRelativeName;

    Sexy::Buffer aBuffer;
    bool aCached = (theApp->ReadBufferFromFile(aFileName, &aBuffer, true) != 0
            && aBuffer.mDataBitSize > 120
            && CheckCachedBuffer(aBuffer, theFileName))
        || (theApp->ReadBufferFromFile(aRelativeName, &aBuffer, true) != 0
            && aBuffer.mDataBitSize > 120
            && CheckCachedBuffer(aBuffer, theFileName))
        || (theApp->ReadBufferFromFile(aCacheName, &aBuffer, true) != 0
            && aBuffer.mDataBitSize > 120
            && CheckCachedBuffer(aBuffer, theFileName));
    if (aCached)
    {
        unsigned char* aData = aBuffer.mData.empty() ? nullptr : aBuffer.mData.data();
        int aByteCount = (aBuffer.mDataBitSize + 7) / 8;
        if (aData != nullptr
            && SerializeRead(aData + sizeof(mDigest), aByteCount - (int)sizeof(mDigest)) != 0)
        {
            logfi("Loaded cached %s successfully.", aData);
            return;
        }
    }

    mFontData->Load(theApp, theFileName);
    mPointSize = mFontData->mDefaultPointSize;
    GenerateActiveFontLayers();
    mRecalcFontSize = true;
    mForceScaledImagesWhite = false;

    if (theApp->mCacheFonts)
    {
        Sexy::Buffer aFontBuffer;
        if (theApp->ReadBufferFromFile(theFileName, &aFontBuffer, true) != 0)
        {
            unsigned char aDigest[16];
            MD5Context aState;
            MD5Init(&aState);
            const unsigned char* aFontBytes = aFontBuffer.mData.empty() ? nullptr : aFontBuffer.mData.data();
            MD5Update(&aState, aFontBytes, (unsigned int)((aFontBuffer.mDataBitSize + 7) / 8));
            MD5Final(aDigest, &aState);

            Sexy::String aDir;
            GetFileDir(&aDir, &aCacheName, 0);
            MkDir(aDir);

            int aSize = SerializeWrite(nullptr, 0);
            unsigned char* aCache = new unsigned char[aSize + (int)sizeof(aDigest)];
            std::memcpy(aCache, aDigest, sizeof(aDigest));
            SerializeWrite(aCache + sizeof(aDigest), aSize);
            GetFileDir(&aDir, &aCacheName, 0);
            MkDir(aDir);
            theApp->WriteBytesToFile(aCacheName, aCache, aSize + sizeof(aDigest));
            delete[] aCache;
        }
    }
}

Sexy::ImageFont::ImageFont(Sexy::Image* theImage, Sexy::String const& theFileName)
    : mFontData(nullptr)
    , mPointSize(0)
    , mRecalcFontSize(false)
    , mActiveListPad(0)
    , mScale(1.0)
    , mForceScaledImagesWhite(false)
    , mApp(nullptr)
    , mDigestComputed(false)
{
    std::memset(mDigest, 0, sizeof(mDigest));
    mFontData = new FontData();
    mFontData->Ref();
    mFontData->LoadLegacy(theImage, theFileName);
    mPointSize = mFontData->mDefaultPointSize;
    GenerateActiveFontLayers();
    mRecalcFontSize = true;
}

Sexy::ImageFont::~ImageFont()
{
    if (mFontData != nullptr)
        mFontData->DeRef();
}

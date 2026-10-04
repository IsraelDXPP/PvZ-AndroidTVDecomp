// FontLayer. An empty layer has empty maps, white multiply, transparent
// add, and the trailing defaults the constructor stores.

#include "FontLayer.h"
#include "CharData.h"
#include "FontData.h"
#include "SharedImageRef.h"

Sexy::FontLayer::FontLayer(Sexy::FontData* theFontData)
{
    mFontData = theFontData;
    mColorMult = Color::White;
    mColorAdd = Color(0, 0, 0, 0);

    mUseAlphaCorrection = 0;
    mDrawMode = -1;
    mOffset.mX = 0;
    mOffset.mY = 0;
    mSpacing = 0;
    mMinPointSize = -1;
    mMaxPointSize = -1;
    mPointSize = 0;
    mAscent = 0;
    mAscentPadding = 0;
    mHeight = 0;
    mDefaultHeight = 0;
    mLineSpacingOffset = 0;
    mBaseOrder = 0;
}

bool Sexy::FontLayer::isGlyph(int theChar)
{
    return mCharDataMap.find(theChar) != mCharDataMap.end();
}

Sexy::CharData* Sexy::FontLayer::SetCharData(int theChar)
{
    return &mCharDataMap[theChar];
}

Sexy::CharData* Sexy::FontLayer::GetCharData(int theChar)
{
    return &mCharDataMap[theChar];
}

Sexy::FontLayer::FontLayer(Sexy::FontLayer const& theOther)
    : mFontData(theOther.mFontData)
    , mExtendedInfo(theOther.mExtendedInfo)
    , mLayerName(theOther.mLayerName)
    , mRequiredTags(theOther.mRequiredTags)
    , mExcludedTags(theOther.mExcludedTags)
    , mKerningValues(theOther.mKerningValues)
    , mCharDataMap(theOther.mCharDataMap)
    , mColorMult(theOther.mColorMult)
    , mColorAdd(theOther.mColorAdd)
    , mImage(theOther.mImage)
    , mImageFileName(theOther.mImageFileName)
    , mUseAlphaCorrection(theOther.mUseAlphaCorrection)
    , mDrawMode(theOther.mDrawMode)
    , mOffset(theOther.mOffset)
    , mSpacing(theOther.mSpacing)
    , mMinPointSize(theOther.mMinPointSize)
    , mMaxPointSize(theOther.mMaxPointSize)
    , mPointSize(theOther.mPointSize)
    , mAscent(theOther.mAscent)
    , mAscentPadding(theOther.mAscentPadding)
    , mHeight(theOther.mHeight)
    , mDefaultHeight(theOther.mDefaultHeight)
    , mLineSpacingOffset(theOther.mLineSpacingOffset)
    , mBaseOrder(theOther.mBaseOrder)
{
}

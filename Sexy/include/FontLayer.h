// FontLayer is one drawing pass of an image font. It is not polymorphic:
// the first word is the FontData that owns it, not a vtable.
//
// Character metrics live in mCharDataMap. Extra descriptor pairs live in
// mExtendedInfo. Tag filters and kerning pairs are ordinary vectors.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_FONTLAYER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_FONTLAYER_H

#include "ReconTypes.h"
#include "CharData.h"
#include "SharedImageRef.h"

#include <map>
#include <vector>

namespace Sexy
{

class FontData;

class FontLayer
{
public:
    // One kerning pair: the following character and the extra advance.
    struct KerningValue
    {
        int mSecondChar;
        int mAmount;
    };

    FontData *mFontData;
    std::map<Sexy::String, Sexy::String> mExtendedInfo;
    Sexy::String mLayerName;
    std::vector<Sexy::String> mRequiredTags;
    std::vector<Sexy::String> mExcludedTags;
    std::vector<KerningValue> mKerningValues;
    std::map<int, Sexy::CharData> mCharDataMap;

    Color mColorMult;
    Color mColorAdd;
    Sexy::SharedImageRef mImage;
    Sexy::String mImageFileName;

    // A word, not a bool. Serialize reads four bytes into it and later
    // passes that word to SetPixelFormatHint.
    int mUseAlphaCorrection;
    int mDrawMode;
    Point mOffset;
    int mSpacing;
    int mMinPointSize;
    int mMaxPointSize;
    int mPointSize;
    int mAscent;
    int mAscentPadding;
    int mHeight;
    int mDefaultHeight;
    int mLineSpacingOffset;
    int mBaseOrder;

    FontLayer(Sexy::FontData *theFontData);
    FontLayer(Sexy::FontLayer const &theOther);
    bool isGlyph(int theChar);
    Sexy::CharData *SetCharData(int theChar);
    Sexy::CharData *GetCharData(int theChar);
};

} // namespace Sexy

using Sexy::FontLayer;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_FONTLAYER_H

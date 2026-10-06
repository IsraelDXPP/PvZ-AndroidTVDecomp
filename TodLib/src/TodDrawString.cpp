// TodDrawString -- the four text drawers from libGameMain.so.
//
// The binary decodes text into a four-byte character string before measuring
// or wrapping it (Graphics::WStringFromString @0x30B1FC). Keep one UTF-32 code
// point per character here and encode runs back to UTF-8 for Font. Justification
// is an int: 0 left,
// 1 and 4 right, 2 and 5 center. Wrapped text whose justification is 3, 4 or
// 5 is measured first and then shifted so the block sits in the middle of
// the rect. {NAME} tags look up gTodStringFormats and swap the font, colour,
// extra line gap and flags for the rest of the line.

#include "ReconFreeFunctions.h"

#include "ActiveFontLayer.h"
#include "ButtonGlyphs.h"
#include "CharData.h"
#include "Font.h"
#include "FontData.h"
#include "FontLayer.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "SexyAppBase.h"

#include <cmath>
#include <cstring>
#include <string>

namespace {

std::u32string DecodeText(const Sexy::String& theText)
{
    std::u32string aCodePoints;
    aCodePoints.reserve(theText.length());
    unsigned int aByteIndex = 0;
    while (aByteIndex < theText.length())
        aCodePoints.push_back(static_cast<char32_t>(ReadCodePoint(theText, aByteIndex)));
    return aCodePoints;
}

Sexy::String EncodeText(const std::u32string& theWide)
{
    return CodePointsToUtf8(std::vector<int>(theWide.begin(), theWide.end()));
}

Sexy::String ReplaceButtonCaptions(const Sexy::String& theText)
{
    Sexy::String aResult;
    TodReplaceButtons(&aResult, &theText);
    return aResult;
}

Sexy::String PrepareText(const Sexy::String& theText)
{
    return ReplaceButtonCaptions(TodStringTranslate(theText));
}

int ShiftX(Sexy::Font* theFont, const std::u32string& theWide, int theX, int theJustification);

static int MulDiv255(int theLeft, int theRight)
{
    int aProduct = (int)((unsigned int)theLeft * (unsigned int)theRight);
    long long aWide = (long long)aProduct * (int)0x80808081;
    int aHi = (int)(aWide >> 32);
    int aSum = (int)((unsigned int)aHi + (unsigned int)aProduct);
    return (aSum >> 7) - (aProduct >> 31);
}

static int ClampChannel(int theValue)
{
    return theValue >= 255 ? 255 : theValue;
}

struct DrawStyle
{
    Sexy::Font* mFont;
    Sexy::Color mColor;
    int mExtra;
    unsigned int mFlags;
};

void ApplyFormatTag(DrawStyle& theStyle, const std::u32string& theName)
{
    if (gTodStringFormatCount <= 0 || gTodStringFormats == nullptr)
        return;
    for (int aIndex = 0; aIndex < gTodStringFormatCount; ++aIndex)
    {
        TodStringListFormat& aFormat = gTodStringFormats[aIndex];
        const char* aName = aFormat.mName.c_str();
        unsigned int aLength = aFormat.mName.length();
        if (theName.size() != aLength)
            continue;
        bool aMatch = true;
        for (unsigned int aChar = 0; aChar < aLength; ++aChar)
        {
            if (theName[aChar] != (char32_t)(unsigned char)aName[aChar])
            {
                aMatch = false;
                break;
            }
        }
        if (!aMatch)
            continue;
        if (aFormat.mFont != nullptr)
            theStyle.mFont = aFormat.mFont;
        if (aFormat.mColor != Sexy::Color(0, 0, 0, 0))
            theStyle.mColor = aFormat.mColor;
        theStyle.mExtra = aFormat.mOffset;
        theStyle.mFlags = aFormat.mFlags;
        return;
    }
}

int DrawLine(Sexy::Graphics* theGraphics, const std::u32string& theWide,
             int theX, int theY, const DrawStyle& theStyle, int theWidth,
             int theJustification, bool theDraw, int theStart, int theCount)
{
    int aOrigin = theX;
    if (theDraw && theGraphics != nullptr)
    {
        if (theJustification == 1 || theJustification == 4 || theJustification == 2 || theJustification == 5)
        {
            int aMeasured = DrawLine(theGraphics, theWide, theX, theY, theStyle, theWidth,
                                     0, false, theStart, theCount);
            if (theJustification == 1 || theJustification == 4)
                aOrigin = theX + theWidth - aMeasured;
            else
                aOrigin = theX + (theWidth - aMeasured) / 2;
        }
    }

    int aEnd = theStart + theCount;
    if (theCount < 0)
        aEnd = (int)theWide.size();
    if (aEnd > (int)theWide.size())
        aEnd = (int)theWide.size();

    DrawStyle aStyle = theStyle;
    std::u32string aRun;
    int aPen = 0;
    bool aCollapsed = false;
    for (int aIndex = theStart; aIndex < aEnd; ++aIndex)
    {
        char32_t aChar = theWide[aIndex];
        if (aChar == U'{')
        {
            int aClose = aIndex + 1;
            while (aClose < aEnd && theWide[aClose] != U'}')
                ++aClose;
            if (aClose < aEnd)
            {
                if (theDraw && theGraphics != nullptr && !aRun.empty())
                {
                    aStyle.mFont->DrawString(theGraphics, aOrigin + aPen, theY, EncodeText(aRun),
                                             aStyle.mColor, theGraphics->mClipRect);
                    aPen += aStyle.mFont->StringWidth(EncodeText(aRun));
                    aRun.clear();
                }
                else if (!aRun.empty())
                {
                    aPen += aStyle.mFont->StringWidth(EncodeText(aRun));
                    aRun.clear();
                }
                ApplyFormatTag(aStyle, theWide.substr(aIndex + 1, aClose - aIndex - 1));
                aIndex = aClose;
                aCollapsed = false;
                continue;
            }
        }

        if (aChar == U'<')
        {
            int aClose = aIndex + 1;
            while (aClose < aEnd && theWide[aClose] != U'>')
                ++aClose;
            if (aClose < aEnd)
            {
                Sexy::String aTag = EncodeText(theWide.substr(aIndex, aClose - aIndex + 1));
                int aCel = ButtonGlyphCel(aTag.c_str(), (int)aTag.size());
                if (aCel >= 0)
                {
                    if (!aRun.empty())
                    {
                        if (theDraw && theGraphics != nullptr)
                            aStyle.mFont->DrawString(theGraphics, aOrigin + aPen, theY, EncodeText(aRun),
                                                     aStyle.mColor, theGraphics->mClipRect);
                        aPen += aStyle.mFont->StringWidth(EncodeText(aRun));
                        aRun.clear();
                    }
                    Sexy::String aSpaces = ButtonTagAsSpaces(aStyle.mFont, aTag.c_str(), (int)aTag.size());
                    int aSlot = aStyle.mFont->StringWidth(aSpaces);
                    if (theDraw && theGraphics != nullptr)
                    {
                        aStyle.mFont->DrawString(theGraphics, aOrigin + aPen, theY, aSpaces,
                                                 aStyle.mColor, theGraphics->mClipRect);
                        if (ButtonGlyphsActive())
                            DrawButtonGlyph(theGraphics, aOrigin + aPen, theY, aStyle.mFont, aCel, aSlot);
                    }
                    aPen += aSlot;
                    aIndex = aClose;
                    aCollapsed = false;
                    continue;
                }
            }
        }

        if ((aStyle.mFlags & 1) != 0 && (aChar == U' ' || aChar == U'\n'))
        {
            if (!aCollapsed)
            {
                aRun += U' ';
                aCollapsed = true;
            }
            continue;
        }
        aCollapsed = false;
        aRun += aChar;
    }

    if (!aRun.empty())
    {
        if (theDraw && theGraphics != nullptr)
            aStyle.mFont->DrawString(theGraphics, aOrigin + aPen, theY, EncodeText(aRun),
                                     aStyle.mColor, theGraphics->mClipRect);
        aPen += aStyle.mFont->StringWidth(EncodeText(aRun));
    }
    return aPen;
}

int ShiftX(Sexy::Font* theFont, const std::u32string& theWide, int theX, int theJustification)
{
    DrawStyle aStyle;
    aStyle.mFont = theFont;
    aStyle.mColor = Sexy::Color::White;
    aStyle.mExtra = 0;
    aStyle.mFlags = 0;
    int aWidth = DrawLine(nullptr, theWide, 0, 0, aStyle, 0, 0, false, 0, (int)theWide.size());
    if (theJustification == 1 || theJustification == 4)
        return theX - aWidth;
    if (theJustification == 2 || theJustification == 5)
        return theX - aWidth / 2;
    return theX;
}

struct MatrixGlyph
{
    Sexy::Image* mImage;
    int mX;
    int mY;
    Sexy::TRect<int> mSrc;
    int mDrawMode;
    Sexy::Color mColor;
    MatrixGlyph* mNext;
};

Sexy::TRect<int> LayerSourceRect(Sexy::ActiveFontLayer* theLayer, int theChar)
{
    auto found = theLayer->mCharRects.find(theChar);
    if (found == theLayer->mCharRects.end())
        found = theLayer->mCharRects.emplace(theChar, Sexy::TRect<int>()).first;
    return found->second;
}

void DrawStringOnMatrix(Sexy::Graphics* theGraphics, Sexy::ImageFont* theFont,
                        const Sexy::SexyMatrix3& theMatrix, const std::u32string& theWide,
                        const Sexy::Color& theColor)
{
    MatrixGlyph* aHeads[256];
    MatrixGlyph* aTails[256];
    MatrixGlyph aRecords[4096];
    for (int aBucket = 0; aBucket < 256; ++aBucket)
    {
        aHeads[aBucket] = nullptr;
        aTails[aBucket] = nullptr;
    }
    if (theFont->mFontData == nullptr || theFont->mFontData->mIsListFont == 0)
        return;

    theFont->Prepare();
    if (theWide.empty())
        return;

    int aRecordCount = 0;
    int aPen = 0;
    for (unsigned int aIndex = 0; aIndex < theWide.size() && aRecordCount < 4096; ++aIndex)
    {
        int aChar = theFont->GetMappedChar((int)theWide[aIndex]);
        int aNext = 0;
        if (aIndex + 1 < theWide.size())
            aNext = theFont->GetMappedChar((int)theWide[aIndex + 1]);

        int aPenMax = aPen;
        for (unsigned int aLayerIndex = 0;
             aLayerIndex < theFont->mActiveLayerList.size() && aRecordCount < 4096;
             ++aLayerIndex)
        {
            Sexy::ActiveFontLayer* aLayer = &theFont->mActiveLayerList[aLayerIndex];
            Sexy::FontLayer* aFontLayer = aLayer->mLayer;
            Sexy::CharData* aGlyph = aFontLayer->GetCharData(aChar);
            double aScale = theFont->mScale;
            if (aFontLayer->mPointSize != 0)
                aScale = theFont->mScale * ((double)theFont->mPointSize / (double)aFontLayer->mPointSize);

            int aX;
            int aY;
            int aAdvance;
            int aKern = 0;
            if (aScale == 1.0)
            {
                aX = aFontLayer->mOffset.mX + aPen + (int)(short)aGlyph->mOffsetX;
                aY = (int)(short)aGlyph->mOffsetY + aFontLayer->mOffset.mY - aFontLayer->mAscent;
                aAdvance = (int)(short)aGlyph->mAdvance;
                if (aNext != 0)
                {
                    aKern = aFontLayer->mSpacing;
                    if (aGlyph->mKernCount != 0)
                    {
                        Sexy::FontLayer::KerningValue* aKerning =
                            aFontLayer->mKerningValues.data() + aGlyph->mKernIndex;
                        for (unsigned short aPair = 0; aPair < aGlyph->mKernCount; ++aPair)
                        {
                            if (aKerning[aPair].mSecondChar == aNext)
                            {
                                aKern += aKerning[aPair].mAmount;
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                aX = aPen + (int)std::floor(aScale * (double)(aFontLayer->mOffset.mX + (int)(short)aGlyph->mOffsetX));
                aY = -(int)std::floor(aScale * (double)(aFontLayer->mAscent - aFontLayer->mOffset.mY - (int)(short)aGlyph->mOffsetY));
                aAdvance = (int)(aScale * (double)(short)aGlyph->mAdvance);
                if (aNext != 0)
                {
                    aKern = aFontLayer->mSpacing;
                    if (aGlyph->mKernCount != 0)
                    {
                        Sexy::FontLayer::KerningValue* aKerning =
                            aFontLayer->mKerningValues.data() + aGlyph->mKernIndex;
                        for (unsigned short aPair = 0; aPair < aGlyph->mKernCount; ++aPair)
                        {
                            if (aKerning[aPair].mSecondChar == aNext)
                                aKern += (int)((double)aKerning[aPair].mAmount * aScale);
                        }
                    }
                }
            }

            Sexy::Color aDrawn;
            aDrawn.mRed = ClampChannel(MulDiv255(aFontLayer->mColorMult.mRed, theColor.mRed) + aFontLayer->mColorAdd.mRed);
            aDrawn.mGreen = ClampChannel(MulDiv255(aFontLayer->mColorMult.mGreen, theColor.mGreen) + aFontLayer->mColorAdd.mGreen);
            aDrawn.mBlue = ClampChannel(MulDiv255(aFontLayer->mColorMult.mBlue, theColor.mBlue) + aFontLayer->mColorAdd.mBlue);
            aDrawn.mAlpha = ClampChannel(MulDiv255(aFontLayer->mColorMult.mAlpha, theColor.mAlpha) + aFontLayer->mColorAdd.mAlpha);

            int aOrder = (short)aGlyph->mPageHeight + aFontLayer->mBaseOrder + 128;
            if (aOrder < 0)
                aOrder = 0;
            if (aOrder > 255)
                aOrder = 255;

            MatrixGlyph& aRecord = aRecords[aRecordCount];
            aRecord.mImage = aLayer->mImage;
            aRecord.mX = aX;
            aRecord.mY = aY;
            aRecord.mSrc = LayerSourceRect(aLayer, aChar);
            aRecord.mDrawMode = aFontLayer->mDrawMode;
            aRecord.mColor = aDrawn;
            aRecord.mNext = nullptr;
            if (aTails[aOrder] != nullptr)
                aTails[aOrder]->mNext = &aRecord;
            else
                aHeads[aOrder] = &aRecord;
            aTails[aOrder] = &aRecord;
            ++aRecordCount;

            int aExtent = aPen + aAdvance + aKern;
            if (aPenMax < aExtent)
                aPenMax = aExtent;
        }
        aPen = aPenMax;
    }

    for (int aBucket = 0; aBucket < 256; ++aBucket)
    {
        for (MatrixGlyph* aRecord = aHeads[aBucket]; aRecord != nullptr; aRecord = aRecord->mNext)
        {
            if (aRecord->mImage == nullptr)
                continue;
            int aMode = aRecord->mDrawMode;
            if (aMode == -1)
                aMode = (int)theGraphics->mDrawMode;
            Sexy::SexyMatrix3 aPlaced;
            aPlaced.LoadIdentity();
            SexyMatrix3Translation(&aPlaced,
                                   (float)aRecord->mX + (float)aRecord->mSrc.mWidth * 0.5f,
                                   (float)aRecord->mY + (float)aRecord->mSrc.mHeight * 0.5f);
            Sexy::SexyMatrix3 aResult;
            SexyMatrix3Multiply(&aResult, theMatrix, aPlaced);
            TodBltMatrix(theGraphics, aRecord->mImage, aResult, theGraphics->mClipRect,
                         aRecord->mColor, aMode, aRecord->mSrc);
        }
    }
}

void EmitLine(Sexy::Graphics* theGraphics, const std::u32string& theWide,
              const Sexy::TRect<int>& theRect, const DrawStyle& theStyle,
              int theLineY, int theStep, int theJustification, bool theDraw,
              bool theDrawOffscreen, int theStart, int theCount)
{
    if (theCount <= 0)
        return;
    int aBaseY = theRect.mY + theLineY;
    float aScreenY = theGraphics->mTransY + (float)aBaseY;
    bool aVisible = aScreenY >= (float)theGraphics->mClipRect.mY
        && aScreenY < (float)(theGraphics->mClipRect.mY + theGraphics->mClipRect.mHeight) + (float)theStep;
    if (aVisible || theDrawOffscreen)
    {
        DrawLine(theGraphics, theWide, theRect.mX, aBaseY, theStyle, theRect.mWidth,
                 theJustification, theDraw, theStart, theCount);
    }
}

int WrappedHeight(Sexy::Graphics* theGraphics, const std::u32string& theWide,
                  const Sexy::TRect<int>& theRect, Sexy::Font* theFont, Sexy::Color theColor,
                  int theJustification, bool theDraw, bool theDrawOffscreen)
{
    DrawStyle aStyle;
    aStyle.mFont = theFont;
    aStyle.mColor = theColor;
    aStyle.mExtra = 0;
    aStyle.mFlags = 0;

    int aLineY = theFont->GetAscent() - theFont->GetAscentPadding();
    int aStep = theFont->GetLineSpacing();
    int aIndex = 0;
    int aPrev = 0;
    int aBreak = -1;
    int aLineFrom = 0;
    int aWidth = 0;
    int aLength = (int)theWide.size();
    char32_t aLast = 0;

    while (aIndex < aLength)
    {
        char32_t aChar = theWide[aIndex];
        if (aChar == U'{')
        {
            int aClose = aIndex + 1;
            while (aClose < aLength && theWide[aClose] != U'}')
                ++aClose;
            if (aClose < aLength)
            {
                int aBefore = aStyle.mFont->GetAscent() - aStyle.mFont->GetAscentPadding();
                ApplyFormatTag(aStyle, theWide.substr(aIndex + 1, aClose - aIndex - 1));
                int aAfter = aStyle.mFont->GetAscent() - aStyle.mFont->GetAscentPadding();
                aLineY += aAfter - aBefore;
                aStep = aStyle.mFont->GetLineSpacing() + aStyle.mExtra;
                aIndex = aClose + 1;
                continue;
            }
        }

        bool aSoft = (aStyle.mFlags & 1) != 0;
        bool aForced = false;
        if (aChar == U' ' || (aSoft && aChar == U'\n'))
        {
            aBreak = aIndex;
            aChar = U' ';
        }
        else if (!aSoft && aChar == U'\n')
        {
            aBreak = aIndex;
            ++aIndex;
            aWidth = theRect.mWidth + 1;
            aForced = true;
        }

        aWidth += aStyle.mFont->CharWidthKern((int)aChar, aPrev);
        if (aWidth <= theRect.mWidth)
        {
            aPrev = (int)aChar;
            aLast = aChar;
            if (!aForced)
                ++aIndex;
            continue;
        }

        bool aWordBreak = aBreak >= aLineFrom && aChar <= 255 && aPrev <= 255;
        int aLineEnd = aWordBreak ? aBreak : aIndex;
        if (!aWordBreak && aLineEnd <= aLineFrom)
            aLineEnd = aLineFrom + 1;
        if (aLineEnd > aLength)
            aLineEnd = aLength;
        EmitLine(theGraphics, theWide, theRect, aStyle, aLineY, aStep, theJustification,
                 theDraw, theDrawOffscreen, aLineFrom, aLineEnd - aLineFrom);

        int aResume = aWordBreak ? aBreak + 1 : aLineEnd;
        while (aResume < aLength)
        {
            char32_t aSkip = theWide[aResume];
            if (aSkip != U' ' && !(aSoft && aSkip == U'\n'))
                break;
            ++aResume;
        }
        aLineFrom = aResume;
        aIndex = aResume;
        aPrev = 0;
        aBreak = -1;
        aWidth = 0;
        aLineY += aStep;
        aLast = 0;
    }

    if (aLength > aLineFrom)
    {
        EmitLine(theGraphics, theWide, theRect, aStyle, aLineY, aStep, theJustification,
                 theDraw, theDrawOffscreen, aLineFrom, aLength - aLineFrom);
        aLineY += aStep;
    }
    else if (aLast == U'\n')
    {
        aLineY += aStep;
    }
    return aStyle.mFont->GetDescent() + aLineY - aStep;
}

} // namespace

void TodDrawString(Sexy::Graphics* theGraphics, const Sexy::String& theText,
                   int theX, int theY, Sexy::Font* theFont, Sexy::Color theColor,
                   int theJustification)
{
    Sexy::String aText = PrepareText(theText);
    int aX = ShiftX(theFont, DecodeText(aText), theX, theJustification);
    theFont->DrawString(theGraphics, aX, theY, aText, theColor, theGraphics->mClipRect);
}

void TodDrawStringMatrix(Sexy::Graphics* theGraphics, Sexy::Font* theFont,
                         const Sexy::SexyMatrix3& theMatrix, const Sexy::String& theText,
                         const Sexy::Color& theColor)
{
    std::u32string aWide = DecodeText(TodStringTranslate(theText));
    DrawStringOnMatrix(theGraphics, static_cast<Sexy::ImageFont*>(theFont), theMatrix, aWide, theColor);
}

int TodDrawStringWrappedHelper(Sexy::Graphics* theGraphics, const Sexy::String& theText,
                               const Sexy::TRect<int>& theRect, Sexy::Font* theFont,
                               Sexy::Color theColor, int theJustification,
                               bool theDraw, bool theDrawOffscreen)
{
    return WrappedHeight(theGraphics, DecodeText(theText), theRect, theFont, theColor,
                         theJustification, theDraw, theDrawOffscreen);
}

int TodDrawStringWrapped(Sexy::Graphics* theGraphics, const Sexy::String& theText,
                         const Sexy::TRect<int>& theRect, Sexy::Font* theFont,
                         const Sexy::Color& theColor, int theJustification, bool theDraw)
{
    std::u32string aWide = DecodeText(PrepareText(theText));
    Sexy::TRect<int> aRect = theRect;
    if ((unsigned)(theJustification - 3) <= 2u)
    {
        int aMeasured = WrappedHeight(theGraphics, aWide, aRect, theFont, theColor,
                                      theJustification, false, theDraw);
        aRect.mY += (theRect.mHeight - aMeasured) / 2;
    }
    return WrappedHeight(theGraphics, aWide, aRect, theFont, theColor,
                         theJustification, true, theDraw);
}

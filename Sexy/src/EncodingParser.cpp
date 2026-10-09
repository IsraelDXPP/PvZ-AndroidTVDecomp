#include "EncodingParser.h"
#include <cstdio>
#include <functional>

namespace Sexy {
int SexyUtf8Validate(void* theState, char const* theText, int theLength);
}

// Lead-byte masks for UTF-8. The index that matches is the sequence length
// minus one, and the mask isolates the payload bits.
//   0  0xC0  continuation byte, length 1
//   1  0xE0  lead 0xC0..0xDF, length 2
//   2  0xF0  lead 0xE0..0xEF, length 3
//   3  0xF8  lead 0xF0..0xF7, length 4
//   4  0xFC  lead 0xF8..0xFB, length 5
static const unsigned short kUtf8LeadMasks[] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

int Sexy::EncodingParser::SetEncodingType(int theEncoding)
{
    switch (theEncoding)
    {
    case 0: mGetCharFunc = &EncodingParser::GetAsciiChar; break;
    case 1: mGetCharFunc = &EncodingParser::GetUTF8Char; break;
    case 2: mGetCharFunc = &EncodingParser::GetUTF16Char; break;
    case 3: mGetCharFunc = &EncodingParser::GetUTF16LEChar; break;
    case 4: mGetCharFunc = &EncodingParser::GetUTF16BEChar; break;
    default: return 0;
    }

    mGetCharFuncAdj = 0;
    mForcedEncodingType = true;
    return 0;
}

int Sexy::EncodingParser::CloseFile()
{
    if (mFile == nullptr)
        return 0;
    p_fclose(mFile);
    mFile = nullptr;
    return 1;
}

int Sexy::EncodingParser::GetAsciiChar(wchar_t* theChar, bool* theFirstChar)
{
    (void)theFirstChar;
    unsigned char aByte = 0;
    if (p_fread(&aByte, 1, 1, mFile) != 1)
        return 0;
    *theChar = aByte;
    return 1;
}

int Sexy::EncodingParser::GetUTF8Char(wchar_t* theChar, bool* theFirstChar)
{
    unsigned char aByte = 0;

    *theFirstChar = true;
    if (p_fread(&aByte, 1, 1, mFile) != 1)
    {
        *theFirstChar = false;
        return 0;
    }

    unsigned int aChar = aByte;
    if ((aByte & 0x80) != 0)
    {
        if ((aByte & 0xC0) != 0xC0)
            return 0;

        unsigned int aMask = 0;
        unsigned int aLength = 0;
        for (int aIndex = 0; aIndex < 5; ++aIndex)
        {
            const unsigned short aLeadMask = kUtf8LeadMasks[aIndex];
            if (((aByte ^ (aLeadMask << 1)) & aLeadMask) == 0)
            {
                aMask = aLeadMask;
                aLength = (unsigned int)aIndex + 1;
                break;
            }
        }
        if (aLength == 0)
            return 0;

        const unsigned char aLead = aByte;
        unsigned char aSecond = 0;
        aChar = aByte & ~aMask;

        for (unsigned int aIndex = 1; aIndex < aLength; ++aIndex)
        {
            if (p_fread(&aByte, 1, 1, mFile) != 1)
                return 0;

            aChar = (aByte & 0x3F) | (aChar << 6);
            if ((aByte & 0xC0) != 0x80)
                return 0;
            if (aIndex == 1)
                aSecond = aByte;
        }

        switch (aLength)
        {
        case 2:
            if ((aLead & 0x3E) == 0)
                return 0;
            break;
        case 3:
            if ((aLead & 0x1F) == 0 && (aSecond & 0x20) == 0)
                return 0;
            break;
        case 4:
            if ((aLead & 0x0F) == 0 && (aSecond & 0x30) == 0)
                return 0;
            break;
        case 5:
            if ((aLead & 0x07) == 0 && (aSecond & 0x38) == 0)
                return 0;
            break;
        default:
            break;
        }
    }

    if ((aChar >= 0xD800 && aChar <= 0xDFFF) || aChar == 0xFFFE || aChar == 0xFFFF)
        return 0;

    if (aChar == 0xFEFF && mFirstChar)
    {
        mFirstChar = false;
        return GetUTF8Char(theChar, theFirstChar);
    }

    *theChar = (wchar_t)aChar;
    *theFirstChar = false;
    return 1;
}

static int SwapUtf16(int theUnit)
{
    return ((theUnit & 0xFF) << 8) | ((theUnit >> 8) & 0xFF);
}

static int DecodeUtf16Pair(int theHigh, int theLow, wchar_t* theChar, bool* theFirstChar)
{
    if ((theHigh & 0xFC00) != 0xD800)
    {
        *theChar = (wchar_t)theHigh;
        *theFirstChar = false;
        return 1;
    }

    *theFirstChar = true;
    if ((theLow & 0xFC00) != 0xDC00)
        return 0;

    *theChar = (wchar_t)(((theLow & 0x3FF) | ((theHigh & 0x3FF) << 10)) + 0x10000);
    *theFirstChar = false;
    return 1;
}

int Sexy::EncodingParser::GetUTF16Char(wchar_t* theChar, bool* theFirstChar)
{
    int aUnit = 0;
    if (p_fread(&aUnit, 2, 1, mFile) != 1)
        return 0;

    if (mFirstChar)
    {
        mFirstChar = false;
        if (aUnit == 0xFEFF)
        {
            mByteSwap = false;
            return GetUTF16Char(theChar, theFirstChar);
        }
        if (aUnit == 0xFFFE)
        {
            mByteSwap = true;
            return GetUTF16Char(theChar, theFirstChar);
        }
    }

    if (mByteSwap)
        aUnit = SwapUtf16(aUnit);

    if ((aUnit & 0xFC00) != 0xD800)
    {
        *theChar = (wchar_t)aUnit;
        *theFirstChar = false;
        return 1;
    }

    int aLow = 0;
    *theFirstChar = true;
    if (p_fread(&aLow, 2, 1, mFile) != 1)
        return 0;
    if (mByteSwap)
        aLow = SwapUtf16(aLow);
    return DecodeUtf16Pair(aUnit, aLow, theChar, theFirstChar);
}

int Sexy::EncodingParser::GetUTF16LEChar(wchar_t* theChar, bool* theFirstChar)
{
    int aUnit = 0;
    if (p_fread(&aUnit, 2, 1, mFile) != 1)
        return 0;

    if ((aUnit & 0xFC00) != 0xD800)
    {
        *theChar = (wchar_t)aUnit;
        *theFirstChar = false;
        return 1;
    }

    int aLow = 0;
    *theFirstChar = true;
    if (p_fread(&aLow, 2, 1, mFile) != 1)
        return 0;
    return DecodeUtf16Pair(aUnit, aLow, theChar, theFirstChar);
}

int Sexy::EncodingParser::GetUTF16BEChar(wchar_t* theChar, bool* theFirstChar)
{
    int aUnit = 0;
    if (p_fread(&aUnit, 2, 1, mFile) != 1)
        return 0;
    aUnit = SwapUtf16(aUnit);

    if ((aUnit & 0xFC00) != 0xD800)
    {
        *theChar = (wchar_t)aUnit;
        *theFirstChar = false;
        return 1;
    }

    int aLow = 0;
    *theFirstChar = true;
    if (p_fread(&aLow, 2, 1, mFile) != 1)
        return 0;
    aLow = SwapUtf16(aLow);
    return DecodeUtf16Pair(aUnit, aLow, theChar, theFirstChar);
}

int Sexy::EncodingParser::OpenFile(Sexy::String const& thePath)
{
    CloseFile();
    SDL_IOStream* aFile = p_fopen(thePath.c_str(), "rb");
    mFile = aFile;
    if (aFile == nullptr)
        return 0;

    if (mForcedEncodingType)
    {
        mFirstChar = true;
        return 1;
    }

    char aBuffer[0x2000]{};
    const int aRead = static_cast<int>(p_fread(aBuffer, 1, sizeof(aBuffer), mFile));
    if (p_fseek(mFile, 0, SEEK_SET) != 0)
    {
        CloseFile();
        return 0;
    }
    mGetCharFuncAdj = 0;
    mGetCharFunc = &EncodingParser::GetAsciiChar;

    const bool aBigEndianBom = aRead >= 2 && aBuffer[0] == (char)0xFE && aBuffer[1] == (char)0xFF;
    const bool aLittleEndianBom = aRead >= 2 && aBuffer[0] == (char)0xFF && aBuffer[1] == (char)0xFE;
    if (aBigEndianBom || aLittleEndianBom)
    {
        mGetCharFunc = &EncodingParser::GetUTF16Char;
        mFirstChar = true;
        return 1;
    }

    if (aRead <= 1)
    {
        mFirstChar = true;
        return 1;
    }

    if (aRead > 2)
    {
        if (aBuffer[0] == (char)0xEF && aBuffer[1] == (char)0xBB && aBuffer[2] == (char)0xBF)
        {
            mGetCharFunc = &EncodingParser::GetUTF8Char;
        }
        else
        {
            // LawnStrings has no BOM. Validate the sample as UTF-8 so a
            // Chinese value is one character, not three Latin-1 bytes.
            for (int aTrim = 0; aTrim < 6 && aTrim < aRead; ++aTrim)
            {
                if (SexyUtf8Validate(nullptr, aBuffer, aRead - aTrim) != 0)
                {
                    mGetCharFuncAdj = 0;
                    mGetCharFunc = &EncodingParser::GetUTF8Char;
                    break;
                }
            }
        }
    }

    mFirstChar = true;
    return 1;
}

bool Sexy::EncodingParser::EndOfFile()
{
    if (!mBufferedText.empty())
        return false;
    if (mFile != nullptr)
        return p_feof(mFile) != 0;
    return true;
}

int Sexy::EncodingParser::GetChar(wchar_t* theChar)
{
    if (theChar == nullptr)
        return 3;

    if (!mBufferedText.empty())
    {
        *theChar = mBufferedText.front();
        mBufferedText.erase(mBufferedText.begin());
        return 0;
    }

    if (mFile == nullptr || p_feof(mFile) != 0)
        return 2;

    bool aFirstChar = false;
    if (std::invoke(mGetCharFunc, this, theChar, &aFirstChar) == 0)
        return aFirstChar ? 1 : 2;
    return 0;
}

Sexy::EncodingParser::EncodingParser()
    : mFile(nullptr),
      mGetCharFunc(&EncodingParser::GetUTF8Char),
      mGetCharFuncAdj(0),
      mForcedEncodingType(false),
      mFirstChar(false),
      mByteSwap(false)
{
}

void Sexy::EncodingParser::SetStringSource(Sexy::String const& theSource)
{
    Sexy::String aWideText;
    StringToWString(&aWideText, theSource.c_str());

    mBufferedText.clear();
    for (unsigned int aIndex = 0; aIndex < aWideText.size(); ++aIndex)
        mBufferedText.push_back((wchar_t)(unsigned char)aWideText[aIndex]);
}

int Sexy::EncodingParser::PutChar(wchar_t const& theChar)
{
    mBufferedText.insert(mBufferedText.begin(), theChar);
    return 1;
}

int Sexy::EncodingParser::PutString(Sexy::String const& theText)
{
    for (unsigned int aIndex = 0; aIndex < theText.size(); ++aIndex)
        mBufferedText.push_back((wchar_t)(unsigned char)theText[aIndex]);
    return 1;
}

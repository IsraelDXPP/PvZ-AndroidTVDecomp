#include "Buffer.h"
#include "ReconData.h"
#include "SexyTransform2D.h"
#include <cstdio>
#include <cstring>

namespace Sexy
{

namespace
{

const char kWebAlphabet[] = ".-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int WebDigit(unsigned char theChar)
{
    if (theChar < 128)
        return gWebStringBits[theChar];
    return -1;
}

int ByteIndex(int theBitPos)
{
    if (theBitPos == 0)
        return 0;
    int anAdjusted = theBitPos + 7;
    if (anAdjusted < 0)
        anAdjusted = theBitPos + 14;
    return anAdjusted >> 3;
}

int AlignUp8(int theBitPos)
{
    int anAdjusted = theBitPos + 7;
    if (anAdjusted < 0)
        anAdjusted = theBitPos + 14;
    return anAdjusted & ~7;
}

unsigned int Crc32Update(unsigned int theSeed, const unsigned char *theBytes, int theCount)
{
    if (!gCrc32Ready)
    {
        gCrc32Ready = true;
        for (int anIndex = 0; anIndex < 256; ++anIndex)
        {
            int anAccum = anIndex << 24;
            for (int aStep = 0; aStep < 8; ++aStep)
            {
                if (anAccum >= 0)
                    anAccum *= 2;
                else
                    anAccum = (2 * anAccum) ^ 0x4C11DB7;
            }
            gCrc32Table[anIndex] = (unsigned int)anAccum;
        }
    }
    unsigned int aCrc = theSeed;
    for (int anIndex = 0; anIndex < theCount; ++anIndex)
        aCrc = gCrc32Table[(theBytes[anIndex] ^ (aCrc >> 24)) & 0xFF] ^ (aCrc << 8);
    return aCrc;
}

int DecodeUtf8(const char **theCursor, int theLen, wchar_t *theChar)
{
    static const unsigned short kMask[] = {0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
    if (theLen == 0)
        return 0;

    const char *aCursor = *theCursor;
    int aCode = (unsigned char)*aCursor++;
    int aBytes[6];
    int aTotal = 1;
    if ((aCode & 0x80) != 0)
    {
        if ((aCode & 0xC0) != 0xC0)
            return 0;
        aBytes[0] = aCode;
        int aExtra = 0;
        for (; aExtra < 5; ++aExtra)
        {
            if ((aCode & kMask[aExtra]) == ((kMask[aExtra] << 1) & kMask[aExtra]))
                break;
        }
        if (aExtra >= 5)
            return 0;
        aCode &= ~kMask[aExtra];
        aTotal = aExtra + 1;
        if (aTotal < 2 || aTotal > 6)
            return 0;
        while (aExtra > 0 && (aCursor - *theCursor) < theLen)
        {
            int aNext = (unsigned char)*aCursor++;
            if ((aNext & 0xC0) != 0x80)
                return 0;
            aBytes[aTotal - aExtra] = aNext;
            aCode = (aCode << 6) | (aNext & 0x3F);
            --aExtra;
        }
        if (aExtra > 0)
            return 0;
        bool aValid = true;
        if (aTotal == 2)
            aValid = (aBytes[0] & 0x3E) != 0;
        else if (aTotal == 3)
            aValid = !((aBytes[0] & 0x1F) == 0 && (aBytes[1] & 0x20) == 0);
        else if (aTotal == 4)
            aValid = !((aBytes[0] & 0x0F) == 0 && (aBytes[1] & 0x30) == 0);
        else if (aTotal == 5)
            aValid = !((aBytes[0] & 0x07) == 0 && (aBytes[1] & 0x38) == 0);
        else if (aTotal == 6)
            aValid = !((aBytes[0] & 0x03) == 0 && (aBytes[1] & 0x3C) == 0);
        if (!aValid)
            return 0;
    }

    int aConsumed = (int)(aCursor - *theCursor);
    if ((aCode >= 0xD800 && aCode <= 0xDFFF) || (aCode >= 0xFFFE && aCode <= 0xFFFF))
        return 0;
    *theChar = (wchar_t)aCode;
    *theCursor = aCursor;
    return aConsumed;
}

void AppendWide(std::wstring &theText, wchar_t theChar)
{
    theText.push_back(theChar);
}

} // namespace

Buffer::Buffer() :
    mData(),
    mDataBitSize(0),
    mReadBitPos(0),
    mWriteBitPos(0)
{
}

Buffer::~Buffer() {}

void Buffer::Seek(SeekMode theMode, unsigned int theOffset) const
{
    switch (theMode)
    {
        case READ_START:
            mReadBitPos = 0;
            break;
        case READ_CURRENT:
            mReadBitPos = 8 * ((int)theOffset + ByteIndex(mReadBitPos));
            break;
        case READ_END:
            mReadBitPos = 8 * (ByteIndex(mReadBitPos) - (int)theOffset);
            break;
        case READ_DATA_END:
            mReadBitPos = AlignUp8(mDataBitSize);
            break;
        case WRITE_START:
            mWriteBitPos = 0;
            break;
        case WRITE_CURRENT:
            mWriteBitPos = 8 * ((int)theOffset + ByteIndex(mWriteBitPos));
            break;
        case WRITE_END:
            mWriteBitPos = 8 * (ByteIndex(mWriteBitPos) - (int)theOffset);
            break;
        case WRITE_DATA_END:
            mWriteBitPos = AlignUp8(mDataBitSize);
            break;
    }
}

Buffer *Buffer::Clear()
{
    mReadBitPos = 0;
    mWriteBitPos = 0;
    mData.clear();
    mDataBitSize = 0;
    return this;
}

int Buffer::GetBitsRequired(int theNum, bool theSigned)
{
    int aMagnitude = ~theNum;
    if (aMagnitude < theNum)
        aMagnitude = theNum;
    int aBits = 0;
    if (aMagnitude != 0)
    {
        while (aMagnitude >= (1 << aBits))
            ++aBits;
    }
    if (theSigned)
        ++aBits;
    return aBits;
}

unsigned char Buffer::ReadByte() const
{
    int aPos = mReadBitPos;
    int aByte = ByteIndex(aPos);
    if ((int)mData.size() <= aByte)
        return 0;
    int aCursor = aPos >= 0 ? aPos : ((aPos + 7) & (aPos >> 31));
    unsigned char aValue;
    if ((aPos & 7) != 0)
    {
        int aShift = aPos % 8;
        unsigned char aLow = mData[aCursor >> 3];
        unsigned char aHigh = mData[(aCursor >> 3) + 1];
        aValue = (unsigned char)((aHigh << (8 - aShift)) | (aLow >> aShift));
    }
    else
    {
        aValue = mData[aCursor >> 3];
    }
    mReadBitPos = aPos + 8;
    return aValue;
}

int Buffer::ReadNumBits(int theBitCount, bool theSigned) const
{
    if (theBitCount <= 0 || mData.empty())
        return 0;
    int aStart = mReadBitPos;
    int aByte = (aStart >= 0 ? aStart : ((aStart + 7) & (aStart >> 31))) >> 3;
    if ((int)mData.size() <= aByte)
        return 0;

    int aValue = 0;
    int aBit = 0;
    int aPos = aStart;
    do
    {
        unsigned int aCarry = (unsigned int)(aPos >> 31) >> 29;
        int aMask = 1 << (aPos - aStart);
        int aWhich = ((aPos + (int)aCarry) & 7) - (int)aCarry;
        aBit = (mData[aByte] >> aWhich) & 1;
        ++aPos;
        mReadBitPos = aPos;
        if (aBit != 0)
            aValue |= aMask;
        if (aPos == theBitCount + aStart)
            break;
        int aNext = aPos >= 0 ? aPos : ((aPos + 7) & (aPos >> 31));
        aByte = aNext >> 3;
    } while (aByte < (int)mData.size());

    if ((aBit & (theSigned ? 1 : 0)) != 0 && theBitCount <= 31)
    {
        int aSign = theBitCount;
        while (aSign != 32)
        {
            aValue |= 1 << aSign;
            ++aSign;
        }
    }
    return aValue;
}

Sexy::String Buffer::ToWebString() const
{
    int aSaved = mReadBitPos;
    int aBits = mWriteBitPos;
    mReadBitPos = 0;

    char aHeader[32];
    std::sprintf(aHeader, "%08X", aBits);
    Sexy::String aText(aHeader);

    int aCount = (aBits + 5) / 6;
    for (int anIndex = 0; anIndex < aCount; ++anIndex)
        aText.push_back(kWebAlphabet[ReadNumBits(6, false)]);

    mReadBitPos = aSaved;
    return aText;
}

bool Buffer::ReadBoolean() const
{
    return ReadByte() != 0;
}

short Buffer::ReadShort() const
{
    unsigned short aLow = ReadByte();
    unsigned short aHigh = ReadByte();
    return (short)(aLow | (unsigned short)(aHigh << 8));
}

long Buffer::ReadLong() const
{
    int aLow = ReadByte();
    int aMidLow = ReadByte();
    int aMidHigh = ReadByte();
    int aHigh = ReadByte();
    return (long)(aLow | (aMidLow << 8) | (aMidHigh << 16) | (aHigh << 24));
}

float Buffer::ReadFloat() const
{
    int aLow = ReadByte();
    int aMidLow = ReadByte();
    int aMidHigh = ReadByte();
    int aHigh = ReadByte();
    return FloatBits(aLow | (aMidLow << 8) | (aMidHigh << 16) | (aHigh << 24));
}

double Buffer::ReadDouble() const
{
    unsigned char aBytes[8];
    for (int anIndex = 0; anIndex < 8; ++anIndex)
        aBytes[anIndex] = ReadByte();
    double aValue;
    std::memcpy(&aValue, aBytes, sizeof(aValue));
    return aValue;
}

signed char Buffer::ReadInt8() const
{
    return (signed char)ReadByte();
}

short Buffer::ReadInt16() const
{
    return ReadShort();
}

int Buffer::ReadInt32() const
{
    return (int)ReadLong();
}

long long Buffer::ReadInt64() const
{
    unsigned char aBytes[8];
    for (int anIndex = 0; anIndex < 8; ++anIndex)
        aBytes[anIndex] = ReadByte();
    long long aValue;
    std::memcpy(&aValue, aBytes, sizeof(aValue));
    return aValue;
}

SexyTransform2D Buffer::ReadTransform2D() const
{
    SexyTransform2D aMatrix;
    aMatrix.m00 = ReadFloat();
    aMatrix.m01 = ReadFloat();
    aMatrix.m02 = ReadFloat();
    aMatrix.m10 = ReadFloat();
    aMatrix.m11 = ReadFloat();
    aMatrix.m12 = ReadFloat();
    aMatrix.m20 = ReadFloat();
    aMatrix.m21 = ReadFloat();
    aMatrix.m22 = ReadFloat();
    return aMatrix;
}

TPoint<float> Buffer::ReadFPoint() const
{
    TPoint<float> aPoint;
    aPoint.mX = (float)ReadDouble();
    aPoint.mY = (float)ReadDouble();
    return aPoint;
}

Sexy::String Buffer::ReadString() const
{
    Sexy::String aText;
    int aLength = ReadShort();
    for (int anIndex = 0; anIndex < aLength; ++anIndex)
        aText.push_back((char)ReadByte());
    return aText;
}

Sexy::String Buffer::ReadLine() const
{
    Sexy::String aText;
    for (;;)
    {
        int aByte = ReadByte();
        if (aByte == 0 || aByte == 10)
            break;
        if (aByte != 13)
            aText.push_back((char)aByte);
    }
    return aText;
}

void Buffer::ReadBytes(unsigned char *theDest, int theCount) const
{
    for (int anIndex = 0; anIndex < theCount; ++anIndex)
        theDest[anIndex] = ReadByte();
}

Sexy::String Buffer::ReadSexyString() const
{
    int aLength = ReadShort();
    Sexy::String aText;
    for (int anIndex = 0; anIndex < aLength; ++anIndex)
    {
        unsigned char aByte = 0;
        ReadBytes(&aByte, 1);
        aText.push_back((char)aByte);
    }
    return aText;
}

unsigned int Buffer::GetCRC32(unsigned int theSeed) const
{
    if (mData.empty())
        return theSeed;
    return Crc32Update(theSeed, &mData[0], (int)mData.size());
}

std::wstring Buffer::UTF8ToWideString() const
{
    if (mData.empty())
        return std::wstring();
    const char *aCursor = (const char *)&mData[0];
    int aLeft = ByteIndex(mDataBitSize);
    std::wstring aText;
    aText.reserve((size_t)(aLeft > 0 ? aLeft : 0));
    bool aFirst = true;
    while (aLeft > 0)
    {
        wchar_t aChar = 0;
        int aConsumed = DecodeUtf8(&aCursor, aLeft, &aChar);
        if (aConsumed == 0)
            break;
        aLeft -= aConsumed;
        if (aFirst)
        {
            aFirst = false;
            if (aChar == 0xFEFF)
                continue;
        }
        AppendWide(aText, aChar);
    }
    return aText;
}

std::wstring Buffer::ReadUTF8String() const
{
    if ((mReadBitPos & 7) != 0)
        mReadBitPos = (mReadBitPos + 8) & ~7;

    int aLength = ReadShort();
    if (aLength == 0)
        return std::wstring();

    int aCursor = mReadBitPos >= 0 ? mReadBitPos : (mReadBitPos + 7);
    int aRemain = mDataBitSize - mReadBitPos;
    if (aRemain < 0)
        aRemain += 7;
    int aLeft = aRemain >> 3;
    if (aLength <= 0 || aLeft <= 0 || mData.empty())
        return std::wstring();

    const char *aBytes = (const char *)&mData[0] + (aCursor >> 3);
    std::wstring aText;
    int aRead = 0;
    while (aLeft > 0 && aRead < aLength)
    {
        wchar_t aChar = 0;
        int aConsumed = DecodeUtf8(&aBytes, aLeft, &aChar);
        if (aConsumed == 0)
            break;
        aLeft -= aConsumed;
        mReadBitPos += 8 * aConsumed;
        AppendWide(aText, aChar);
        ++aRead;
    }
    return aText;
}

void Buffer::WriteNumBits(int theValue, int theBitCount)
{
    int aPos = mWriteBitPos;
    for (int aBit = 0; aBit < theBitCount; ++aBit)
    {
        if ((aPos & 7) == 0)
        {
            if ((int)mData.size() <= (aPos >> 3))
                mData.push_back(0);
            else
                mData[aPos >> 3] = 0;
        }
        if (((theValue >> aBit) & 1) != 0)
        {
            unsigned int aCarry = (unsigned int)(aPos >> 31) >> 29;
            int aIndex = (aPos >= 0 ? aPos : ((aPos + 7) & (aPos >> 31))) >> 3;
            int aShift = ((aPos + (int)aCarry) & 7) - (int)aCarry;
            if (aIndex >= 0 && aIndex < (int)mData.size())
                mData[aIndex] = (unsigned char)(mData[aIndex] | (1 << aShift));
        }
        mWriteBitPos = ++aPos;
    }
    if (mDataBitSize < aPos)
        mDataBitSize = aPos;
}

void Buffer::FromWebString(const Sexy::String &theString)
{
    Clear();
    if (theString.size() <= 3)
        return;

    int aShift = 28;
    int aBits = 0;
    for (int aDigit = 0; aDigit < 8; ++aDigit)
    {
        unsigned char aChar = (unsigned char)theString[(unsigned int)aDigit];
        int aNibble = (aChar - 48) << aShift;
        if ((unsigned char)(aChar - 48) > 9)
        {
            aNibble = (aChar - 55) << aShift;
            if ((unsigned char)(aChar - 65) > 5)
            {
                aNibble = (aChar - 92) << aShift;
                if ((unsigned char)(aChar - 97) > 5)
                    aNibble = 0;
            }
        }
        aShift -= 4;
        aBits += aNibble;
    }

    int aIndex = 8;
    while (aBits > 0)
    {
        unsigned char aChar = (unsigned char)theString[(unsigned int)aIndex];
        int aTake = aBits >= 6 ? 6 : aBits;
        aBits -= aTake;
        ++aIndex;
        WriteNumBits(WebDigit(aChar), aTake);
    }
    mReadBitPos = 0;
}

void Buffer::WriteByte(unsigned char theByte)
{
    int aPos = mWriteBitPos;
    if (aPos >= mDataBitSize)
    {
        if ((aPos & 7) != 0)
        {
            int aIndex = (aPos >= 0 ? aPos : ((aPos + 7) & (aPos >> 31))) >> 3;
            int aShift = aPos % 8;
            if (aIndex >= 0 && aIndex < (int)mData.size())
                mData[aIndex] = (unsigned char)(mData[aIndex] | (theByte << aShift));
            mData.push_back((unsigned char)(theByte >> (8 - aShift)));
        }
        else
        {
            mData.push_back(theByte);
        }
    }
    else if (!mData.empty())
    {
        unsigned int aIndex = 0;
        if (aPos != 0)
            aIndex = (unsigned int)ByteIndex(aPos);
        if (aIndex < mData.size() - 1)
        {
            if ((aPos & 7) != 0)
            {
                int aShift = aPos % 8;
                int aKeep = 8 - aShift;
                mData[aIndex] = (unsigned char)(mData[aIndex] | (theByte << aShift));
                if (aIndex + 1 < mData.size())
                    mData[aIndex + 1] = (unsigned char)(theByte >> aKeep);
                else
                    mData.push_back((unsigned char)(theByte >> aKeep));
            }
            else
            {
                mData[aIndex] = theByte;
            }
        }
    }

    int aNext = mWriteBitPos + 8;
    mWriteBitPos = aNext;
    if (aNext > mDataBitSize)
        mDataBitSize = aNext;
}

void Buffer::WriteBoolean(bool theValue)
{
    WriteByte(theValue ? 1 : 0);
}

void Buffer::WriteShort(short theValue)
{
    WriteByte((unsigned char)theValue);
    WriteByte((unsigned char)((unsigned short)theValue >> 8));
}

void Buffer::WriteLong(long theValue)
{
    unsigned int aBits = (unsigned int)theValue;
    WriteByte((unsigned char)aBits);
    WriteByte((unsigned char)(aBits >> 8));
    WriteByte((unsigned char)(aBits >> 16));
    WriteByte((unsigned char)(aBits >> 24));
}

void Buffer::WriteFloat(float theValue)
{
    WriteLong((long)FloatToBits(theValue));
}

void Buffer::WriteTransform2D(const SexyTransform2D &theMatrix)
{
    WriteFloat(theMatrix.m00);
    WriteFloat(theMatrix.m01);
    WriteFloat(theMatrix.m02);
    WriteFloat(theMatrix.m10);
    WriteFloat(theMatrix.m11);
    WriteFloat(theMatrix.m12);
    WriteFloat(theMatrix.m20);
    WriteFloat(theMatrix.m21);
    WriteFloat(theMatrix.m22);
}

void Buffer::WriteDouble(double theValue)
{
    unsigned char aBytes[8];
    std::memcpy(aBytes, &theValue, sizeof(aBytes));
    for (int anIndex = 0; anIndex < 8; ++anIndex)
        WriteByte(aBytes[anIndex]);
}

void Buffer::WriteFPoint(TPoint<float> thePoint)
{
    WriteDouble((double)thePoint.mX);
    WriteDouble((double)thePoint.mY);
}

void Buffer::WriteInt8(signed char theValue)
{
    WriteByte((unsigned char)theValue);
}

void Buffer::WriteInt16(short theValue)
{
    WriteShort(theValue);
}

void Buffer::WriteInt32(int theValue)
{
    WriteLong((long)theValue);
}

void Buffer::WriteInt64(long long theValue)
{
    unsigned char aBytes[8];
    std::memcpy(aBytes, &theValue, sizeof(aBytes));
    for (int anIndex = 0; anIndex < 8; ++anIndex)
        WriteByte(aBytes[anIndex]);
}

void Buffer::WriteString(const Sexy::String &theString)
{
    WriteShort((short)theString.size());
    for (unsigned int anIndex = 0; anIndex < theString.size(); ++anIndex)
        WriteByte((unsigned char)theString[anIndex]);
}

void Buffer::WriteUTF8String(const std::wstring &theString)
{
    if ((mWriteBitPos & 7) != 0)
        mWriteBitPos = (mWriteBitPos + 8) & ~7;
    WriteShort((short)theString.size());
    for (size_t anIndex = 0; anIndex < theString.size(); ++anIndex)
    {
        unsigned int aCode = (unsigned int)theString[anIndex];
        if (aCode < 0x80)
        {
            WriteByte((unsigned char)aCode);
        }
        else if (aCode < 0x800)
        {
            WriteByte((unsigned char)(0xC0 | (aCode >> 6)));
            WriteByte((unsigned char)(0x80 | (aCode & 0x3F)));
        }
        else if (aCode < 0x10000)
        {
            WriteByte((unsigned char)(0xE0 | (aCode >> 12)));
            WriteByte((unsigned char)(0x80 | ((aCode >> 6) & 0x3F)));
            WriteByte((unsigned char)(0x80 | (aCode & 0x3F)));
        }
        else if (aCode < 0x110000)
        {
            WriteByte((unsigned char)(0xF0 | (aCode >> 18)));
            WriteByte((unsigned char)(0x80 | ((aCode >> 12) & 0x3F)));
            WriteByte((unsigned char)(0x80 | ((aCode >> 6) & 0x3F)));
            WriteByte((unsigned char)(0x80 | (aCode & 0x3F)));
        }
    }
}

void Buffer::WriteBuffer(const std::vector<unsigned char> &theBytes)
{
    WriteLong((long)theBytes.size());
    for (size_t anIndex = 0; anIndex < theBytes.size(); ++anIndex)
        WriteByte(theBytes[anIndex]);
}

void Buffer::WriteBuffer(const Buffer &theBuffer)
{
    WriteBuffer(theBuffer.mData);
}

void Buffer::ReadBuffer(std::vector<unsigned char> *theBytes) const
{
    theBytes->clear();
    int aCount = (int)ReadLong();
    if (aCount <= 0)
        return;
    theBytes->resize((size_t)aCount);
    ReadBytes(&(*theBytes)[0], aCount);
}

void Buffer::ReadBuffer(Buffer *theBuffer) const
{
    ReadBuffer(&theBuffer->mData);
    theBuffer->mDataBitSize = 8 * (int)theBuffer->mData.size();
}

void Buffer::Resize(unsigned int theByteCount)
{
    Clear();
    mDataBitSize = 8 * (int)theByteCount;
    if (theByteCount > mData.size())
        mData.resize(theByteCount, 0);
}

void Buffer::WriteBytes(const unsigned char *theSrc, int theCount)
{
    if ((mWriteBitPos & 7) != 0)
    {
        for (int anIndex = 0; anIndex < theCount; ++anIndex)
            WriteByte(theSrc[anIndex]);
        return;
    }
    if (theCount > 0 && theSrc != nullptr)
        mData.insert(mData.end(), theSrc, theSrc + theCount);
    mWriteBitPos += 8 * theCount;
    if (mWriteBitPos > mDataBitSize)
        mDataBitSize = mWriteBitPos;
}

void Buffer::WriteSexyString(const Sexy::String &theString)
{
    WriteShort((short)theString.size());
    if (theString.size() > 0)
        WriteBytes((const unsigned char *)theString.c_str(), (int)theString.size());
}

void Buffer::WriteLine(const Sexy::String &theString)
{
    Sexy::String aLine = theString;
    aLine.Append("\r\n", 2);
    if (aLine.size() > 0)
        WriteBytes((const unsigned char *)aLine.c_str(), (int)aLine.size());
}

void Buffer::SetData(const std::vector<unsigned char> &theBytes)
{
    mData = theBytes;
    mDataBitSize = 8 * (int)mData.size();
}

void Buffer::SetData(const unsigned char *theSrc, int theCount)
{
    mData.clear();
    if (theCount > 0 && theSrc != nullptr)
        mData.insert(mData.end(), theSrc, theSrc + theCount);
    mDataBitSize = 8 * (int)mData.size();
}

const unsigned char *Buffer::GetDataPtr() const
{
    if (mData.empty())
        return nullptr;
    return &mData[0];
}

int Buffer::GetDataLen() const
{
    return ByteIndex(mDataBitSize);
}

int Buffer::GetDataLenBits() const
{
    return mDataBitSize;
}

bool Buffer::AtEnd() const
{
    return mReadBitPos >= mDataBitSize;
}

bool Buffer::PastEnd() const
{
    return mReadBitPos > mDataBitSize;
}

} // namespace Sexy

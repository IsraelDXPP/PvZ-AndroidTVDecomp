// Buffer is a bit cursor over a byte vector. The destructor is the only virtual.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_BUFFER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_BUFFER_H

#include "ReconTypes.h"
#include <cstring>

namespace Sexy
{

class SexyTransform2D;

class Buffer
{
public:
    virtual ~Buffer();

    std::vector<unsigned char> mData;
    int mDataBitSize;
    mutable int mReadBitPos;
    mutable int mWriteBitPos;

    // Read cursors are 0..3, write cursors are 4..7. CURRENT and END move by
    // whole bytes; START resets the cursor.
    enum SeekMode
    {
        READ_START = 0,
        READ_CURRENT = 1,
        READ_END = 2,
        READ_DATA_END = 3,
        WRITE_START = 4,
        WRITE_CURRENT = 5,
        WRITE_END = 6,
        WRITE_DATA_END = 7
    };

    Buffer();

    void Seek(SeekMode theMode, unsigned int theOffset) const;
    Buffer *Clear();
    static int GetBitsRequired(int theNum, bool theSigned);

    unsigned char ReadByte() const;
    int ReadNumBits(int theBitCount, bool theSigned) const;
    Sexy::String ToWebString() const;
    bool ReadBoolean() const;
    short ReadShort() const;
    long ReadLong() const;
    float ReadFloat() const;
    double ReadDouble() const;
    signed char ReadInt8() const;
    short ReadInt16() const;
    int ReadInt32() const;
    long long ReadInt64() const;
    SexyTransform2D ReadTransform2D() const;
    TPoint<float> ReadFPoint() const;
    Sexy::String ReadString() const;
    Sexy::String ReadLine() const;
    void ReadBytes(unsigned char *theDest, int theCount) const;
    Sexy::String ReadSexyString() const;
    unsigned int GetCRC32(unsigned int theSeed) const;
    std::wstring UTF8ToWideString() const;
    std::wstring ReadUTF8String() const;

    void WriteNumBits(int theValue, int theBitCount);
    void FromWebString(const Sexy::String &theString);
    void WriteByte(unsigned char theByte);
    void WriteBoolean(bool theValue);
    void WriteShort(short theValue);
    void WriteLong(long theValue);
    void WriteFloat(float theValue);
    void WriteTransform2D(const SexyTransform2D &theMatrix);
    void WriteDouble(double theValue);
    void WriteFPoint(TPoint<float> thePoint);
    void WriteInt8(signed char theValue);
    void WriteInt16(short theValue);
    void WriteInt32(int theValue);
    void WriteInt64(long long theValue);
    void WriteString(const Sexy::String &theString);
    void WriteUTF8String(const std::wstring &theString);
    void WriteBuffer(const std::vector<unsigned char> &theBytes);
    void WriteBuffer(const Buffer &theBuffer);
    void ReadBuffer(std::vector<unsigned char> *theBytes) const;
    void ReadBuffer(Buffer *theBuffer) const;
    void Resize(unsigned int theByteCount);
    void WriteBytes(const unsigned char *theSrc, int theCount);
    void WriteSexyString(const Sexy::String &theString);
    void WriteLine(const Sexy::String &theString);
    void SetData(const std::vector<unsigned char> &theBytes);
    void SetData(const unsigned char *theSrc, int theCount);

    const unsigned char *GetDataPtr() const;
    int GetDataLen() const;
    int GetDataLenBits() const;
    bool AtEnd() const;
    bool PastEnd() const;
};

} // namespace Sexy

using Sexy::Buffer;

inline float FloatBits(int theBits)
{
    float aValue;
    std::memcpy(&aValue, &theBits, sizeof(aValue));
    return aValue;
}

inline int FloatToBits(float theValue)
{
    int aBits;
    std::memcpy(&aBits, &theValue, sizeof(aBits));
    return aBits;
}

inline double DoubleBits(long long theBits)
{
    double aValue;
    std::memcpy(&aValue, &theBits, sizeof(aValue));
    return aValue;
}

#endif // LAWNPROJECTTV_SEXY_INCLUDE_BUFFER_H

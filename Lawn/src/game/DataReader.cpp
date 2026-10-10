#include "DataReader.h"
#include "DataReaderException.h"
#include <cstdio>
#include <cstring>

DataReader::DataReader()
{
    mFile = nullptr;
    mData = nullptr;
    mDataLen = 0;
    mDataPos = 0;
    mOwnData = false;
}

void DataReader::Close()
{
    if (mFile != nullptr)
    {
        SDL_CloseIO(mFile);
        mFile = nullptr;
    }
    if (mOwnData && mData != nullptr)
        operator delete[](mData);
    mData = nullptr;
    mDataLen = 0;
    mDataPos = 0;
    mOwnData = false;
}

DataReader::~DataReader()
{
    Close();
}

bool DataReader::OpenFile(const Sexy::String &theFileName)
{
    Close();
    mFile = SDL_IOFromFile(theFileName.c_str(), "rb");
    return mFile != nullptr;
}

void DataReader::OpenMemory(const void *theData, unsigned long theDataLen, bool takeOwnership)
{
    Close();
    mData = (char *)theData;
    mDataLen = theDataLen;
    mOwnData = takeOwnership;
}

int DataReader::Rewind(unsigned long theNumBytes)
{
    if (mDataPos < theNumBytes)
        throw DataReaderException();

    mDataPos -= theNumBytes;
    mData -= theNumBytes;
    return 1;
}

int DataReader::ReadBytes(void *theMem, unsigned long theNumBytes)
{
    if (mData != nullptr)
    {
        unsigned long newPos = mDataPos + theNumBytes;
        mDataPos = newPos;
        if (newPos > mDataLen)
            throw DataReaderException();

        memcpy(theMem, mData, theNumBytes);
        mData += theNumBytes;
        return (int)theNumBytes;
    }

    if (mFile == nullptr)
        throw DataReaderException();

    const size_t bytesRead = SDL_ReadIO(mFile, theMem, theNumBytes);
    if (theNumBytes != bytesRead)
        throw DataReaderException();
    return (int)bytesRead;
}

long long DataReader::ReadLongLong()
{
    long long value = 0;
    ReadBytes(&value, 8u);
    return value;
}

int DataReader::ReadLong()
{
    int value = 0;
    ReadBytes(&value, 4u);
    return value;
}

int DataReader::ReadShort()
{
    unsigned short value = 0;
    ReadBytes(&value, 2u);
    return value;
}

int DataReader::ReadByte()
{
    unsigned char value = 0;
    ReadBytes(&value, 1u);
    return value;
}

bool DataReader::ReadBool()
{
    return ReadByte() != 0;
}

// @0x189970 and @0x189984: return the stored floating-point value.
float DataReader::ReadFloat()
{
    float value = 0.0f;
    ReadBytes(&value, sizeof(value));
    return value;
}

double DataReader::ReadDouble()
{
    double value = 0.0;
    ReadBytes(&value, sizeof(value));
    return value;
}

int DataReader::ReadString(Sexy::String &theStr)
{
    unsigned int length = (unsigned int)ReadShort();
    StringResize(&theStr, length, 0);
    return ReadBytes(theStr.mData, length);
}

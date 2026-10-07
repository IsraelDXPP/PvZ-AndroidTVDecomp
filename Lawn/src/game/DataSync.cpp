// DataSync reads a field from the save stream, or writes the field that is
// already there. The reader is used when one is attached; otherwise the writer
// is used. The TV build keeps the reader or writer result so callers can see
// how many bytes moved.

#include "DataSync.h"
#include "DataReader.h"
#include "DataReaderException.h"
#include "DataWriter.h"
#include <cstring>

namespace {

template <class T>
int ReadInto(DataReader *theReader, T &theNum)
{
    theNum = static_cast<T>(theReader->ReadLongLong());
    return static_cast<int>(theNum);
}

template <class T>
int WriteWide(DataWriter *theWriter, T theNum)
{
    return theWriter->WriteLongLong(static_cast<unsigned long long>(theNum));
}

// A signed byte is widened through unsigned char, the same as the other narrow
// stores: the extra bits are the value's own, not a sign extension.
template <class T>
int WriteWideByte(DataWriter *theWriter, T theNum)
{
    return theWriter->WriteLongLong(static_cast<unsigned char>(theNum));
}

template <class T>
int ReadLongInto(DataReader *theReader, T &theNum)
{
    theNum = static_cast<T>(theReader->ReadLong());
    return static_cast<int>(theNum);
}

template <class T>
int WriteLongOf(DataWriter *theWriter, T theNum)
{
    return theWriter->WriteLong(static_cast<unsigned long>(theNum));
}

template <class T>
int WriteLongOfByte(DataWriter *theWriter, T theNum)
{
    return theWriter->WriteLong(static_cast<unsigned char>(theNum));
}

template <class T>
int ReadShortInto(DataReader *theReader, T &theNum)
{
    theNum = static_cast<T>(theReader->ReadShort());
    return static_cast<int>(theNum);
}

template <class T>
int WriteShortOf(DataWriter *theWriter, T theNum)
{
    return theWriter->WriteShort(static_cast<unsigned short>(theNum));
}

template <class T>
int WriteShortOfByte(DataWriter *theWriter, T theNum)
{
    return theWriter->WriteShort(static_cast<unsigned char>(theNum));
}

template <class T>
int ReadByteInto(DataReader *theReader, T &theNum)
{
    theNum = static_cast<T>(theReader->ReadByte());
    return static_cast<int>(theNum);
}

template <class T>
int WriteByteOf(DataWriter *theWriter, T theNum)
{
    return theWriter->WriteByte(static_cast<unsigned char>(theNum));
}

} // namespace

int DataSync::SyncBytes(void *theData, unsigned long theDataLen)
{
    if (mReader != nullptr)
        return mReader->ReadBytes(theData, theDataLen);
    return mWriter->WriteBytes(theData, theDataLen);
}

int DataSync::SyncLongLong(int &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

unsigned int DataSync::SyncLongLong(unsigned long &theNum)
{
    if (mReader == nullptr)
        return static_cast<unsigned int>(WriteWide(mWriter, theNum));
    theNum = static_cast<unsigned long>(mReader->ReadLongLong());
    return static_cast<unsigned int>(theNum);
}

int DataSync::SyncLongLong(unsigned short &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncLongLong(unsigned char &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncLongLong(long long &theNum)
{
    if (mReader != nullptr)
    {
        theNum = mReader->ReadLongLong();
        return static_cast<int>(theNum);
    }
    return mWriter->WriteLongLong(static_cast<unsigned long long>(theNum));
}

int DataSync::SyncLongLong(long &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncLongLong(short &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncLongLong(char &theNum)
{
    if (mReader == nullptr)
        return WriteWideByte(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncSLongLong(int &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

unsigned int DataSync::SyncSLongLong(unsigned long &theNum)
{
    if (mReader == nullptr)
        return static_cast<unsigned int>(WriteWide(mWriter, theNum));
    theNum = static_cast<unsigned long>(mReader->ReadLongLong());
    return static_cast<unsigned int>(theNum);
}

int DataSync::SyncSLongLong(unsigned short &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncSLongLong(unsigned char &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncSLongLong(long &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncSLongLong(short &theNum)
{
    if (mReader == nullptr)
        return WriteWide(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncSLongLong(char &theNum)
{
    if (mReader == nullptr)
        return WriteWideByte(mWriter, theNum);
    return ReadInto(mReader, theNum);
}

int DataSync::SyncLong(int &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncLong(unsigned long &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncLong(unsigned short &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncLong(unsigned char &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncLong(long long &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteLong(static_cast<unsigned long>(theNum));
    theNum = static_cast<unsigned long>(mReader->ReadLong());
    return static_cast<int>(theNum);
}

int DataSync::SyncLong(long &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncLong(short &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncLong(char &theNum)
{
    if (mReader == nullptr)
        return WriteLongOfByte(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncSLong(int &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncSLong(unsigned long &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncSLong(unsigned short &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncSLong(unsigned char &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncSLong(long &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncSLong(short &theNum)
{
    if (mReader == nullptr)
        return WriteLongOf(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncSLong(char &theNum)
{
    if (mReader == nullptr)
        return WriteLongOfByte(mWriter, theNum);
    return ReadLongInto(mReader, theNum);
}

int DataSync::SyncShort(int &theNum)
{
    if (mReader == nullptr)
        return WriteShortOf(mWriter, theNum);
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncShort(unsigned long &theNum)
{
    if (mReader == nullptr)
        return WriteShortOf(mWriter, theNum);
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncShort(unsigned short &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteShort(theNum);
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncShort(unsigned char &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteShort(theNum);
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncShort(long long &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteShort(static_cast<unsigned short>(theNum));
    theNum = static_cast<unsigned short>(mReader->ReadShort());
    return static_cast<int>(theNum);
}

int DataSync::SyncShort(long &theNum)
{
    if (mReader == nullptr)
        return WriteShortOf(mWriter, theNum);
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncShort(short &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteShort(static_cast<unsigned short>(theNum));
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncShort(char &theNum)
{
    if (mReader == nullptr)
        return WriteShortOfByte(mWriter, theNum);
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncSShort(int &theNum)
{
    if (mReader == nullptr)
        return WriteShortOf(mWriter, theNum);
    theNum = static_cast<short>(mReader->ReadShort());
    return static_cast<int>(theNum);
}

int DataSync::SyncSShort(unsigned long &theNum)
{
    if (mReader == nullptr)
        return WriteShortOf(mWriter, theNum);
    theNum = static_cast<short>(mReader->ReadShort());
    return static_cast<int>(theNum);
}

int DataSync::SyncSShort(unsigned short &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteShort(theNum);
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncSShort(unsigned char &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteShort(theNum);
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncSShort(long &theNum)
{
    if (mReader == nullptr)
        return WriteShortOf(mWriter, theNum);
    theNum = static_cast<short>(mReader->ReadShort());
    return static_cast<int>(theNum);
}

int DataSync::SyncSShort(short &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteShort(static_cast<unsigned short>(theNum));
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncSShort(char &theNum)
{
    if (mReader == nullptr)
        return WriteShortOfByte(mWriter, theNum);
    return ReadShortInto(mReader, theNum);
}

int DataSync::SyncByte(int &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncByte(unsigned long &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncByte(unsigned short &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncByte(unsigned char &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteByte(theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncByte(long long &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    theNum = static_cast<unsigned char>(mReader->ReadByte());
    return static_cast<int>(theNum);
}

int DataSync::SyncByte(long &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncByte(short &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncByte(char &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteByte(static_cast<unsigned char>(theNum));
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncSByte(int &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncSByte(unsigned long &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncSByte(unsigned short &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncSByte(unsigned char &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteByte(theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncSByte(long &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncSByte(short &theNum)
{
    if (mReader == nullptr)
        return WriteByteOf(mWriter, theNum);
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncSByte(char &theNum)
{
    if (mReader == nullptr)
        return mWriter->WriteByte(static_cast<unsigned char>(theNum));
    return ReadByteInto(mReader, theNum);
}

int DataSync::SyncBool(bool &theValue)
{
    if (mReader == nullptr)
        return mWriter->WriteBool(theValue);
    int theRead = mReader->ReadBool();
    theValue = theRead != 0;
    return theRead;
}

void DataSync::SyncFloat(float &theValue)
{
    if (mReader != nullptr)
        theValue = mReader->ReadFloat();
    else
        mWriter->WriteFloat(theValue);
}

void DataSync::SyncDouble(double &theValue)
{
    if (mReader != nullptr)
        theValue = mReader->ReadDouble();
    else
        mWriter->WriteDouble(theValue);
}

int DataSync::SyncString(Sexy::String &theString)
{
    if (mReader != nullptr)
        return mReader->ReadString(theString);
    return mWriter->WriteString(theString);
}

int DataSync::SyncPointer(void **theSlot)
{
    mPointerSyncList.push_back(theSlot);
    return 0;
}

int DataSync::ResetPointerTable()
{
    mIntToPointerMap.clear();
    mPointerToIntMap.clear();
    mPointerSyncList.clear();
    mCurPointerIndex = 1;
    // Index 0 is the null pointer, both ways.
    mPointerToIntMap.emplace(nullptr, 0);
    mIntToPointerMap.emplace(0, nullptr);
    return 0;
}

int DataSync::Reset()
{
    mReader = nullptr;
    mWriter = nullptr;
    return ResetPointerTable();
}

DataSync::DataSync()
{
    Reset();
}

DataSync::DataSync(DataReader &theReader)
{
    Reset();
    mReader = &theReader;
}

DataSync::DataSync(DataWriter &theWriter)
{
    Reset();
    mWriter = &theWriter;
}

int DataSync::SetReader(DataReader &theReader)
{
    int theResult = Reset();
    mReader = &theReader;
    return theResult;
}

int DataSync::SetWriter(DataWriter &theWriter)
{
    int theResult = Reset();
    mWriter = &theWriter;
    return theResult;
}

int DataSync::SyncPointers()
{
    if (mReader != nullptr)
    {
        for (void **theSlot : mPointerSyncList)
        {
            int theIndex = mReader->ReadLong();
            auto theFound = mIntToPointerMap.find(theIndex);
            if (theFound == mIntToPointerMap.end())
                throw DataReaderException();
            *theSlot = theFound->second;
        }
    }
    else
    {
        for (void **theSlot : mPointerSyncList)
        {
            void *thePointer = *theSlot;
            auto theFound = mPointerToIntMap.find(thePointer);
            // A pointer that was never registered has no index. Null's index is
            // 0, which is the only reserved miss the save format understands.
            int theIndex = theFound != mPointerToIntMap.end() ? theFound->second : 0;
            mWriter->WriteLong(static_cast<unsigned long>(theIndex));
        }
    }
    return ResetPointerTable();
}

int DataSync::RegisterPointer(void *thePointer)
{
    auto theExisting = mPointerToIntMap.find(thePointer);
    if (theExisting != mPointerToIntMap.end())
        return theExisting->second;

    int theIndex = mCurPointerIndex;
    ++mCurPointerIndex;
    mPointerToIntMap.emplace(thePointer, theIndex);
    mIntToPointerMap.emplace(theIndex, thePointer);
    return theIndex;
}

DataSync::~DataSync()
{
    mIntToPointerMap.clear();
    mPointerToIntMap.clear();
}

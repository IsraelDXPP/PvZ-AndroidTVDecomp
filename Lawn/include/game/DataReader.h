// DataReader -- binary reader used by DataSync.
//
// Polymorphic only so the destructor is virtual. The object holds either an
// open FILE or a byte buffer plus a cursor that walks through that buffer.

#pragma once

#include "ReconTypes.h"
#include <stdio.h>

class DataReader
{
public:
    virtual ~DataReader();

    FILE* mFile;
    char* mData;
    unsigned long mDataLen;
    unsigned long mDataPos;
    bool mOwnData;

    DataReader();
    void Close();
    bool OpenFile(const Sexy::String& theFileName);
    void OpenMemory(const void* theData, unsigned long theDataLen, bool takeOwnership);
    int Rewind(unsigned long theNumBytes);
    int ReadBytes(void* theMem, unsigned long theNumBytes);
    long long ReadLongLong();
    int ReadLong();
    int ReadShort();
    int ReadByte();
    bool ReadBool();
    float ReadFloat();
    double ReadDouble();
    int ReadString(Sexy::String& theStr);
};

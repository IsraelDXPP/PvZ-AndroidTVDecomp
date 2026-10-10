// DataReader -- binary reader used by DataSync.
//
// Polymorphic only so the destructor is virtual. The object holds either an
// open SDL stream or a byte buffer plus a cursor that walks through that buffer.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_DATAREADER_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_DATAREADER_H

#include "ReconTypes.h"
#include <SDL3/SDL_iostream.h>

class DataReader
{
public:
    virtual ~DataReader();

    SDL_IOStream *mFile;
    char *mData;
    unsigned long mDataLen;
    unsigned long mDataPos;
    bool mOwnData;

    DataReader();
    void Close();
    bool OpenFile(const Sexy::String &theFileName);
    void OpenMemory(const void *theData, unsigned long theDataLen, bool takeOwnership);
    int Rewind(unsigned long theNumBytes);
    int ReadBytes(void *theMem, unsigned long theNumBytes);
    long long ReadLongLong();
    int ReadLong();
    int ReadShort();
    int ReadByte();
    bool ReadBool();
    float ReadFloat();
    double ReadDouble();
    int ReadString(Sexy::String &theStr);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_DATAREADER_H

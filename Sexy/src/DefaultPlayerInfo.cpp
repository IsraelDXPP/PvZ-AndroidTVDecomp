#include "DefaultPlayerInfo.h"

#include "Buffer.h"
#include "DataReader.h"
#include "DataSync.h"
#include "DataWriter.h"
#include "PlayerInfo.h"
#include "SexyAppBase.h"

namespace
{
    // PlayerInfo stores the detail-file count in the word after the manager
    // pointer. The header still spells that word without a purpose name, so
    // the count is read through a view of the same layout.
    int DetailFileCount(const Sexy::PlayerInfo* theProfile)
    {
        return theProfile->mDetailFileCount;
    }
}

int Sexy::DefaultPlayerInfo::GetId()
{
    return mId;
}

int Sexy::DefaultPlayerInfo::GetProfileId()
{
    return mProfileId;
}

Sexy::String Sexy::DefaultPlayerInfo::GetName()
{
    return mName;
}

void Sexy::DefaultPlayerInfo::Reset()
{
}

void Sexy::DefaultPlayerInfo::SyncDetails(DataSync& theSync, int theVersion)
{
    (void)theSync;
    (void)theVersion;
}

void Sexy::DefaultPlayerInfo::SyncSummary(DataSync& theSync)
{
    theSync.SyncString(mName);
    theSync.SyncLong(mUseSeq);
    theSync.SyncLong(mId);
    theSync.SyncLong(mProfileId);
}

void Sexy::DefaultPlayerInfo::DeleteUserFiles()
{
    for (int theSlot = 0; theSlot < DetailFileCount(this); ++theSlot)
    {
        Sexy::String thePath;
        StrFormat(&thePath, "userdata/user%d-%d-%d.dat", mProfileId, GetId(), theSlot);

        Sexy::String theFolder;
        GetAppDataFolder(&theFolder);
        theFolder += thePath;

        ((SexyAppBase*)gSexyAppBase)->EraseFile(theFolder);
    }
}

Sexy::DefaultPlayerInfo::DefaultPlayerInfo(Sexy::ProfileMgr* theProfileMgr)
    : PlayerInfo(theProfileMgr)
{
    mId = -1;
    mProfileId = -1;
}

void Sexy::DefaultPlayerInfo::LoadDetails()
{
    if (DetailFileCount(this) <= 0)
        return;

    for (int theSlot = 0; theSlot < DetailFileCount(this); ++theSlot)
    {
        Sexy::Buffer theBuffer;

        Sexy::String theFolder;
        GetAppDataFolder(&theFolder);
        Sexy::String theFile;
        StrFormat(&theFile, "userdata/user%d-%d-%d.dat", mProfileId, GetId(), theSlot);
        theFolder += theFile;

        if (((SexyAppBase*)gSexyAppBase)->ReadBufferFromFile(theFolder, &theBuffer, false) == 0)
            return;

        DataReader theReader;
        const unsigned char* theBits = theBuffer.mData.empty() ? nullptr : &theBuffer.mData[0];
        theReader.OpenMemory(theBits, (unsigned long)((theBuffer.mDataBitSize + 7) / 8), false);

        DataSync theSync(theReader);
        SyncDetails(theSync, theSlot);
    }
}

bool Sexy::DefaultPlayerInfo::SaveDetails()
{
    if (DetailFileCount(this) <= 0)
        return true;

    bool theSaved = true;
    for (int theSlot = 0; theSlot < DetailFileCount(this); ++theSlot)
    {
        DataWriter theWriter;
        theWriter.OpenMemory(32);
        DataSync theSync(theWriter);
        SyncDetails(theSync, theSlot);

        Sexy::String theFolder;
        GetAppDataFolder(&theFolder);
        theFolder += "userdata";
        MkDir(theFolder);

        Sexy::String theFile;
        GetAppDataFolder(&theFile);
        Sexy::String theName;
        StrFormat(&theName, "userdata/user%d-%d-%d.dat", mProfileId, GetId(), theSlot);
        theFile += theName;

        if (((SexyAppBase*)gSexyAppBase)->WriteBytesToFile(
                theFile, theWriter.mBuffer, theWriter.mBufferPos) == 0)
            theSaved = false;
    }
    return theSaved;
}

Sexy::DefaultPlayerInfo::~DefaultPlayerInfo()
{
}

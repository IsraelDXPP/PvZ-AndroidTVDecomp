#include "DefaultPlayerInfo.h"

#include "Buffer.h"
#include "DataReader.h"
#include "DataSync.h"
#include "DataWriter.h"
#include "PlayerInfo.h"
#include "SexyAppBase.h"

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
    for (int theSlot = 0; theSlot < mDetailFileCount; ++theSlot)
    {
        Sexy::String thePath;
        StrFormat(&thePath, "userdata/user%d-%d-%d.dat", mProfileId, GetId(), theSlot);

        Sexy::String theFolder;
        GetAppDataFolder(&theFolder);
        theFolder += thePath;

        gSexyAppBase->EraseFile(theFolder);
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
    if (mDetailFileCount <= 0)
        return;

    for (int theSlot = 0; theSlot < mDetailFileCount; ++theSlot)
    {
        Sexy::Buffer theBuffer;

        Sexy::String theFolder;
        GetAppDataFolder(&theFolder);
        Sexy::String theFile;
        StrFormat(&theFile, "userdata/user%d-%d-%d.dat", mProfileId, GetId(), theSlot);
        theFolder += theFile;

        if (gSexyAppBase->ReadBufferFromFile(theFolder, &theBuffer, false) == 0)
            return;

        DataReader theReader;
        const unsigned char* theBits = theBuffer.mData.empty() ? nullptr : &theBuffer.mData[0];
        theReader.OpenMemory(theBits, (unsigned long)((theBuffer.mDataBitSize + 7) / 8), false);

        DataSync theSync(theReader);
        SyncDetails(theSync, theSlot);
        if (theSync.mVersionMismatch)
        {
            // Preserve the profile identity, reset every detail slot, and replace
            // the files together instead of mixing old garden/statistics data in.
            Reset();
            DeleteUserFiles();
            if (!SaveDetails())
                OutputDebug("Failed to replace incompatible profile %d-%d\n", mProfileId, GetId());
            return;
        }
    }
}

bool Sexy::DefaultPlayerInfo::SaveDetails()
{
    if (mDetailFileCount <= 0)
        return true;

    bool theSaved = true;
    for (int theSlot = 0; theSlot < mDetailFileCount; ++theSlot)
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

        if (gSexyAppBase->WriteBytesToFile(
                theFile, theWriter.mBuffer, theWriter.mBufferPos) == 0)
            theSaved = false;
    }
    return theSaved;
}

Sexy::DefaultPlayerInfo::~DefaultPlayerInfo()
{
}

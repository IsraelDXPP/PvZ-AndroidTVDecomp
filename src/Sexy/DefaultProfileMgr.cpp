#include "DefaultProfileMgr.h"

#include "Buffer.h"
#include "DataReader.h"
#include "DataSync.h"
#include "DataWriter.h"
#include "PlayerInfo.h"
#include "ProfileMgr.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"

#include <cstring>

namespace {

void AddRefProfile(Sexy::PlayerInfo* theProfile)
{
    if (theProfile == nullptr)
        return;
    ++theProfile->mRefCount;
}

void ReleaseProfile(Sexy::PlayerInfo* theProfile)
{
    if (theProfile == nullptr)
        return;
    int previous = theProfile->mRefCount--;
    if (previous <= 1)
        delete theProfile;
}

void ReleaseList(Sexy::DefaultProfileMgr::DefaultPlayerList& theList)
{
    for (int slot = 0; slot < 8; ++slot)
    {
        ReleaseProfile(theList.mProfiles[slot]);
        theList.mProfiles[slot] = nullptr;
    }
}

bool SameNameIgnoreCase(const Sexy::String& theLeft, const Sexy::String& theRight)
{
    return strcasecmp(theLeft.c_str(), theRight.c_str()) == 0;
}

} // namespace

int Sexy::DefaultProfileMgr::GetNumProfiles()
{
    return mNumProfiles;
}

Sexy::PlayerInfo* Sexy::DefaultProfileMgr::GetAnyProfile()
{
    if (mProfiles.empty())
        return nullptr;

    Sexy::PlayerInfo* profile = mProfiles.begin()->second.mProfiles[0];
    if (profile == nullptr)
        return nullptr;

    profile->LoadDetails();
    int useSeq = mNextProfileUseSeq;
    ++mNextProfileUseSeq;
    profile->mUseSeq = useSeq;
    return profile;
}

std::list<Sexy::PlayerInfo*> Sexy::DefaultProfileMgr::GetProfileList()
{
    std::list<Sexy::PlayerInfo*> result;
    for (auto& entry : mProfiles)
    {
        Sexy::PlayerInfo* profile = entry.second.mProfiles[0];
        if (profile != nullptr)
            result.push_back(profile);
    }
    return result;
}

int Sexy::DefaultProfileMgr::SetUseSeq(DefaultPlayerList& theList, unsigned int theUseSeq)
{
    for (int slot = 0; slot < 8; ++slot)
    {
        Sexy::PlayerInfo* profile = theList.mProfiles[slot];
        if (profile != nullptr)
            profile->mUseSeq = (int)theUseSeq;
    }
    return 0;
}

Sexy::DefaultProfileMgr::~DefaultProfileMgr()
{
    Clear();
}

int Sexy::DefaultProfileMgr::Clear()
{
    for (auto& entry : mProfiles)
        ReleaseList(entry.second);
    mProfiles.clear();
    mNextProfileUseSeq = 1;
    mNumProfiles = 0;
    mNextProfileId = 0;
    return 0;
}

Sexy::DefaultProfileMgr::DefaultProfileMgr(Sexy::ProfileEventListener* theListener)
    : Sexy::ProfileMgr(theListener)
    , mProfiles()
    , mNumProfiles(0)
    , mNextProfileId(0)
    , mNextProfileUseSeq(1)
{
    Clear();
}

Sexy::PlayerInfo* Sexy::DefaultProfileMgr::GetProfile(Sexy::String const& theName, int theSlot)
{
    auto found = mProfiles.find(theName);
    if (found == mProfiles.end())
        return nullptr;
    if (theSlot < 0 || theSlot >= 8)
        return nullptr;

    Sexy::PlayerInfo* profile = found->second.mProfiles[theSlot];
    if (profile == nullptr || profile->GetId() != theSlot)
        return nullptr;

    profile->LoadDetails();
    int useSeq = mNextProfileUseSeq;
    ++mNextProfileUseSeq;
    SetUseSeq(found->second, (unsigned int)useSeq);
    return profile;
}

Sexy::PlayerInfo* Sexy::DefaultProfileMgr::GetProfile(Sexy::String const& theName)
{
    auto found = mProfiles.find(theName);
    if (found == mProfiles.end())
        return nullptr;

    Sexy::PlayerInfo* profile = found->second.mProfiles[0];
    if (profile == nullptr)
        return nullptr;

    profile->LoadDetails();
    int useSeq = mNextProfileUseSeq;
    ++mNextProfileUseSeq;
    SetUseSeq(found->second, (unsigned int)useSeq);
    return profile;
}

void Sexy::DefaultProfileMgr::SyncState(DataSync& theSync)
{
    DataReader* reader = theSync.mReader;
    DataWriter* writer = theSync.mWriter;

    int profileVersion = GetProfileVersion();
    theSync.SyncLong(profileVersion);
    theSync.mVersion = profileVersion;
    if (profileVersion != GetProfileVersion())
        return;

    if (reader != nullptr)
    {
        for (auto& entry : mProfiles)
            ReleaseList(entry.second);
        mProfiles.clear();
        mNumProfiles = 0;

        int entryCount = reader->ReadShort();
        if (entryCount == 0)
        {
            mNextProfileId = 1;
            mNextProfileUseSeq = 1;
            return;
        }

        int nextProfileId = 0;
        int nextUseSeq = 0;
        for (int entry = 0; entry < entryCount; ++entry)
        {
            int playerCount = reader->ReadShort();
            DefaultPlayerList list;

            for (int slot = 0; slot < playerCount && slot < 8; ++slot)
            {
                Sexy::PlayerInfo* profile = CreatePlayerInfo();
                profile->SyncSummary(theSync);

                if (nextUseSeq < profile->mUseSeq)
                    nextUseSeq = profile->mUseSeq;
                if (profile->GetProfileId() > nextProfileId)
                    nextProfileId = profile->GetProfileId();

                int id = profile->GetId();
                if (id < 0 || id >= 8)
                {
                    delete profile;
                    continue;
                }

                Sexy::PlayerInfo* previous = list.mProfiles[id];
                if (profile != previous)
                {
                    ReleaseProfile(previous);
                    AddRefProfile(profile);
                    list.mProfiles[id] = profile;
                }
            }

            if (list.mProfiles[0] == nullptr)
            {
                ReleaseList(list);
                continue;
            }

            Sexy::String name = list.mProfiles[0]->GetName();
            DefaultPlayerList stored;
            for (int slot = 0; slot < 8; ++slot)
            {
                Sexy::PlayerInfo* profile = list.mProfiles[slot];
                if (profile != nullptr)
                    AddRefProfile(profile);
                stored.mProfiles[slot] = profile;
            }
            auto existing = mProfiles.find(name);
            if (existing != mProfiles.end())
            {
                ReleaseList(existing->second);
                existing->second = stored;
            }
            else
            {
                mProfiles.emplace(name, stored);
                ++mNumProfiles;
            }
            ReleaseList(list);
        }

        mNextProfileId = nextProfileId + 1;
        mNextProfileUseSeq = nextUseSeq + 1;
        return;
    }

    writer->WriteShort((unsigned short)mNumProfiles);
    for (auto& entry : mProfiles)
    {
        writer->WriteShort(8);
        for (int slot = 0; slot < 8; ++slot)
        {
            Sexy::PlayerInfo* profile = entry.second.mProfiles[slot];
            if (profile != nullptr)
                profile->SyncSummary(theSync);
        }
    }
}

int Sexy::DefaultProfileMgr::DeleteProfile(Sexy::String const& theName)
{
    auto found = mProfiles.find(theName);
    if (found == mProfiles.end())
        return 0;

    for (int slot = 0; slot < 8; ++slot)
    {
        Sexy::PlayerInfo* profile = found->second.mProfiles[slot];
        if (profile != nullptr)
            profile->DeleteUserFiles();
    }
    ReleaseList(found->second);
    mProfiles.erase(found);
    if (mNumProfiles > 0)
        --mNumProfiles;
    return 1;
}

int Sexy::DefaultProfileMgr::DeleteOldestProfile()
{
    if (mProfiles.empty())
        return 0;

    auto oldest = mProfiles.begin();
    for (auto cursor = mProfiles.begin(); cursor != mProfiles.end(); ++cursor)
    {
        Sexy::PlayerInfo* current = cursor->second.mProfiles[0];
        Sexy::PlayerInfo* best = oldest->second.mProfiles[0];
        if (current != nullptr && best != nullptr && current->mUseSeq < best->mUseSeq)
            oldest = cursor;
    }

    for (int slot = 0; slot < 8; ++slot)
    {
        Sexy::PlayerInfo* profile = oldest->second.mProfiles[slot];
        if (profile != nullptr)
            profile->DeleteUserFiles();
    }
    ReleaseList(oldest->second);
    mProfiles.erase(oldest);
    if (mNumProfiles > 0)
        --mNumProfiles;
    return 0;
}

Sexy::DefaultProfileMgr* Sexy::DefaultProfileMgr::DeleteOldProfiles()
{
    while (mNumProfiles > 200)
        DeleteOldestProfile();
    return this;
}

Sexy::PlayerInfo* Sexy::DefaultProfileMgr::AddProfile(Sexy::String const& theName)
{
    DefaultPlayerList empty;
    auto inserted = mProfiles.emplace(theName, empty);
    if (!inserted.second)
        return nullptr;

    ++mNumProfiles;
    int profileId = mNextProfileId;
    int useSeq = mNextProfileUseSeq;
    ++mNextProfileUseSeq;
    ++mNextProfileId;

    DefaultPlayerList& list = inserted.first->second;
    for (int slot = 0; slot < 8; ++slot)
    {
        Sexy::PlayerInfo* profile = CreatePlayerInfo();
        profile->mName = theName;
        profile->mId = slot;
        profile->mProfileId = profileId;
        profile->mUseSeq = useSeq;
        AddRefProfile(profile);
        list.mProfiles[slot] = profile;
    }

    DeleteOldProfiles();
    return list.mProfiles[0];
}

int Sexy::DefaultProfileMgr::RenameProfile(Sexy::String const& theOldName, Sexy::String const& theNewName)
{
    auto found = mProfiles.find(theOldName);
    if (found == mProfiles.end())
        return 0;

    if (SameNameIgnoreCase(theOldName, theNewName))
    {
        for (int slot = 0; slot < 8; ++slot)
        {
            Sexy::PlayerInfo* profile = found->second.mProfiles[slot];
            if (profile != nullptr)
                profile->mName = theNewName;
        }
        return 1;
    }

    DefaultPlayerList copied;
    for (int slot = 0; slot < 8; ++slot)
        copied.mProfiles[slot] = found->second.mProfiles[slot];

    auto inserted = mProfiles.emplace(theNewName, copied);
    if (!inserted.second)
        return 0;

    mProfiles.erase(found);

    DefaultPlayerList& renamed = mProfiles[theNewName];
    for (int slot = 0; slot < 8; ++slot)
    {
        Sexy::PlayerInfo* profile = renamed.mProfiles[slot];
        if (profile != nullptr)
            profile->mName = theNewName;
    }
    return 1;
}

void Sexy::DefaultProfileMgr::Load()
{
    Sexy::Buffer buffer;
    Sexy::String path;
    GetAppDataFolder(&path);
    path += "userdata/users.dat";

    if (gSexyAppBase->ReadBufferFromFile(path, &buffer, false) == 0)
        return;

    DataReader reader;
    const unsigned char* bits = buffer.mData.empty() ? nullptr : &buffer.mData[0];
    reader.OpenMemory(bits, (unsigned long)((buffer.mDataBitSize + 7) / 8), false);

    DataSync sync(reader);
    SyncState(sync);
}

void Sexy::DefaultProfileMgr::Save()
{
    DataWriter writer;
    writer.OpenMemory(0x20);
    DataSync sync(writer);
    SyncState(sync);

    Sexy::String folder;
    GetAppDataFolder(&folder);
    folder += "userdata";
    MkDir(folder);

    Sexy::String path;
    GetAppDataFolder(&path);
    path += "userdata/users.dat";
    gSexyAppBase->WriteBytesToFile(path, writer.mBuffer, writer.mBufferPos);
}

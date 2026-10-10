// The profile map the app saves and loads. Each name owns eight profile slots.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_DEFAULTPROFILEMGR_H
#define LAWNPROJECTTV_SEXY_INCLUDE_DEFAULTPROFILEMGR_H

#include "ReconTypes.h"
#include "Buffer.h"
#include "DataReader.h"
#include "DataSync.h"
#include "DataWriter.h"
#include "ProfileMgr.h"
#include "SexyAppBase.h"

#include <list>
#include <map>

namespace Sexy
{

class PlayerInfo;

class DefaultProfileMgr_DefaultPlayerList
{
public:
    Sexy::PlayerInfo *mProfiles[8];

    DefaultProfileMgr_DefaultPlayerList()
    {
        for (int slot = 0; slot < 8; ++slot)
            mProfiles[slot] = nullptr;
    }
};

class DefaultProfileMgr : public Sexy::ProfileMgr
{
public:
    virtual ~DefaultProfileMgr();

    virtual int Clear();
    virtual void Load();
    virtual void Save();
    virtual int GetNumProfiles();
    virtual Sexy::PlayerInfo *GetProfile(Sexy::String const &theName, int theUseSeq);
    virtual Sexy::PlayerInfo *GetProfile(Sexy::String const &theName);
    virtual Sexy::PlayerInfo *AddProfile(Sexy::String const &theName);
    virtual Sexy::PlayerInfo *GetAnyProfile();
    virtual int DeleteProfile(Sexy::String const &theName);
    virtual int RenameProfile(Sexy::String const &theOldName, Sexy::String const &theNewName);
    virtual std::list<Sexy::PlayerInfo *> GetProfileList();

    typedef Sexy::DefaultProfileMgr_DefaultPlayerList DefaultPlayerList;

    std::map<Sexy::String, DefaultPlayerList> mProfiles;
    int mNumProfiles;
    int mNextProfileId;
    int mNextProfileUseSeq;

    int SetUseSeq(DefaultPlayerList &theList, unsigned int theUseSeq);
    DefaultProfileMgr(Sexy::ProfileEventListener *theListener);
    void SyncState(DataSync &theSync);
    int DeleteOldestProfile();
    DefaultProfileMgr *DeleteOldProfiles();
};

} // namespace Sexy

using Sexy::DefaultProfileMgr;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_DEFAULTPROFILEMGR_H

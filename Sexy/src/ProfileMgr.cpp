#include "ProfileMgr.h"
#include "ProfileEventListener.h"

namespace Sexy
{

ProfileMgr::ProfileMgr(Sexy::ProfileEventListener *listener) :
    mListener(listener)
{
}

ProfileMgr::~ProfileMgr() {}

ProfileEventListener *ProfileMgr::GetListener()
{
    return mListener;
}

int ProfileMgr::GetProfileVersion()
{
    if (mListener == nullptr)
        return 0;
    return mListener->GetProfileVersion();
}

PlayerInfo *ProfileMgr::CreatePlayerInfo()
{
    if (mListener == nullptr)
        return nullptr;
    return mListener->CreatePlayerInfo();
}

void ProfileMgr::Update() {}

} // namespace Sexy

// The profile backend. It holds the listener the app registered and forwards
// version and player creation to that listener. Update does nothing.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class ProfileEventListener;

class ProfileMgr
{
public:
    ProfileEventListener* mListener;

    ProfileMgr(Sexy::ProfileEventListener* listener);
    ~ProfileMgr();

    ProfileEventListener* GetListener();
    int GetProfileVersion();
    PlayerInfo* CreatePlayerInfo();
    void Update();
};

} // namespace Sexy

using Sexy::ProfileMgr;

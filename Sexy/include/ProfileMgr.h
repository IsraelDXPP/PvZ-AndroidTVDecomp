// The profile backend. It holds the listener the app registered and forwards
// version and player creation to that listener. Update does nothing.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PROFILEMGR_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PROFILEMGR_H

#include "ReconTypes.h"

namespace Sexy
{

class ProfileEventListener;

class ProfileMgr
{
public:
    ProfileEventListener *mListener;

    ProfileMgr(Sexy::ProfileEventListener *listener);
    ~ProfileMgr();

    ProfileEventListener *GetListener();
    int GetProfileVersion();
    PlayerInfo *CreatePlayerInfo();
    void Update();
};

} // namespace Sexy

using Sexy::ProfileMgr;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PROFILEMGR_H

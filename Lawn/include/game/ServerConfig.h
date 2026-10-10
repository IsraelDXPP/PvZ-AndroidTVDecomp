// The local SDL shop has no publisher server URL or data encryption.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_SERVERCONFIG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_SERVERCONFIG_H

#include "ReconTypes.h"

class ServerConfig
{
public:
    static Sexy::String GetDomainURL();
    static bool IsEncryptionEnabled();
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_SERVERCONFIG_H

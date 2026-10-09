// The local SDL shop has no publisher server URL or data encryption.

#pragma once

#include "ReconTypes.h"

class ServerConfig
{
public:
    static Sexy::String GetDomainURL();
    static bool IsEncryptionEnabled();
};

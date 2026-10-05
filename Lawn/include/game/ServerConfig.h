// ServerConfig is two static helpers. The TV binary has no instance data:
// getDomainURL returns the shop URL by value, and encryption stays off.

#pragma once

#include "ReconTypes.h"

class ServerConfig
{
public:
    static Sexy::String GetDomainURL();
    static bool IsEncryptionEnabled();
};

#include "ServerConfig.h"

Sexy::String ServerConfig::GetDomainURL()
{
    // The SDL port uses the local shop/session implementation.
    return Sexy::String();
}

bool ServerConfig::IsEncryptionEnabled()
{
    return false;
}

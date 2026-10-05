#include "ServerConfig.h"

#include <cstdio>

Sexy::String ServerConfig::GetDomainURL()
{
    const char* theUrl =
        "http://xiaomigameserver.kalazhu.com/mobilegameserver/server/index.php";
    printf("!!!! SHOP_SERVER:%s\n", theUrl);
    return Sexy::String(theUrl);
}

bool ServerConfig::IsEncryptionEnabled()
{
    return false;
}

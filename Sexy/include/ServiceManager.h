// ServiceManager -- the process-wide service the app names and releases.
//
// The platform layer owns the real instance. This port's getInstance answers
// null, and setName / release accept that.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SERVICEMANAGER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SERVICEMANAGER_H

#include "SexyString.h"

namespace Sexy
{

class ServiceManager
{
public:
    static ServiceManager *getInstance(ServiceManager *theSingleton);
    static int setName(ServiceManager *theManager, const Sexy::String &theName);
    static void release(ServiceManager *theManager);
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SERVICEMANAGER_H

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_CRITSECT_H
#define LAWNPROJECTTV_SEXY_INCLUDE_CRITSECT_H
#include <SDL3/SDL_mutex.h>

namespace Sexy
{
class CritSect
{
public:
    SDL_Mutex *mSection;
    CritSect();
    ~CritSect();
    CritSect(const CritSect &) = delete;
    CritSect &operator=(const CritSect &) = delete;
    bool TryLock();
    int Lock();
    int Unlock();
};
}
namespace PakLib
{
class CritSect : public Sexy::CritSect
{
public:
    int Enter()
    {
        return Lock();
    }
    int Leave()
    {
        return Unlock();
    }
};
}

#endif // LAWNPROJECTTV_SEXY_INCLUDE_CRITSECT_H

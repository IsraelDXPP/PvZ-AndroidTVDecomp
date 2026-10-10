#include "CritSect.h"
Sexy::CritSect::CritSect() :
    mSection(SDL_CreateMutex())
{
}
Sexy::CritSect::~CritSect()
{
    SDL_DestroyMutex(mSection);
}
bool Sexy::CritSect::TryLock()
{
    return SDL_TryLockMutex(mSection);
}
int Sexy::CritSect::Lock()
{
    SDL_LockMutex(mSection);
    return 0;
}
int Sexy::CritSect::Unlock()
{
    SDL_UnlockMutex(mSection);
    return 0;
}

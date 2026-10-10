// Factories for the portable SDL3 application/resource drivers.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SDLAPPDRIVER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SDLAPPDRIVER_H

namespace Sexy
{
class SexyAppBase;
class IAppDriver;
class IResStreamsDriver;
class IAudioDriver;
class IHttpDriver;
class IFileDriver;
}
extern "C" Sexy::IAppDriver *Lawn_CreateAppDriver(Sexy::SexyAppBase *theApp);

// Retained local interfaces for services absent from SDL.
extern "C" Sexy::IResStreamsDriver *Lawn_CreateResStreamsDriver();

extern "C" Sexy::IAudioDriver *Lawn_CreateAudioDriver(Sexy::SexyAppBase *theApp);

extern "C" Sexy::IHttpDriver *Lawn_CreateHttpDriver(Sexy::SexyAppBase *theApp);

extern "C" Sexy::IFileDriver *Lawn_CreateFileDriver(Sexy::SexyAppBase *theApp);

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SDLAPPDRIVER_H

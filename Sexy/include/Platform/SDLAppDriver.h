// Factories for the portable SDL3 application/resource drivers.

#pragma once

namespace Sexy {
class SexyAppBase;
class IAppDriver;
class IResStreamsDriver;
class IAudioDriver;
class IHttpDriver;
class IFileDriver;
}
extern "C" Sexy::IAppDriver* LawnTV_CreateAppDriver(Sexy::SexyAppBase* theApp);

// Retained local interfaces for services absent from SDL.
extern "C" Sexy::IResStreamsDriver* LawnTV_CreateResStreamsDriver();

extern "C" Sexy::IAudioDriver* LawnTV_CreateAudioDriver(Sexy::SexyAppBase* theApp);

extern "C" Sexy::IHttpDriver* LawnTV_CreateHttpDriver(Sexy::SexyAppBase* theApp);

extern "C" Sexy::IFileDriver* LawnTV_CreateFileDriver(Sexy::SexyAppBase* theApp);

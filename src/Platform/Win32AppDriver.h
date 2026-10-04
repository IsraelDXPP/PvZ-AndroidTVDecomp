// Win32AppDriver.h -- C entry points for the Windows platform driver objects.
//
// In the binary SexyAppBase::SexyAppBase @0x33F288 builds four platform driver
// objects and stores them in raw object slots:
//
//     +744  Sexy::gFileDriver             (global, not built here)
//     +748  Sexy::BaseAppDriver           CreateAppDriver(theApp)
//     +756  Sexy::IAudioDriver            CreateAudioDriver(theApp)
//     +760  Sexy::IResStreamsDriver       CreateResStreamsDriver()
//     +764  Sexy::IHttpDriver             CreateHttpDriver(theApp)
//
// The Android build got these from JNI/EGL/AAudio/HttpURLConnection. On Windows
// they are implemented by Win32AppDriver.cpp on top of the existing
// Platform/ components (GLWindow, GLRenderBackend, Screenshot).
//
// The five Sexy::*Driver::CreateXxxDriver factory symbols are declared inside
// Sexy/SexyAppBase.cpp as file-local classes with no definitions, so the link
// fails until those bodies are supplied. They should be wired straight onto the
// four exports below, for example:
//
//     Sexy::BaseAppDriver* Sexy::BaseAppDriver::CreateAppDriver(SexyAppBase* a2)
//     {
//         return (Sexy::BaseAppDriver*)LawnTV_CreateAppDriver(a2);
//     }
//
// These are declared extern "C" on purpose. The returned object is a hand-built
// vtable blob, not a C++ object with a compiler-generated vtable: the binary is
// Itanium ABI (two destructor slots) while MSVC emits one, so every slot number
// past the destructors would be off by one and the game's raw
// `driver->vtbl[N]()` calls would dispatch to the wrong function. See the
// implementation for the slot table.
//
// The header deliberately includes nothing, so it can be included from the
// transcribed sources without dragging anything into them.

#pragma once

// The application driver (the object that lives at SexyAppBase + 748).
// `theApp` is the SexyAppBase the driver belongs to; it is only used to read the
// desired back-buffer size and may be null.
extern "C" void* LawnTV_CreateAppDriver(void* theApp);

// The resource-stream driver (SexyAppBase + 760). Placeholder: a dumb object
// whose vtable answers "success" to everything.
extern "C" void* LawnTV_CreateResStreamsDriver();

// The audio driver (SexyAppBase + 756). Placeholder, as above.
extern "C" void* LawnTV_CreateAudioDriver(void* theApp);

// The HTTP driver (SexyAppBase + 764). Placeholder, as above.
extern "C" void* LawnTV_CreateHttpDriver(void* theApp);

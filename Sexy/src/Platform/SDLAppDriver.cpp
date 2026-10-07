// SDL3 application and resource drivers. Game-facing interfaces retain their original semantics.

#include "SDLAppDriver.h"
#include "BaseAppDriver.h"
#include "GLImage.h"

#include "Audio/SDLAudioBackend.h"
#include "SDLWindow.h"
#include "Bgfx/BgfxRenderBackend.h"
#include "Screenshot.h"
#include "SDLFile.h"
#include "Buffer.h"

#include "AndroidShims.h"
#include "ReconHelpers.h"
#include "ReconTypes.h"
#include "SexyTypes.h"
#include "SexyAppBase.h"
#include "NativeDisplay.h"

// The driver's Start slot calls the app's Init (see Start below), so the
// concrete app type is needed here.
#include "game/LawnApp.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "Bgfx/BgfxRenderDevice.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <string.h>

namespace
{

struct SDLAppDriver : Sexy::IAppDriver
{
    Sexy::SexyAppBase*       mApp;           // the application, may be null
    Sexy::SDLWindow*         mWindow;        // the display, may be null
    bool                    mOwnsWindow;    // false when an outer owner made it
    Sexy::IGraphicsBackend* mGraphics;
    int                     mWidth;
    int                     mHeight;
    double                  mLastUpdateTime = -1.0;

    int InitAppDriver() override;
    int Startup() override;
    int Terminate() override;
    int Start() override;
    int Init() override;
    int UpdateAppStep(bool* theContinue) override;
    int DrawDirtyStuff(bool theForce) override;
    int Shutdown() override;
    int StartLoadingThread() override;
    int WaitForLoadingThread() override;
    double GetLoadingThreadProgress() override;
    int EnforceCursor() override;
    int KeyDown(int theKeyCode) override;
    int DebugKeyDown(int theKeyCode) override;
    int Is3DAccelerated() override;
    int Is3DAccelerationSupported() override;
    int Is3DAccelerationRecommended() override;
    int PreDrawScreen() override;
    int ReadBufferFromFile(const Sexy::String& theFileName, Sexy::Buffer* theBuffer, bool theIsCompressed) override;
    bool WriteBufferToFile(const Sexy::String& theFileName, const Sexy::Buffer* theBuffer) override;
    bool WriteBytesToFile(const Sexy::String& theFileName, const void* theData, unsigned long theLength) override;
    int ShouldPauseUpdates() override;
    int RehupFocus() override;
    int GetMemoryInfo(void* theMemoryInfo) override;
    int TakeScreenshot(Sexy::MemoryImage* theImage) override;
    int DoMainLoop() override;
    int DoUpdateFramesF(float theFTime) override;
    int DoUpdateFrames() override;
    int MakeWindow() override;
    int CreateGraphics() override;
    int InitGraphics() override;
    int DestroyGraphics() override;
    int UpdateInput() override;
};

struct SDLResStreamsDriver : Sexy::IResStreamsDriver {};
struct SDLAudioDriver : Sexy::IAudioDriver {};
struct SDLHttpDriver : Sexy::IHttpDriver {};
struct SDLFileDriver : Sexy::IFileDriver {};

const int kDefaultWidth = 800;
const int kDefaultHeight = 600;

// ---------------------------------------------------------------------------
// Window / display
// ---------------------------------------------------------------------------

// Read the application's requested stage size through its actual fields.
void AppDriver_ReadAppSize(const Sexy::SexyAppBase* app, int& width, int& height)
{
    width = app && app->mWidth > 0 ? app->mWidth : kDefaultWidth;
    height = app && app->mHeight > 0 ? app->mHeight : kDefaultHeight;
}

// Create the display if it does not exist yet.
//
// If an SDL window already exists, somebody outside the driver (main.cpp
// creates a SDLWindow before LawnApp) already owns the display, and opening a
// second window would create a separate display. In that
// case the driver records the size and reuses that window.
void AppDriver_CenterScreen(SDLAppDriver* theSelf);

bool AppDriver_EnsureWindow(SDLAppDriver* theSelf)
{
    if (!theSelf)
        return false;
    if (theSelf->mWindow)
        return true;

    AppDriver_ReadAppSize(theSelf->mApp, theSelf->mWidth, theSelf->mHeight);

    if (Sexy::SDLWindow* active = Sexy::ActiveWindow())
    {
        theSelf->mWindow = active;
        theSelf->mOwnsWindow = false;
        theSelf->mWidth = active->mWidth;
        theSelf->mHeight = active->mHeight;
        return true;
    }

    Sexy::SDLWindow* aWindow = new Sexy::SDLWindow();
    if (!aWindow->Create("Plants vs. Zombies TV",
                         theSelf->mWidth, theSelf->mHeight,
                         true, true, true))
    {
        TOD_ERROR("SDLAppDriver: SDLWindow::Create(%dx%d) failed",
                  theSelf->mWidth, theSelf->mHeight);
        delete aWindow;
        return false;
    }

    theSelf->mWindow = aWindow;
    theSelf->mOwnsWindow = true;
    TOD_TRACE("SDLAppDriver: created %dx%d", theSelf->mWidth, theSelf->mHeight);
    return true;
}

// BaseAppDriver::InitGraphics @0x3F3644.
//
//   r0 = driver[+0x40]          display width, stored at app+0x620
//   r2 = driver[+0x44]          display height, stored at app+0x624
//   r1 = app.mWidth  (+0x4C)
//   r5 = app.mHeight (+0x50)
//   app+0x618 = (mWidth  - displayWidth)  / 2
//   app+0x61C = (mHeight - displayHeight) / 2
//
// On a 1280 window and an 800 stage that origin is -240. DrawScreen
// translates by its negative, which is the centering shift.
void AppDriver_CenterScreen(SDLAppDriver* theSelf)
{
    if (theSelf == nullptr || theSelf->mApp == nullptr)
        return;

    Sexy::SexyAppBase* anApp = theSelf->mApp;
    if (anApp->mWidgetManager == nullptr)
        return;

    int aDisplayWidth = theSelf->mWidth;
    int aDisplayHeight = theSelf->mHeight;

    if (anApp->mPlatformDriverOrQueue == nullptr)
        anApp->mPlatformDriverOrQueue = new Sexy::NativeDisplay();
    anApp->mPlatformDriverOrQueue->mScreenHeight = aDisplayHeight;

    anApp->mScreenBounds.mWidth = aDisplayWidth;
    anApp->mScreenBounds.mHeight = aDisplayHeight;
    anApp->mScreenBounds.mX = (anApp->mWidth - aDisplayWidth) / 2;
    anApp->mScreenBounds.mY = (anApp->mHeight - aDisplayHeight) / 2;

    Sexy::TRect<int> aSource(0, 0, aDisplayWidth, aDisplayHeight);
    anApp->mWidgetManager->Resize(anApp->mScreenBounds, aSource);

    // BaseAppDriver::InitGraphics @0x3F3684. The byte at app+0x795 is
    // mRuntimeFlags[1], which LawnApp sets. OnArrowKeys refuses to walk
    // focus links while this stays off, so a d-pad press never leaves the
    // button that already has focus.
    anApp->mWidgetManager->SetAllowFullKeyboardMode(anApp->mRuntimeFlags[1]);
}

void AppDriver_DestroyWindow(SDLAppDriver* theSelf)
{
    if (!theSelf || !theSelf->mWindow)
        return;

    // SDL itself stays initialised: it is torn down once, by ShutdownSDL(),
    // after every window is gone.
    if (theSelf->mOwnsWindow)
    {
        theSelf->mWindow->Destroy();
        delete theSelf->mWindow;
    }
    theSelf->mWindow = nullptr;
    theSelf->mOwnsWindow = false;
}

// ---------------------------------------------------------------------------
// Graphics backend
// ---------------------------------------------------------------------------
void AppDriver_CreateGraphics(SDLAppDriver* theSelf)
{
    if (!theSelf || theSelf->mGraphics)
        return;

    // The binary picks its video driver through the same registry, reading
    // SEXY_VIDEO_DRIVER (BaseAppDriver::CreateGraphics @0x3f4384).
    theSelf->mGraphics =
        Sexy::GraphicsBackendFactory::Create(Sexy::GraphicsBackendFactory::GetRequestedName());
}

bool AppDriver_InitGraphics(SDLAppDriver* theSelf)
{
    if (!theSelf)
        return false;

    AppDriver_CreateGraphics(theSelf);
    if (!theSelf->mGraphics)
        return false;


    bool anOk = theSelf->mGraphics->Init(theSelf->mWidth, theSelf->mHeight);
    if (!anOk)
        TOD_ERROR("SDLAppDriver: graphics initialization failed");

    if (anOk)
        TOD_TRACE("SDLAppDriver: graphics backend = %s", theSelf->mGraphics->GetName());
    return anOk;
}

void AppDriver_DestroyGraphics(SDLAppDriver* theSelf)
{
    if (!theSelf || !theSelf->mGraphics)
        return;
    theSelf->mGraphics->Shutdown();
    delete theSelf->mGraphics;
    theSelf->mGraphics = nullptr;
}

// ---------------------------------------------------------------------------
// Real slot bodies
// ---------------------------------------------------------------------------

// [2] InitAppDriver()
int SDLAppDriver::InitAppDriver()
{
    return 1;
}

// [3] Startup()
//
// The real one makes the window, creates the graphics driver and starts it.
// That is also the order the rest of the driver depends on, so the three steps
// are performed here rather than left to the caller.
int SDLAppDriver::Startup()
{
    SDLAppDriver* self = this;
    if (!self)
        return 0;

    bool aHaveWindow = AppDriver_EnsureWindow(self);
    bool aHaveGraphics = aHaveWindow && AppDriver_InitGraphics(self);
    // BaseAppDriver::InitGraphics @0x3F3644 sizes mScreenBounds after the
    // display exists. Startup is that point in this driver.
    AppDriver_CenterScreen(self);

    return aHaveGraphics ? 1 : 0;
}

// [4] Terminate()
int SDLAppDriver::Terminate()
{
    SDLAppDriver* self = this;
    if (!self)
        return 0;
    AppDriver_DestroyGraphics(self);
    AppDriver_DestroyWindow(self);
    return 1;
}

// BaseAppDriver::Start initializes the application.
int SDLAppDriver::Start()
{
    SDLAppDriver* self = this;
    if (!self || self->mApp == nullptr)
        return 0;

    static_cast<LawnApp*>(self->mApp)->Init();
    return 1;
}

// [6] Init()
int SDLAppDriver::Init()
{
    // The binary creates BOTH audio objects here, and this slot is where it
    // happens: BaseAppDriver::Init @0x3F3792 stores the sound driver's slot 3
    // result into `[app + 0x4E8]` (SexyAppBase::mSoundManager) and slot 4's into
    // `[app + 0x304]` (SexyAppBase::mMusicInterface), then falls back to a bare
    // `new MusicInterface()` at 0x3F37B6 when the music driver returns null.
    //
    // The binary reaches them through its audio driver object (SexyAppBase+756).
    // This port has no such driver -- the AG/audiere layer is replaced -- so the
    // two objects are constructed directly from the SDL backend. That keeps
    // the game's own call sites unchanged: they only ever go through
    // mSoundManager and mMusicInterface.
    //
    // Both are optional. If SDL cannot open a device, Init() returns false and
    // the members stay null, which is the state every transcribed body already
    // guards for (`if (mSoundManager == nullptr) return;`), so a machine with no
    // sound card still runs.
    SDLAppDriver* self = this;
    if (self && self->mApp)
    {
        Sexy::SexyAppBase* anApp = self->mApp;

        if (anApp->mSoundManager == nullptr)
        {
            Sexy::SDLSoundManager* aSoundManager = new Sexy::SDLSoundManager;
            if (aSoundManager->Init())
            {
                anApp->mSoundManager = aSoundManager;
                // The music interface shares the manager's SDL audio device rather
                // than opening a second device. The app deletes it in Cleanup,
                // before the sound manager, so the manager must give up ownership.
                anApp->mMusicInterface = aSoundManager->GetMusicInterface();
                aSoundManager->mMusicInterface = nullptr;
                TOD_TRACE("SDLAppDriver: audio backend = SDL");
            }
            else
            {
                delete aSoundManager;
                TOD_WARN("SDLAppDriver: no audio device, running silent");
            }
        }
    }
    return 1;
}

// [7] UpdateAppStep(bool*)
//
// BaseAppDriver::UpdateAppStep @0x3F3C94. The flag starts false and becomes
// true only when a real update advanced mUpdateCount. State 0 collects input
// and then calls UpdateApp (vtable +520). State 1 runs one update, the
// fractional update, and DrawDirtyStuff. Logic advances in mFrameTime-sized
// steps independently of presentation and its frame cap.
int SDLAppDriver::UpdateAppStep(bool* theContinue)
{
    SDLAppDriver* self = this;
    if (theContinue)
        *theContinue = false;
    if (!self || !self->mApp)
        return 0;

    LawnApp* anApp = static_cast<LawnApp*>(self->mApp);
    // Leave destruction to the outer frame loop after callbacks and modal
    // waits have returned. Quit only requests this transition.
    if (anApp->mShutDown || (anApp->mExitToTop && anApp->mUpdateDepth == 0))
        return 0;

    Sexy::SDLWindow* aWindow = self->mWindow != nullptr ? self->mWindow : Sexy::ActiveWindow();
    bool aModalWait = anApp->mUpdateDepth > 0;
    double frameStart = aWindow != nullptr ? aWindow->GetTime() : 0.0;
    if (aModalWait && aWindow != nullptr)
    {
        if (!aWindow->ProcessEvents())
            return 0;
    }

    anApp->CheckControllerStatus();
    ++anApp->mUpdateAppDepth;

    if (anApp->mUpdateAppState == 0)
    {
        // The binary calls UpdateApp here, which immediately re-enters this
        // slot. The work that call is for is the input drain.
        // Callbacks may enter WaitForResult. Advance the state and detach this
        // batch before dispatch so nested steps cannot replay the opening key.
        anApp->mUpdateAppState = 1;
        anApp->PopulateMessages();
        std::vector<Sexy::Event> aEvents;
        aEvents.swap(anApp->mQueuedEvents);
        anApp->ProcessMessages(aEvents);
    }
    else
    {
        int anUpdateCount = anApp->mUpdateCount;
        double aStepSeconds = std::max(anApp->mFrameTime, 1) * 0.001;
        double aNow = (SDL_GetTicksNS() / 1000000000.0);
        double anElapsed = self->mLastUpdateTime < 0.0
            ? aStepSeconds : std::clamp(aNow - self->mLastUpdateTime, 0.0, 0.25);
        self->mLastUpdateTime = aNow;
        anApp->mPendingUpdatesAcc += anElapsed * anApp->mUpdateMultiplier;
        // Resource loading may run synchronously in TitleScreen::Update.
        // Do not replay its blocking time as a burst of loading-bar updates.
        if (anApp->mTitleScreen != nullptr)
            anApp->mPendingUpdatesAcc = std::min(anApp->mPendingUpdatesAcc, aStepSeconds);
        while (anApp->mPendingUpdatesAcc >= aStepSeconds)
        {
            anApp->mPendingUpdatesAcc -= aStepSeconds;
            self->DoUpdateFrames();
            if (anApp->mShutDown || anApp->mExitToTop)
                break;
        }
        self->DoUpdateFramesF( static_cast<float>(anElapsed / aStepSeconds));
        anApp->DrawDirtyStuff();
        if (theContinue)
            *theContinue = anApp->mUpdateCount != anUpdateCount;
        anApp->mUpdateAppState = 0;

        // The outer frame loop is suspended while a dialog waits. Present
        // the same offscreen target here and keep the normal frame cap.
        if (aModalWait && aWindow != nullptr)
        {
            if (Sexy::BgfxRenderDevice::sCurrent != nullptr)
                Sexy::BgfxRenderDevice::sCurrent->Present();
            aWindow->WaitForFrame(frameStart, anApp->mMaxFPS);
        }
    }

    --anApp->mUpdateAppDepth;
    anApp->ProcessSafeDeleteList();
    return !anApp->mShutDown && !(anApp->mExitToTop && anApp->mUpdateDepth == 0);
}

// [90] DoUpdateFramesF(float)
//
// BaseAppDriver::DoUpdateFramesF @0x3F39A0 forwards to the widget manager
// when v-sync updates are on and the window is not minimized.
int SDLAppDriver::DoUpdateFramesF(float theFrac)
{
    SDLAppDriver* self = this;
    if (!self || !self->mApp)
        return 0;
    LawnApp* anApp = static_cast<LawnApp*>(self->mApp);
    if (anApp->mVSyncUpdates && !anApp->mMinimized && anApp->mWidgetManager != nullptr)
        anApp->mWidgetManager->UpdateFrameF(theFrac);
    return 1;
}

// [91] DoUpdateFrames()
//
// BaseAppDriver::DoUpdateFrames @0x3F39B8. The demo-buffer branch is the
// recorded-playback path; a live run takes the other one and calls UpdateFrames.
int SDLAppDriver::DoUpdateFrames()
{
    SDLAppDriver* self = this;
    if (!self || !self->mApp)
        return 0;
    LawnApp* anApp = static_cast<LawnApp*>(self->mApp);

    if (anApp->mPlayingDemoBuffer)
    {
        if (anApp->mLoadingThreadCompleted && !anApp->mLoaded && anApp->mDemoLoadingComplete)
        {
            anApp->mLoaded = true;
            anApp->mYieldMainThread = false;
            anApp->LoadingThreadCompleted();
        }
        if (anApp->mLoaded == anApp->mDemoLoadingComplete && anApp->mUpdateCount != anApp->mLastDemoUpdateCnt)
        {
            anApp->UpdateFrames();
            return 1;
        }
        return 0;
    }

    if (anApp->mLoadingThreadCompleted && !anApp->mLoaded)
    {
        anApp->mLoaded = true;
        anApp->mYieldMainThread = false;
        anApp->LoadingThreadCompleted();
    }
    anApp->UpdateFrames();
    return 1;
}

// [9] DrawDirtyStuff(bool)
//
// BaseAppDriver::DrawDirtyStuff @0x3F4628 calls WidgetManager::DrawScreen.
// The widgets paint the manager's image. The window only presents that image.
int SDLAppDriver::DrawDirtyStuff(bool theForce)
{
    (void)theForce;
    SDLAppDriver* self = this;
    if (!self)
        return 0;

    LawnApp* anApp = static_cast<LawnApp*>(self->mApp);
    if (anApp == nullptr || anApp->mWidgetManager == nullptr)
        return 0;


    Sexy::WidgetManager* aManager = anApp->mWidgetManager;
    // DrawDirtyStuff @0x3F4628 presents the manager image as drawn.
    // Resize stores mWidth as dest.mWidth + 2*dest.mX, which is the
    // stage width, not the window. The image has to be the display.
    int aWidth = self->mWidth > 0 ? self->mWidth : aManager->mWidth;
    int aHeight = self->mHeight > 0 ? self->mHeight : aManager->mHeight;
    if (aWidth <= 0)
        aWidth = 1280;
    if (aHeight <= 0)
        aHeight = 720;

    // The screen image is a GLImage when a device is live, and a MemoryImage
    // otherwise. GLImage::Blt takes the hardware path only while that image is
    // the device's current draw target; every other image stays on the
    // software rasterizer. That is the split in GLImage::Blt @0x475888.
    bool aHardware = Sexy::BgfxRenderDevice::sCurrent != nullptr;
    bool aScreenIsHardware = dynamic_cast<Sexy::GLImage*>(aManager->mImage) != nullptr;
    if (aManager->mImage == nullptr || aHardware != aScreenIsHardware)
    {
        delete aManager->mImage;
        aManager->mImage = aHardware ? new Sexy::GLImage() : new Sexy::MemoryImage();
        aManager->mImage->Create(aWidth, aHeight);
    }
    else if (aManager->mImage->mWidth != aWidth || aManager->mImage->mHeight != aHeight)
    {
        aManager->mImage->Create(aWidth, aHeight);
    }

    anApp->mIsDrawing = true;
    if (aHardware)
    {
        // BeginFrame clears the entire FBO. Repaint every visible widget,
        // including static dialogs over an animated menu.
        aManager->MarkAllDirty();
        Sexy::BgfxRenderDevice::sCurrent->SetDrawTarget(aManager->mImage);
        Sexy::BgfxRenderDevice::sCurrent->BeginFrame(Sexy::Color(0, 0, 0, 255));
    }
    int aDrew = aManager->DrawScreen();
    if (aHardware)
    {
        Sexy::BgfxRenderDevice::sCurrent->EndFrame();
        Sexy::BgfxRenderDevice::sCurrent->SetDrawTarget(nullptr);
    }
    anApp->mIsDrawing = false;
    if (aDrew == 0)
        return 0;

    // A software screen still has to be uploaded. A hardware screen already
    // drew its quads into the device, so uploading it again is the stutter.
    if (!aHardware && Sexy::BgfxRenderDevice::sCurrent != nullptr)
        Sexy::BgfxRenderDevice::sCurrent->BlitImage(aManager->mImage);
    return 1;
}

// [12] Shutdown()
int SDLAppDriver::Shutdown()
{
    SDLAppDriver* self = this;
    if (!self)
        return 0;
    AppDriver_DestroyGraphics(self);
    AppDriver_DestroyWindow(self);
    return 1;
}

// 0 = one slice on the calling thread. 1 = a worker. -1 until the first call
// reads SEXY_LOADING_THREAD. Android and web leave it off; this desktop
// build turns it on in setup.env.
int gLoadingThreadMode = -1;
SDL_Thread* gLoadingThread = nullptr;

int SDLCALL LoadingThreadMain(void* theArg)
{
    LawnApp* anApp = static_cast<LawnApp*>(theArg);
    while (anApp != nullptr && !anApp->mShutDown && !anApp->mCloseRequest
           && !anApp->mLoadingThreadCompleted)
    {
        anApp->LoadingThreadProc();
        // One slice just ran. Yield so the main thread can paint the bar
        // even when this core is busy decoding the next batch.
        SDL_Delay(1);
    }
    return 0;
}

// [28] StartLoadingThread()
//
// The first call still prepares the loader-bar art on this thread: the bar
// cannot draw until those images exist. Every later slice runs on the worker,
// so TitleScreen::Update can return and the frame loop can present the bar
// while images, fonts and sounds come in.
int SDLAppDriver::StartLoadingThread()
{
    SDLAppDriver* self = this;
    if (!self || !self->mApp)
        return 0;

    LawnApp* anApp = static_cast<LawnApp*>(self->mApp);
    if (anApp->mLoadingThreadCompleted)
        return 1;

    if (gLoadingThreadMode < 0)
        gLoadingThreadMode = Sexy::GetEnvOption("SEXY_LOADING_THREAD", false) != 0 ? 1 : 0;

    // Default path: the caller (the frame loop) pulls one slice and returns,
    // so a platform with no worker threads can still move the bar.
    if (gLoadingThreadMode == 0)
    {
        anApp->LoadingThreadProc();
        return 1;
    }

    if (gLoadingThread != nullptr)
        return 1;

    anApp->PrepareTitleLoad();
    if (anApp->mLoadingThreadCompleted || anApp->mShutDown || anApp->mCloseRequest)
        return 1;

    gLoadingThread = SDL_CreateThread(LoadingThreadMain, "resource-loader", anApp);
    if (gLoadingThread == nullptr)
        anApp->LoadingThreadProc();
    return 1;
}

// [29] WaitForLoadingThread()
int SDLAppDriver::WaitForLoadingThread()
{
    if (gLoadingThread == nullptr)
        return 1;
    SDL_WaitThread(gLoadingThread, nullptr);
    gLoadingThread = nullptr;
    return 1;
}

// [30] GetLoadingThreadProgress()
//
// The fraction the loading bar reads: completed tasks over the total the
// loading thread counted. Both live on the app, at the offsets the binary uses.
double SDLAppDriver::GetLoadingThreadProgress()
{
    SDLAppDriver* aDriver = this;
    if (aDriver == nullptr || aDriver->mApp == nullptr)
        return 0.0;
    return aDriver->mApp->GetLoadingThreadProgress();
}

// [37] EnforceCursor()
int SDLAppDriver::EnforceCursor()
{
    return 1;
}

// [41] KeyDown(int)
int SDLAppDriver::KeyDown(int theKey)
{
    return mWindow && mWindow->IsKeyDown(theKey);
}

// [42] DebugKeyDown(int)
int SDLAppDriver::DebugKeyDown(int theKey)
{
    return KeyDown(theKey);
}

// [44]/[45]/[46] Is3DAccelerated / Is3DAccelerationSupported /
// Is3DAccelerationRecommended
//
// SDLWindow supplies the native window used by bgfx, and the game
// takes the accelerated path when these answer 1.
int SDLAppDriver::Is3DAccelerated()
{
    return 1;
}

int SDLAppDriver::Is3DAccelerationSupported()
{
    return 1;
}

int SDLAppDriver::Is3DAccelerationRecommended()
{
    return 1;
}

// [51] PreDrawScreen()
int SDLAppDriver::PreDrawScreen()
{
    return 1;
}

// Read logical resources through p_fopen, including mounted pak files.
int SDLAppDriver::ReadBufferFromFile(const Sexy::String& thePath,
                                    Sexy::Buffer* theBuffer, bool theFlag)
{
    (void)theFlag;

    if (theBuffer == nullptr)
        return 0;

    const char* aName = thePath.c_str();
    if (aName == nullptr || aName[0] == '\0')
        return 0;

    FILE* aStream = p_fopen(aName, "rb");
    if (aStream == nullptr)
        return 0;

    std::vector<unsigned char> aBytes;
    unsigned char aChunk[16384];
    for (;;)
    {
        int aRead = (int)fread(aChunk, 1, sizeof(aChunk), aStream);
        if (aRead <= 0)
            break;
        aBytes.insert(aBytes.end(), aChunk, aChunk + aRead);
    }
    p_fclose(aStream);

    if (aBytes.empty())
        return 0;

    // SetData builds the byte vector at the Buffer's +4 field and sets the bit
    // count at +16 to 8 * size, which is what ParseResourcesFile reads back
    // through (data, bitSize) to build the string source.
    theBuffer->SetData(aBytes.data(), (int)aBytes.size());
    return 1;
}

// [74] ShouldPauseUpdates()
int SDLAppDriver::ShouldPauseUpdates()
{
    return 0;
}

// [75] RehupFocus()
int SDLAppDriver::RehupFocus()
{
    return 1;
}

// [86] GetMemoryInfo(MemoryInfo&)
//
// TODO(recon): the recon declares SexyAppBase::GetMemoryInfo as returning int
// with no MemoryInfo type anywhere in the tree, and the call site passes only
// `this`, so there is no out-parameter to fill. 0 = "unknown"; the callers
// already tolerate it.
int SDLAppDriver::GetMemoryInfo(void* theMemoryInfo)
{
    (void)theMemoryInfo;
    return 0;
}

// [87] TakeScreenshot(MemoryImage&)
//
// Captures the supplied offscreen image.
int SDLAppDriver::TakeScreenshot(Sexy::MemoryImage* theImage)
{
    if (!theImage)
        return 0;
    // Platform/Screenshot.h owns the numbered-file policy.
    return Sexy::Screenshot::SaveNext(theImage) ? 1 : 0;
}

// [88] DoMainLoop()
//
// The frame loop lives in main.cpp, which owns the message pump; the driver
// only reports that the loop is entered and left normally.
int SDLAppDriver::DoMainLoop()
{
    return 1;
}

// [92] MakeWindow()
int SDLAppDriver::MakeWindow()
{
    SDLAppDriver* self = this;
    if (!self)
        return 0;
    return AppDriver_EnsureWindow(self) ? 1 : 0;
}

// [93] CreateGraphics()
int SDLAppDriver::CreateGraphics()
{
    SDLAppDriver* self = this;
    if (!self)
        return 0;
    AppDriver_CreateGraphics(self);
    return self->mGraphics ? 1 : 0;
}

// [94] InitGraphics()
int SDLAppDriver::InitGraphics()
{
    return AppDriver_InitGraphics(this) ? 1 : 0;
}

// [95] DestroyGraphics()
int SDLAppDriver::DestroyGraphics()
{
    AppDriver_DestroyGraphics(this);
    return 1;
}

// [96] UpdateInput()
int SDLAppDriver::UpdateInput()
{
    SDLAppDriver* self = this;
    if (self && self->mWindow)
        self->mWindow->ProcessEvents();
    return 1;
}

bool SDLAppDriver::WriteBufferToFile(const Sexy::String& theFileName, const Sexy::Buffer* theBuffer)
{
    if (theBuffer == nullptr)
        return false;
    int dataLength = theBuffer->GetDataLen();
    if (dataLength < 0 || static_cast<size_t>(dataLength) != theBuffer->mData.size())
    {
        logfe("Invalid output buffer: path=%s, bytes=%d, allocated=%zu",
              theFileName.c_str(), dataLength, theBuffer->mData.size());
        return false;
    }
    return WriteBytesToFile(theFileName, theBuffer->GetDataPtr(), dataLength);
}
bool SDLAppDriver::WriteBytesToFile(const Sexy::String& theFileName, const void* theData, unsigned long theLength)
{
    // BaseAppDriver::WriteBytesToFile @0x3f4548 writes the supplied path.
    // Saves must use the output path, without asset-root redirection.
    return Sexy::SDLFile::WriteOutputFile(theFileName.c_str(), theData, theLength);
}
SDLAppDriver* MakeAppDriver(Sexy::SexyAppBase* theApp)
{
    SDLAppDriver* aDriver = new SDLAppDriver();
    aDriver->mApp        = theApp;
    aDriver->mWindow     = nullptr;
    aDriver->mOwnsWindow = false;
    aDriver->mGraphics   = nullptr;
    aDriver->mWidth      = kDefaultWidth;
    aDriver->mHeight     = kDefaultHeight;
    return aDriver;
}

} // namespace

// ---------------------------------------------------------------------------
// Exports
//
// The driver objects are intentionally never freed: they are referenced from
// raw slots inside SexyAppBase for the lifetime of the process, and the
// transcribed Cleanup dispatches through the deleting-destructor slot without
// freeing anything.
// ---------------------------------------------------------------------------
extern "C" Sexy::IAppDriver* Lawn_CreateAppDriver(Sexy::SexyAppBase* theApp)
{
    return MakeAppDriver(theApp);
}

extern "C" Sexy::IResStreamsDriver* Lawn_CreateResStreamsDriver()
{
    return new SDLResStreamsDriver();
}

extern "C" Sexy::IAudioDriver* Lawn_CreateAudioDriver(Sexy::SexyAppBase* theApp)
{
    (void)theApp;
    return new SDLAudioDriver();
}

extern "C" Sexy::IHttpDriver* Lawn_CreateHttpDriver(Sexy::SexyAppBase* theApp)
{
    (void)theApp;
    return new SDLHttpDriver();
}

extern "C" Sexy::IFileDriver* Lawn_CreateFileDriver(Sexy::SexyAppBase* theApp)
{
    (void)theApp;
    return new SDLFileDriver();
}

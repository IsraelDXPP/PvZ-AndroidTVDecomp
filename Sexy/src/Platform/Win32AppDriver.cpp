// Win32AppDriver.cpp -- the Windows platform drivers, with a hand-built vtable.
//
// WHY THE VTABLE IS BUILT BY HAND
// ------------------------------
// The binary is Android/ARM32, built with the Itanium C++ ABI, where a class
// with a virtual destructor spends TWO vtable slots on it (D2 complete-object
// and D0 deleting destructor). MSVC spends one. So a vtable produced by writing
// `virtual` declarations in C++ would put every method one slot early, and the
// transcribed bodies -- which dispatch through a raw byte offset taken off the
// driver pointer at SexyAppBase+748, such as `driver->vtbl_byte[120]` --
// would call the wrong function. Index N here is therefore the binary's slot
// number, and the array is filled positionally, never by C++ declaration order.
//
// CALLING CONVENTION
// ------------------
// The transcribed call sites all spell the slot as `__fastcall` with the driver
// in the first register slot, which on x86-32 is ECX -- the same register
// __thiscall uses for `this`. A `__fastcall` definition therefore matches, and
// every entry point below takes `void* theThis` first.
//
// ARITY -- READ THIS BEFORE EDITING
// ---------------------------------
// Under x86 `__fastcall` the CALLEE pops the stack arguments, so a slot's
// parameter count must match what its call site pushes. Two register arguments
// (this + one) are free: a one-parameter and a two-parameter function are
// ABI-identical. Three and four parameters are not, so only the slots whose
// call sites really do push stack arguments are declared that wide.
//
// The recon's call sites forward fewer arguments than the binary did -- IDA
// modelled only r0 in the 16-byte `ldr/ldr/bx` tail-call forwarders, so
// SexyAppBase::ReadBufferFromFile ends up calling slot 71 with `this` alone even
// though the binary's method takes (String const&, Buffer*, bool). The arities
// below follow the CALL SITES in this tree, not the binary, because those are
// what will actually execute. The slots with stack arguments are:
//
//     [ 27] HandleCmdLineParam    SexyAppBase.cpp:1986  this + 2
//     [ 36] SetCursorImage        SexyAppBase.cpp:2626  this + 2
//     [ 40] SwitchScreenMode      SexyAppBase.cpp:772   this + 3
//
// If a forwarder is ever fixed to pass its real arguments through, the matching
// slot here must be widened in the same change or the caller's stack unwinds
// wrongly.
//
// The vtable order below is the ground truth extracted from the binary's
// _ZTVN4Sexy13BaseAppDriverE @0x68C7D8 (97 slots, 0..96).

#include "Win32AppDriver.h"
#include "BaseAppDriver.h"
#include "GLImage.h"

#include "Audio/OpenALAudioBackend.h"
#include "GLWindow.h"
#include "GL/GLRenderBackend.h"
#include "Screenshot.h"

#include "AndroidShims.h"
#include "ReconHelpers.h"
#include "ReconTypes.h"
#include "SexyTypes.h"
#include "SexyAppBase.h"

// The driver's Start slot calls the app's Init (see Slot_Start below), so the
// concrete app type is needed here.
#include "Game/LawnApp.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "GL/GLRenderDevice.h"

#include <GLFW/glfw3.h>
#include <process.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Calling convention
// ---------------------------------------------------------------------------
#if defined(_M_IX86)
    #define LTVCALL __cdecl
#else
    // x64 has a single calling convention, so the hand-built table is an x86-32
    // ABI device and does not carry over. The macro keeps the file compilable
    // there for reference; the project itself builds x86 (see tools/check1.py).
    #define LTVCALL
#endif

namespace
{

// ---------------------------------------------------------------------------
// Object layout
//
// Only the first three words are contractual: the transcribed code reads the
// vtable pointer from offset 0 and the game reads nothing else. mApp is kept so
// a slot can reach back to the SexyAppBase, and mWindow is the display the
// driver owns.
// ---------------------------------------------------------------------------
enum Win32DriverKind
{
    WIN32DRV_APP        = 0,
    WIN32DRV_RESSTREAMS = 1,
    WIN32DRV_AUDIO      = 2,
    WIN32DRV_HTTP       = 3
};

struct Win32AppDriver : Sexy::BaseAppDriver
{
    void*                   mApp;           // the SexyAppBase, may be null
    Sexy::GLWindow*         mWindow;        // the display, may be null
    int                     mKind;
    bool                    mOwnsWindow;    // false when an outer owner made it
    Sexy::IGraphicsBackend* mGraphics;
    int                     mWidth;
    int                     mHeight;

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

struct Win32ResStreamsDriver : Sexy::IResStreamsDriver {};
struct Win32AudioDriver : Sexy::IAudioDriver {};
struct Win32HttpDriver : Sexy::IHttpDriver {};
struct Win32FileDriver : Sexy::IFileDriver {};

// Slots 0..96, from _ZTVN4Sexy13BaseAppDriverE @0x68C7D8.
const int kSlotCount = 97;

// A dumb driver's table is a placeholder with the same slot count, so an
// unexpected dispatch cannot run off the end of it.
const int kDumbSlotCount = 97;

// SexyAppBase::SexyAppBase @0x33F288 initialises mWidth at +1608 and mHeight at
// +1612 to 800x600 -- the same numbers used as the fallback below, so a null or
// unreadable app pointer costs nothing.
//
// TODO(recon): SexyAppBase.h models the object as opaque `mObject[0xB28]` and
// declares no mWidth/mHeight accessor, so these are raw offsets.
const int kOffsetAppWidth  = 1608;
const int kOffsetAppHeight = 1612;

const int kDefaultWidth  = 800;
const int kDefaultHeight = 600;

int LTVCALL Slot_DoUpdateFrames(void* theThis);
int LTVCALL Slot_DoUpdateFramesF(void* theThis, float theFrac);

// ---------------------------------------------------------------------------
// Generic slot bodies
//
// Every one of the 97 slots is filled: the game dispatches blindly, and a null
// entry would jump to address 0. "Fail" is the safe default for anything that
// hands a value back (0 = unsupported/none), "Ok" for anything that performs an
// action (1 = succeeded). The arity is part of the name because it is part of
// the ABI -- see the ARITY note at the top of the file.
// ---------------------------------------------------------------------------
int LTVCALL Stub_Fail1(void* theThis)
{
    (void)theThis;
    return 0;
}

int LTVCALL Stub_Ok1(void* theThis)
{
    (void)theThis;
    return 1;
}

int LTVCALL Stub_Fail2(void* theThis, void* a1)
{
    (void)theThis; (void)a1;
    return 0;
}

int LTVCALL Stub_Ok2(void* theThis, void* a1)
{
    (void)theThis; (void)a1;
    return 1;
}

int LTVCALL Stub_Fail3(void* theThis, void* a1, void* a2)
{
    (void)theThis; (void)a1; (void)a2;
    return 0;
}

int LTVCALL Stub_Ok3(void* theThis, void* a1, void* a2)
{
    (void)theThis; (void)a1; (void)a2;
    return 1;
}

int LTVCALL Stub_Fail4(void* theThis, void* a1, void* a2, void* a3)
{
    (void)theThis; (void)a1; (void)a2; (void)a3;
    return 0;
}

int LTVCALL Stub_Ok4(void* theThis, void* a1, void* a2, void* a3)
{
    (void)theThis; (void)a1; (void)a2; (void)a3;
    return 1;
}

// ---------------------------------------------------------------------------
// Window / display
// ---------------------------------------------------------------------------

// The size the app asked for, read out of the app object.
void AppDriver_ReadAppSize(void* theApp, int& theWidth, int& theHeight)
{
    theWidth  = kDefaultWidth;
    theHeight = kDefaultHeight;
    if (!theApp)
        return;

    const unsigned char* anApp = (const unsigned char*)theApp;
    int aWidth  = *(const int*)(anApp + kOffsetAppWidth);
    int aHeight = *(const int*)(anApp + kOffsetAppHeight);

    // Sanity bound: a garbage field must not become a 2-billion-pixel window.
    if (aWidth  > 0 && aWidth  <= 8192) theWidth  = aWidth;
    if (aHeight > 0 && aHeight <= 8192) theHeight = aHeight;
}

// Create the display if it does not exist yet.
//
// If a GL context is already current, somebody outside the driver (main.cpp
// creates a GLWindow before LawnApp) already owns the display, and opening a
// second GLFW window would just move the context out from under it. In that
// case the driver records the size and reuses the existing context.
void AppDriver_CenterScreen(Win32AppDriver* theSelf);

bool AppDriver_EnsureWindow(Win32AppDriver* theSelf)
{
    if (!theSelf)
        return false;
    if (theSelf->mWindow)
        return true;

    AppDriver_ReadAppSize(theSelf->mApp, theSelf->mWidth, theSelf->mHeight);

    if (glfwGetCurrentContext() != NULL)
    {
        theSelf->mOwnsWindow = false;
        int aWidth = 0;
        int aHeight = 0;
        GLFWwindow* aCurrent = glfwGetCurrentContext();
        glfwGetWindowSize(aCurrent, &aWidth, &aHeight);
        if (aWidth > 0 && aHeight > 0)
        {
            theSelf->mWidth = aWidth;
            theSelf->mHeight = aHeight;
        }
        TOD_TRACE("Win32AppDriver: reusing the current GL context, %dx%d",
                  theSelf->mWidth, theSelf->mHeight);
        return true;
    }

    Sexy::GLWindow* aWindow = new Sexy::GLWindow();
    if (!aWindow->Create("Plants vs. Zombies TV",
                         theSelf->mWidth, theSelf->mHeight,
                         true, true, true))
    {
        TOD_ERROR("Win32AppDriver: GLWindow::Create(%dx%d) failed",
                  theSelf->mWidth, theSelf->mHeight);
        delete aWindow;
        return false;
    }

    theSelf->mWindow = aWindow;
    theSelf->mOwnsWindow = true;
    TOD_TRACE("Win32AppDriver: created %dx%d", theSelf->mWidth, theSelf->mHeight);
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
void AppDriver_CenterScreen(Win32AppDriver* theSelf)
{
    if (theSelf == nullptr || theSelf->mApp == nullptr)
        return;

    Sexy::SexyAppBase* anApp = (Sexy::SexyAppBase*)theSelf->mApp;
    if (anApp->mWidgetManager == nullptr)
        return;

    int aDisplayWidth = theSelf->mWidth;
    int aDisplayHeight = theSelf->mHeight;

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

void AppDriver_DestroyWindow(Win32AppDriver* theSelf)
{
    if (!theSelf || !theSelf->mWindow)
        return;

    // GLFW itself stays initialised: it is torn down once, by ShutdownGLFW(),
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
void AppDriver_CreateGraphics(Win32AppDriver* theSelf)
{
    if (!theSelf || theSelf->mGraphics)
        return;

    // The binary picks its video driver through the same registry, reading
    // SEXY_VIDEO_DRIVER (BaseAppDriver::CreateGraphics @0x3f4384).
    theSelf->mGraphics =
        Sexy::GraphicsBackendFactory::Create(Sexy::GraphicsBackendFactory::GetRequestedName());
}

bool AppDriver_InitGraphics(Win32AppDriver* theSelf)
{
    if (!theSelf)
        return false;

    AppDriver_CreateGraphics(theSelf);
    if (!theSelf->mGraphics)
        return false;

    if (theSelf->mWindow)
        theSelf->mWindow->MakeCurrent();

    // A GL backend must not be initialised without a current context: glad
    // resolved its entry points against one, and GLRenderDevice::Init calls
    // them. Fall back to the software backend instead of crashing.
    bool aHaveContext = glfwGetCurrentContext() != NULL;
    bool anIsGL = strcmp(theSelf->mGraphics->GetName(), "OpenGL") == 0;

    bool anOk = false;
    if (!anIsGL || aHaveContext)
        anOk = theSelf->mGraphics->Init(theSelf->mWidth, theSelf->mHeight);

    if (!anOk)
    {
        TOD_WARN("Win32AppDriver: backend '%s' failed to init, using software",
                 theSelf->mGraphics->GetName());
        delete theSelf->mGraphics;
        theSelf->mGraphics = Sexy::GraphicsBackendFactory::Create("software");
        anOk = theSelf->mGraphics && theSelf->mGraphics->Init(theSelf->mWidth, theSelf->mHeight);
    }

    if (anOk)
        TOD_TRACE("Win32AppDriver: graphics backend = %s", theSelf->mGraphics->GetName());
    return anOk;
}

void AppDriver_DestroyGraphics(Win32AppDriver* theSelf)
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

// [0]/[1] ~BaseAppDriver() -- D2 and D0.
//
// The Itanium deleting destructor frees the object; here the driver lives for
// the whole process, and SexyAppBase::Cleanup only dispatches through the slot
// without freeing (see the operator delete next to the gFileDriver release).
void LTVCALL Slot_Destructor(void* theThis)
{
    (void)theThis;
}

// [2] InitAppDriver()
int LTVCALL Slot_InitAppDriver(void* theThis)
{
    (void)theThis;
    return 1;
}

// [3] Startup()
//
// The real one makes the window, creates the graphics driver and starts it.
// That is also the order the rest of the driver depends on, so the three steps
// are performed here rather than left to the caller.
int LTVCALL Slot_Startup(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (!self || self->mKind != WIN32DRV_APP)
        return 0;

    bool aHaveWindow = AppDriver_EnsureWindow(self);
    AppDriver_InitGraphics(self);
    // BaseAppDriver::InitGraphics @0x3F3644 sizes mScreenBounds after the
    // display exists. Startup is that point in this driver.
    AppDriver_CenterScreen(self);

    // The graphics backend is optional (the software one always succeeds); the
    // display is not.
    return aHaveWindow ? 1 : 0;
}

// [4] Terminate()
int LTVCALL Slot_Terminate(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (!self)
        return 0;
    AppDriver_DestroyGraphics(self);
    AppDriver_DestroyWindow(self);
    return 1;
}

// [5] Start()
//
// SexyAppBase::Start (0x336474) is a three-instruction forwarder: it reads the
// driver pointer from SexyAppBase+748, takes vtable byte 20 off it and tail-calls
// that slot, i.e. `driver->vtbl[5](driver)`. Slot 5 is where the app gets
// initialised, and this slot was returning 1 without doing anything -- so
// LawnApp::Init was never entered. Confirmed with the debugger:
//
//     bp LawnTV!LawnApp::Init   ->  never hit
//
// LawnApp::Init is where the game does its real setup: LoadResourceManifest,
// which parses properties/resources.xml via
// ResourceManager::ParseResourcesFile -> DoParseResources, then
// TodLoadResources("Init"), then `new TitleScreen(this)`. With the slot empty,
// none of that ran: the group table stayed empty, StartLoadResources computed its
// resource count as (end - begin) / 12 over nothing and reported "pulled 0
// resource(s)", and no title screen was ever created.
//
// LawnApp::Init has no direct callers in the binary at all, which is what a
// virtual call looks like; the driver's Start slot is the caller the forwarder
// above leads to.
// Report a fault with the faulting module and the fault-context stack.
//
// main.cpp's ReportFault is static to that translation unit, so this carries its
// own copy of the two parts that matter here: which module owns the instruction,
// and the return addresses above it. The module question is the one that took
// several rounds to answer the first time -- the rva printed by subtracting the
// executable's base is meaningless for an address inside a DLL, and an
// instruction address that stays constant across runs while every module base
// moves is a fixed-base system DLL, not this image.
static int ReportInitFault(unsigned int theCode, _EXCEPTION_POINTERS* theInfo)
{
    void* aAddr = theInfo->ExceptionRecord->ExceptionAddress;
    char aModule[MAX_PATH] = {0};
    unsigned long aRva = 0;

    MEMORY_BASIC_INFORMATION aInfo;
    if (::VirtualQuery(aAddr, &aInfo, sizeof(aInfo)) == sizeof(aInfo))
    {
        ::GetModuleFileNameA((HMODULE)aInfo.AllocationBase, aModule,
                             sizeof(aModule) - 1);
        aRva = (unsigned long)((char*)aAddr - (char*)aInfo.AllocationBase);
    }
    const char* aLeaf = strrchr(aModule, '\\');

    fprintf(stderr, "InitFault: code=0x%08lX instr=%p (%s+0x%lX)\n",
            (unsigned long)theCode, aAddr, aLeaf ? aLeaf + 1 : aModule, aRva);
    for (DWORD i = 0; i < theInfo->ExceptionRecord->NumberParameters && i < 2; ++i)
        fprintf(stderr, "InitFault:   info[%lu] = 0x%08lX\n", (unsigned long)i,
                (unsigned long)theInfo->ExceptionRecord->ExceptionInformation[i]);

    // Walk the frame chain from the fault context, not from the handler's own
    // stack.
    //
    // The first version read `_AddressOfReturnAddress()[i]` as if the return
    // addresses were a linear array on the stack. They are not: each frame's EBP
    // points at the saved EBP, with the return address four bytes above it, and
    // reading past that lands in locals and arguments. The output showed it --
    // the first entry came out as the exception code itself:
    //
    //     InitFault:   [01] C0000005  +0x40010005
    //
    // The exception context has the faulting EBP, so the chain can be followed
    // the way a debugger does. Every candidate is checked first: it has to live in
    // committed memory and grow upward, which stops a walk that runs into a frame
    // pointer that was never a frame pointer.
    unsigned long aFramePtr = (unsigned long)theInfo->ContextRecord->Ebp;
    unsigned long aPrevious = 0;
    for (int i = 0; i < 16 && aFramePtr != 0; ++i)
    {
        if (aFramePtr <= aPrevious || (aFramePtr & 3) != 0
            || aFramePtr < 0x10000u || aFramePtr > 0x7FF00000u)
            break;

        MEMORY_BASIC_INFORMATION aStackInfo;
        if (::VirtualQuery((void*)aFramePtr, &aStackInfo, sizeof(aStackInfo))
                != sizeof(aStackInfo)
            || aStackInfo.State != MEM_COMMIT)
            break;

        unsigned long* aFrame = (unsigned long*)aFramePtr;
        unsigned long aNext = aFrame[0];
        unsigned long aRet = aFrame[1];

        if (aRet >= 0x10000u)
        {
            char aM[MAX_PATH] = {0};
            unsigned long aR = 0;
            MEMORY_BASIC_INFORMATION aI;
            if (::VirtualQuery((void*)aRet, &aI, sizeof(aI)) == sizeof(aI))
            {
                ::GetModuleFileNameA((HMODULE)aI.AllocationBase, aM,
                                     sizeof(aM) - 1);
                aR = (unsigned long)(aRet - (unsigned long)aI.AllocationBase);
            }
            const char* aL = strrchr(aM, '\\');
            fprintf(stderr, "InitFault:   [%02d] ret=%p  %s+0x%lX\n", i,
                    (void*)aRet, aL ? aL + 1 : aM, aR);
        }

        aPrevious = aFramePtr;
        aFramePtr = aNext;
    }
    fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}

// The Init call, with a handler attached.
//
// Init is where the game does its real setup, and it is reached through the
// driver rather than through any of the wrappers in main.cpp -- so a fault inside
// it was ending the process with nothing reported at all:
//
//     step: LawnApp::Init entered
//     EXIT: -1073741819
//
// A separate function because __try cannot appear where objects need unwinding
// (C2712); nothing here does.
static int CallAppInit(LawnApp* theApp)
{
    __try
    {
        theApp->Init();
        return 1;
    }
    __except (ReportInitFault(GetExceptionCode(), GetExceptionInformation()))
    {
        return 0;
    }
}

int LTVCALL Slot_Start(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (!self || self->mApp == nullptr)
        return 0;

    CallAppInit(reinterpret_cast<LawnApp*>(self->mApp));
    return 1;
}

// [6] Init()
int LTVCALL Slot_Init(void* theThis)
{
    // The binary creates BOTH audio objects here, and this slot is where it
    // happens: BaseAppDriver::Init @0x3F3792 stores the sound driver's slot 3
    // result into `[app + 0x4E8]` (SexyAppBase::mSoundManager) and slot 4's into
    // `[app + 0x304]` (SexyAppBase::mMusicInterface), then falls back to a bare
    // `new MusicInterface()` at 0x3F37B6 when the music driver returns null.
    //
    // The binary reaches them through its audio driver object (SexyAppBase+756).
    // This port has no such driver -- the AG/audiere layer is replaced -- so the
    // two objects are constructed directly from the Windows backend. That keeps
    // the game's own call sites unchanged: they only ever go through
    // mSoundManager and mMusicInterface.
    //
    // Both are optional. If OpenAL cannot open a device, Init() returns false and
    // the members stay null, which is the state every transcribed body already
    // guards for (`if (mSoundManager == nullptr) return;`), so a machine with no
    // sound card still runs.
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (self && self->mApp)
    {
        Sexy::SexyAppBase* anApp = (Sexy::SexyAppBase*)self->mApp;

        if (anApp->mSoundManager == nullptr)
        {
            Sexy::OpenALSoundManager* aSoundManager = new Sexy::OpenALSoundManager;
            if (aSoundManager->Init())
            {
                anApp->mSoundManager = aSoundManager;
                // The music interface shares the manager's OpenAL context rather
                // than opening a second device. The app deletes it in Cleanup,
                // before the sound manager, so the manager must give up ownership.
                anApp->mMusicInterface = aSoundManager->GetMusicInterface();
                aSoundManager->mMusicInterface = nullptr;
                TOD_TRACE("Win32AppDriver: audio backend = OpenAL");
            }
            else
            {
                delete aSoundManager;
                TOD_WARN("Win32AppDriver: no audio device, running silent");
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
// fractional update, and DrawDirtyStuff.
int LTVCALL Slot_UpdateAppStep(void* theThis, void* theContinue)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (theContinue)
        *(bool*)theContinue = false;
    if (!self || !self->mApp)
        return 0;

    LawnApp* anApp = (LawnApp*)self->mApp;
    // Leave destruction to the outer Windows loop after callbacks and modal
    // waits have returned. Quit only requests this transition.
    if (anApp->mShutDown || (anApp->mExitToTop && anApp->mUpdateDepth == 0))
        return 0;

    Sexy::GLWindow* aWindow = self->mWindow != nullptr ? self->mWindow : Sexy::ActiveWindow();
    bool aModalWait = anApp->mUpdateDepth > 0;
    double frameStart = aWindow != nullptr ? aWindow->GetTime() : 0.0;
    if (aModalWait && aWindow != nullptr)
    {
        if (!aWindow->ProcessEvents())
            return 0;
        aWindow->MakeCurrent();
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
        Slot_DoUpdateFrames(self);
        Slot_DoUpdateFramesF(self, 1.0f);
        anApp->DrawDirtyStuff();
        if (theContinue)
            *(bool*)theContinue = anApp->mUpdateCount != anUpdateCount;
        anApp->mUpdateAppState = 0;

        // The outer Windows loop is suspended while a dialog waits. Present
        // the same offscreen target here and keep the normal frame cap.
        if (aModalWait && aWindow != nullptr)
        {
            if (Sexy::GLRenderDevice::sCurrent != nullptr)
                Sexy::GLRenderDevice::sCurrent->Present();
            aWindow->SwapBuffers();
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
int LTVCALL Slot_DoUpdateFramesF(void* theThis, float theFrac)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (!self || !self->mApp)
        return 0;
    LawnApp* anApp = (LawnApp*)self->mApp;
    if (anApp->mVSyncUpdates && !anApp->mMinimized && anApp->mWidgetManager != nullptr)
        anApp->mWidgetManager->UpdateFrameF(theFrac);
    return 1;
}

// [91] DoUpdateFrames()
//
// BaseAppDriver::DoUpdateFrames @0x3F39B8. The demo-buffer branch is the
// recorded-playback path; a live run takes the other one and calls UpdateFrames.
int LTVCALL Slot_DoUpdateFrames(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (!self || !self->mApp)
        return 0;
    LawnApp* anApp = (LawnApp*)self->mApp;

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
int LTVCALL Slot_DrawDirtyStuff(void* theThis, void* a1)
{
    (void)a1;
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (!self)
        return 0;

    LawnApp* anApp = (LawnApp*)self->mApp;
    if (anApp == nullptr || anApp->mWidgetManager == nullptr)
        return 0;

    if (self->mWindow)
        self->mWindow->MakeCurrent();

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
    bool aHardware = Sexy::GLRenderDevice::sCurrent != nullptr;
    bool aScreenIsHardware = dynamic_cast<Sexy::GLImage*>(aManager->mImage) != nullptr;
    if (aManager->mImage == nullptr || aHardware != aScreenIsHardware)
    {
        delete aManager->mImage;
        aManager->mImage = aHardware ? (Sexy::MemoryImage*)new Sexy::GLImage() : new Sexy::MemoryImage();
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
        Sexy::GLRenderDevice::sCurrent->SetDrawTarget(aManager->mImage);
        Sexy::GLRenderDevice::sCurrent->BeginFrame(Sexy::Color(0, 0, 0, 255));
    }
    int aDrew = aManager->DrawScreen();
    if (aHardware)
    {
        Sexy::GLRenderDevice::sCurrent->EndFrame();
        Sexy::GLRenderDevice::sCurrent->SetDrawTarget(nullptr);
    }
    anApp->mIsDrawing = false;
    if (aDrew == 0)
        return 0;

    // A software screen still has to be uploaded. A hardware screen already
    // drew its quads into the device, so uploading it again is the stutter.
    if (!aHardware && Sexy::GLRenderDevice::sCurrent != nullptr)
        Sexy::GLRenderDevice::sCurrent->BlitImage(aManager->mImage);
    return 1;
}

// [12] Shutdown()
int LTVCALL Slot_Shutdown(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
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
HANDLE gLoadingThread = nullptr;

unsigned __stdcall LoadingThreadMain(void* theArg)
{
    LawnApp* anApp = (LawnApp*)theArg;
    while (anApp != nullptr && !anApp->mShutDown && !anApp->mCloseRequest
           && !anApp->mLoadingThreadCompleted)
    {
        anApp->LoadingThreadProc();
        // One slice just ran. Yield so the main thread can paint the bar
        // even when this core is busy decoding the next batch.
        Sleep(1);
    }
    return 0;
}

// [28] StartLoadingThread()
//
// The first call still prepares the loader-bar art on this thread: the bar
// cannot draw until those images exist. Every later slice runs on the worker,
// so TitleScreen::Update can return and the frame loop can present the bar
// while images, fonts and sounds come in.
int LTVCALL Slot_StartLoadingThread(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (!self || !self->mApp)
        return 0;

    LawnApp* anApp = (LawnApp*)self->mApp;
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

    anApp->LoadingThreadProc();
    if (anApp->mLoadingThreadCompleted || anApp->mShutDown || anApp->mCloseRequest)
        return 1;

    unsigned aThreadId = 0;
    gLoadingThread = (HANDLE)_beginthreadex(nullptr, 0, LoadingThreadMain, anApp, 0, &aThreadId);
    if (gLoadingThread == nullptr)
        anApp->LoadingThreadProc();
    return 1;
}

// [29] WaitForLoadingThread()
int LTVCALL Slot_WaitForLoadingThread(void* theThis)
{
    (void)theThis;
    if (gLoadingThread == nullptr)
        return 1;
    WaitForSingleObject(gLoadingThread, INFINITE);
    CloseHandle(gLoadingThread);
    gLoadingThread = nullptr;
    return 1;
}

// [30] GetLoadingThreadProgress()
//
// The fraction the loading bar reads: completed tasks over the total the
// loading thread counted. Both live on the app, at the offsets the binary uses.
int LTVCALL Slot_GetLoadingThreadProgress(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    char *aApp = (char*)self->mApp;
    int aTotal = *reinterpret_cast<int *>(aApp + 0x588);
    int aDone = *reinterpret_cast<int *>(aApp + 0x58C);
    if (aTotal <= 0)
        return 0;
    return (int)(100.0 * aDone / aTotal);
}

// [37] EnforceCursor()
int LTVCALL Slot_EnforceCursor(void* theThis)
{
    (void)theThis;
    return 1;
}

// [41] KeyDown(int)
int LTVCALL Slot_KeyDown(void* theThis, void* theKey)
{
    (void)theThis; (void)theKey;
    return 0;
}

// [42] DebugKeyDown(int)
int LTVCALL Slot_DebugKeyDown(void* theThis, void* theKey)
{
    (void)theThis; (void)theKey;
    return 0;
}

// [44]/[45]/[46] Is3DAccelerated / Is3DAccelerationSupported /
// Is3DAccelerationRecommended
//
// Windows always has a GL 3.3 context available through GLWindow, and the game
// takes the accelerated path when these answer 1.
int LTVCALL Slot_Is3DAccelerated(void* theThis)
{
    (void)theThis;
    return 1;
}

int LTVCALL Slot_Is3DAccelerationSupported(void* theThis)
{
    (void)theThis;
    return 1;
}

int LTVCALL Slot_Is3DAccelerationRecommended(void* theThis)
{
    (void)theThis;
    return 1;
}

// [51] PreDrawScreen()
int LTVCALL Slot_PreDrawScreen(void* theThis)
{
    (void)theThis;
    return 1;
}

// [71] ReadBufferFromFile(String const&, Buffer*, bool)
//
// TODO(recon): this slot cannot be implemented as things stand. The binary's
// method is a 16-byte ARM tail call (`ldr/ldr/bx`) that forwards r0-r3
// untouched, but IDA modelled only r0, so the transcribed
// SexyAppBase::ReadBufferFromFile @0x338248 discards its own arguments and
// calls this slot with `this` alone -- neither the path nor the Buffer is
// reachable from here.
//
// Returning 0 makes the resource loaders report "file not found" and carry on,
// which is what they already do for a missing pak. The signature is deliberately
// one parameter wide: widening it here would make the callee pop stack words
// the current call site never pushed.
//
// To make it real: change SexyAppBase::ReadBufferFromFile to forward (a2, a3,
// a4), widen this entry to four parameters, and read the file with
// Sexy::Win32File::ReadFile() before handing the bytes to Buffer::SetData().
// [71] ReadBufferFromFile(const String&, Buffer*, bool)
//
// The real one reads the file and hands the bytes to the Buffer. It was a stub
// returning 0, and SexyAppBase::ReadBufferFromFile passed no arguments either, so
// every resource read failed regardless of path:
//
//     ParseResourcesFile -> ReadBufferFromFile("properties/resources.xml") -> 0
//       -> Fail("Resource file not found: ")
//
// The caller now forwards (path, buffer, flag), and the entry is widened to match.
// LTVCALL is __cdecl, so the extra words are caller-popped and the earlier
// one-parameter shape is not needed.
//
// Win32File::ReadFile resolves the path against the asset root as well as the
// working directory, so both the flat layout the build stages and a rooted one
// work.
int LTVCALL Slot_ReadBufferFromFile(void* theThis, const Sexy::String* thePath,
                                    Sexy::Buffer* theBuffer, int theFlag)
{
    (void)theThis;
    (void)theFlag;

    if (thePath == nullptr || theBuffer == nullptr)
        return 0;

    const char* aName = thePath->c_str();
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

    // TEMPORARY TRACE: how many bytes actually arrived.
    //
    // The parser reports a 512-byte source while resources.xml is 48753 bytes, so
    // either the read stopped early or the Buffer stored a different size. This
    // line separates the two.
    fprintf(stderr, "DRV_READ '%s' -> %u bytes\n", aName,
            (unsigned)aBytes.size());
    fflush(stderr);

    if (aBytes.empty())
        return 0;

    // SetData builds the byte vector at the Buffer's +4 field and sets the bit
    // count at +16 to 8 * size, which is what ParseResourcesFile reads back
    // through (data, bitSize) to build the string source.
    theBuffer->SetData(aBytes.data(), (int)aBytes.size());
    return 1;
}

// [74] ShouldPauseUpdates()
int LTVCALL Slot_ShouldPauseUpdates(void* theThis)
{
    (void)theThis;
    return 0;
}

// [75] RehupFocus()
int LTVCALL Slot_RehupFocus(void* theThis)
{
    (void)theThis;
    return 1;
}

// [86] GetMemoryInfo(MemoryInfo&)
//
// TODO(recon): the recon declares SexyAppBase::GetMemoryInfo as returning int
// with no MemoryInfo type anywhere in the tree, and the call site passes only
// `this`, so there is no out-parameter to fill. 0 = "unknown"; the callers
// already tolerate it.
int LTVCALL Slot_GetMemoryInfo(void* theThis)
{
    (void)theThis;
    return 0;
}

// [87] TakeScreenshot(MemoryImage&)
//
// This one does receive its argument: SexyAppBase::TakeScreenshot @0x337C80
// passes &a2 in the second register slot.
int LTVCALL Slot_TakeScreenshot(void* theThis, void* theImage)
{
    (void)theThis;
    if (!theImage)
        return 0;
    // Platform/Screenshot.h owns the numbered-file policy.
    return Sexy::Screenshot::SaveNext((Sexy::MemoryImage*)theImage) ? 1 : 0;
}

// [88] DoMainLoop()
//
// The frame loop lives in main.cpp, which owns the message pump; the driver
// only reports that the loop is entered and left normally.
int LTVCALL Slot_DoMainLoop(void* theThis)
{
    (void)theThis;
    return 1;
}

// [92] MakeWindow()
int LTVCALL Slot_MakeWindow(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (!self)
        return 0;
    return AppDriver_EnsureWindow(self) ? 1 : 0;
}

// [93] CreateGraphics()
int LTVCALL Slot_CreateGraphics(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (!self)
        return 0;
    AppDriver_CreateGraphics(self);
    return self->mGraphics ? 1 : 0;
}

// [94] InitGraphics()
int LTVCALL Slot_InitGraphics(void* theThis)
{
    return AppDriver_InitGraphics((Win32AppDriver*)theThis) ? 1 : 0;
}

// [95] DestroyGraphics()
int LTVCALL Slot_DestroyGraphics(void* theThis)
{
    AppDriver_DestroyGraphics((Win32AppDriver*)theThis);
    return 1;
}

// [96] UpdateInput()
int LTVCALL Slot_UpdateInput(void* theThis)
{
    Win32AppDriver* self = (Win32AppDriver*)theThis;
    if (self && self->mWindow)
        self->mWindow->ProcessEvents();
    return 1;
}

// ---------------------------------------------------------------------------
// The application driver vtable
//
// Index = the binary's slot number. Nothing may be null.
//
// The array is left unsized so the assertion below can see how many entries the
// initializer really has: a list that is one entry short would otherwise
// zero-fill the tail and leave a null slot, which is a jump to address 0 the
// first time the game dispatches through it.
// ---------------------------------------------------------------------------
int Win32AppDriver::InitAppDriver() { return Slot_InitAppDriver(this); }
int Win32AppDriver::Startup() { return Slot_Startup(this); }
int Win32AppDriver::Terminate() { return Slot_Terminate(this); }
int Win32AppDriver::Start() { return Slot_Start(this); }
int Win32AppDriver::Init() { return Slot_Init(this); }
int Win32AppDriver::UpdateAppStep(bool* theContinue) { return Slot_UpdateAppStep(this, theContinue); }
int Win32AppDriver::DrawDirtyStuff(bool theForce) { (void)theForce; return Slot_DrawDirtyStuff(this, nullptr); }
int Win32AppDriver::Shutdown() { return Slot_Shutdown(this); }
int Win32AppDriver::StartLoadingThread() { return Slot_StartLoadingThread(this); }
int Win32AppDriver::WaitForLoadingThread() { return Slot_WaitForLoadingThread(this); }
double Win32AppDriver::GetLoadingThreadProgress() { return (double)Slot_GetLoadingThreadProgress(this); }
int Win32AppDriver::EnforceCursor() { return Slot_EnforceCursor(this); }
int Win32AppDriver::KeyDown(int theKeyCode) { (void)theKeyCode; return Slot_KeyDown(this, nullptr); }
int Win32AppDriver::DebugKeyDown(int theKeyCode) { (void)theKeyCode; return Slot_DebugKeyDown(this, nullptr); }
int Win32AppDriver::Is3DAccelerated() { return Slot_Is3DAccelerated(this); }
int Win32AppDriver::Is3DAccelerationSupported() { return Slot_Is3DAccelerationSupported(this); }
int Win32AppDriver::Is3DAccelerationRecommended() { return Slot_Is3DAccelerationRecommended(this); }
int Win32AppDriver::PreDrawScreen() { return Slot_PreDrawScreen(this); }
int Win32AppDriver::ReadBufferFromFile(const Sexy::String& theFileName, Sexy::Buffer* theBuffer, bool theIsCompressed)
{
    return Slot_ReadBufferFromFile(this, &theFileName, theBuffer, theIsCompressed ? 1 : 0);
}
int Win32AppDriver::ShouldPauseUpdates() { return Slot_ShouldPauseUpdates(this); }
int Win32AppDriver::RehupFocus() { return Slot_RehupFocus(this); }
int Win32AppDriver::GetMemoryInfo(void* theMemoryInfo) { (void)theMemoryInfo; return Slot_GetMemoryInfo(this); }
int Win32AppDriver::TakeScreenshot(Sexy::MemoryImage* theImage) { return Slot_TakeScreenshot(this, theImage); }
int Win32AppDriver::DoMainLoop() { return Slot_DoMainLoop(this); }
int Win32AppDriver::DoUpdateFramesF(float theFTime) { return Slot_DoUpdateFramesF(this, theFTime); }
int Win32AppDriver::DoUpdateFrames() { return Slot_DoUpdateFrames(this); }
int Win32AppDriver::MakeWindow() { return Slot_MakeWindow(this); }
int Win32AppDriver::CreateGraphics() { return Slot_CreateGraphics(this); }
int Win32AppDriver::InitGraphics() { return AppDriver_InitGraphics(this) ? 1 : 0; }
int Win32AppDriver::DestroyGraphics() { return Slot_DestroyGraphics(this); }
int Win32AppDriver::UpdateInput() { return Slot_UpdateInput(this); }

Win32AppDriver* MakeAppDriver(void* theApp)
{
    Win32AppDriver* aDriver = new Win32AppDriver();
    aDriver->mApp        = theApp;
    aDriver->mWindow     = nullptr;
    aDriver->mKind       = WIN32DRV_APP;
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
extern "C" void* LawnTV_CreateAppDriver(void* theApp)
{
    return MakeAppDriver(theApp);
}

extern "C" void* LawnTV_CreateResStreamsDriver()
{
    return new Win32ResStreamsDriver();
}

extern "C" void* LawnTV_CreateAudioDriver(void* theApp)
{
    (void)theApp;
    return new Win32AudioDriver();
}

extern "C" void* LawnTV_CreateHttpDriver(void* theApp)
{
    (void)theApp;
    return new Win32HttpDriver();
}

extern "C" void* LawnTV_CreateFileDriver(void* theApp)
{
    (void)theApp;
    return new Win32FileDriver();
}

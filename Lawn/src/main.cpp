// main.cpp -- SDL3 callback entry point.
//
//   LawnTV.exe [--assets <dir>] [--width N] [--height N]
//              [--frames N] [--screenshot <file>] [--headless] [-tod]
//
// F12 writes the offscreen frame to screenshots/shot_NNNN.png.
// --screenshot reads that same offscreen target, so it works with --headless.

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "LawnApp.h"
#include "Platform/MpegVideo.h"
#include "Platform/SDLWindow.h"
#include "Platform/Bgfx/BgfxRenderDevice.h"
#include "Platform/Screenshot.h"
#include "Platform/SDLFile.h"
#include "PakLib/FileSystemManager.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "SexyTypes.h"

#include <atomic>
#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

static void LogLine(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    fputs(buffer, stderr);
    fputc('\n', stderr);
}

struct LaunchOptions
{
    std::string assetRoot = "assets";
    std::string screenshotPath;
    int width = 1280;
    int height = 720;
    int frames = 0;
    bool headless = false;
    bool debugKeys = false;
};

struct ApplicationState
{
    LaunchOptions options;
    Sexy::SDLWindow window;
    std::unique_ptr<LawnApp> app;
    std::unique_ptr<Sexy::MemoryImage> capture;
    int frameCount = 0;
    int callbackRate = -1;
    std::atomic<bool> success{true};
};

static bool TakeValue(const std::vector<std::string>& args, size_t& index, std::string& value)
{
    if (index + 1 >= args.size())
        return false;
    value = args[++index];
    return true;
}

static LaunchOptions ParseCommandLine(int argc, char** argv)
{
    LaunchOptions options;
    std::vector<std::string> args(argv, argv + argc);

    for (size_t i = 1; i < args.size(); ++i)
    {
        const std::string& arg = args[i];
        std::string value;
        if (arg == "--assets" && TakeValue(args, i, value))
            options.assetRoot = value;
        else if (arg == "--width" && TakeValue(args, i, value))
            options.width = atoi(value.c_str());
        else if (arg == "--height" && TakeValue(args, i, value))
            options.height = atoi(value.c_str());
        else if (arg == "--frames" && TakeValue(args, i, value))
            options.frames = atoi(value.c_str());
        else if (arg == "--screenshot" && TakeValue(args, i, value))
        {
            options.screenshotPath = value;
            options.headless = true;
            if (options.frames <= 0)
                options.frames = 1;
        }
        else if (arg == "--headless" || arg == "--test-pattern")
            options.headless = true;
        else if (arg == "-tod" || arg == "--debug-keys")
            options.debugKeys = true;
    }
    return options;
}

// Assets live next to the executable, so a double-click, a shell in another
// directory, and a debugger all resolve the same files.
static std::string ExecutableDirectory()
{
    const char* directory = SDL_GetBasePath();
    return directory ? directory : "";
}

static void ResolveAssetRoot(LaunchOptions& options)
{
    if (options.assetRoot != "assets")
        return;
    std::string directory = ExecutableDirectory();
    if (!directory.empty())
        options.assetRoot = directory + "/assets";
}

static bool PresentFrame(Sexy::SDLWindow& window, Sexy::BgfxRenderDevice& device,
                         Sexy::MemoryImage& capture, const LaunchOptions& options, int frameIndex)
{
    bool screenshotKeyDown = window.IsKeyDown(KEYCODE_F12);
    bool hotkeyCapture = Sexy::ScreenshotHotkey::IsCaptureRequested(screenshotKeyDown);
    bool finalCapture = !options.screenshotPath.empty() &&
        (options.frames <= 0 || frameIndex + 1 >= options.frames);
    // Readback synchronizes the GPU and copies the whole frame. Presentation
    // can use the FBO directly; only an actual screenshot needs CPU pixels.
    bool captured = (hotkeyCapture || finalCapture) && device.ReadPixels(capture);
    Sexy::ScreenshotHotkey::Poll(captured ? &capture : nullptr, screenshotKeyDown);
    bool success = !finalCapture || captured;
    if (finalCapture && captured)
    {
        bool saved = Sexy::Screenshot::Save(&capture, options.screenshotPath);
        success = saved;
        LogLine("screenshot: %s (%dx%d) -> %s",
                saved ? "wrote" : "failed",
                capture.mWidth, capture.mHeight,
                options.screenshotPath.c_str());
    }

    device.Present();
    return success;
}

static void UpdateCallbackRate(ApplicationState& state)
{
    int maxFPS = MpegVideoIsPlaying() != 0 ? 30 : state.app->mMaxFPS;
    if (maxFPS < 0)
        maxFPS = 0;
    if (state.callbackRate != maxFPS)
    {
        // SDL controls scheduling; callbacks do one frame and return without
        // sleeping. A zero rate preserves SEXY_MAX_FPS=0 (uncapped).
        std::string rate = std::to_string(maxFPS);
        SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, rate.c_str());
        state.callbackRate = maxFPS;
    }
}

SDL_AppResult SDLCALL SDL_AppInit(void** appstate, int argc, char** argv)
{
    ApplicationState* application = new ApplicationState();
    // SDL_AppQuit is also called when initialization fails. Transfer ownership
    // before acquiring resources so every exit uses the same cleanup path.
    *appstate = application;

    application->options = ParseCommandLine(argc, argv);
    LaunchOptions& options = application->options;
    ResolveAssetRoot(options);
    Sexy::SDLFile::SetAssetRoot(options.assetRoot);
    LogLine("Lawn: asset root = %s", Sexy::SDLFile::GetAssetRoot().c_str());

    if (!application->window.Create("Plants vs. Zombies", options.width, options.height,
                                    true, true, !options.headless))
    {
        LogLine("Lawn: failed to create the window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    application->app = std::make_unique<LawnApp>();
    LawnApp& app = *application->app;
    if (options.debugKeys)
        app.HandleCmdLineParam("-tod", "");
    app.HardwareInit();
    app.Startup();

    if (Sexy::BgfxRenderDevice::sCurrent == nullptr)
    {
        LogLine("Lawn: graphics startup failed; check SEXY_VIDEO_DRIVER and assets/shaders");
        return SDL_APP_FAILURE;
    }
    application->capture = std::make_unique<Sexy::MemoryImage>();
    app.Start();
    UpdateCallbackRate(*application);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL SDL_AppIterate(void* appstate)
{
    ApplicationState& state = *static_cast<ApplicationState*>(appstate);
    if (state.window.mClosed)
        return state.success ? SDL_APP_SUCCESS : SDL_APP_FAILURE;

    // SDL has already delivered this iteration's events through SDL_AppEvent.
    state.window.UpdateFrame();
    // The first step drains game input; the second updates widgets and draws.
    if (!state.app->UpdateAppStep(nullptr) || !state.app->UpdateAppStep(nullptr))
        return state.success ? SDL_APP_SUCCESS : SDL_APP_FAILURE;

    if (Sexy::BgfxRenderDevice::sCurrent != nullptr)
        state.success = PresentFrame(state.window, *Sexy::BgfxRenderDevice::sCurrent,
                                     *state.capture, state.options, state.frameCount) && state.success;

    ++state.frameCount;
    if (state.options.frames > 0 && state.frameCount >= state.options.frames)
        return state.success ? SDL_APP_SUCCESS : SDL_APP_FAILURE;

    UpdateCallbackRate(state);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL SDL_AppEvent(void* appstate, SDL_Event* event)
{
    ApplicationState& state = *static_cast<ApplicationState*>(appstate);
    state.window.ProcessEvent(*event);
    if (state.window.mClosed)
        return state.success ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
    return SDL_APP_CONTINUE;
}

void SDLCALL SDL_AppQuit(void* appstate, SDL_AppResult)
{
    std::unique_ptr<ApplicationState> state(static_cast<ApplicationState*>(appstate));
    if (!state)
        return;

    LogLine("Lawn: exited after %d frame(s)", state->frameCount);
    if (state->app)
    {
        // Loading and video must stop while the app and graphics still exist.
        state->app->mCloseRequest = true;
        state->app->WaitForLoadingThread();
        MpegVideoClose();
        state->capture.reset();
        state->app->Shutdown();
        state->app.reset();
    }
    PakLib::FileSystemManager::getManager()->release();
    // The state destroys the window last. SDL calls SDL_Quit after this returns.
}

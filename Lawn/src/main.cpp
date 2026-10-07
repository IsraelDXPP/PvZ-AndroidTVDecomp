// main.cpp -- SDL3 entry point.
//
//   LawnTV.exe [--assets <dir>] [--width N] [--height N]
//              [--frames N] [--screenshot <file>] [--headless]
//
// F12 writes the offscreen frame to screenshots/shot_NNNN.png.
// --screenshot reads that same offscreen target, so it works with --headless.

#include "LawnApp.h"
#include "Platform/MpegVideo.h"
#include "Platform/SDLWindow.h"
#include "Platform/Bgfx/BgfxRenderDevice.h"
#include "Platform/Screenshot.h"
#include "Platform/SDLFile.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "SexyTypes.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
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
#ifdef _DEBUG
    bool debugKeys = false;
#endif
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
#ifdef _DEBUG
        else if (arg == "-tod")
            options.debugKeys = true;
#endif
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

static int Run(const LaunchOptions& options)
{
    Sexy::SDLWindow window;
    if (!window.Create("Plants vs. Zombies TV", options.width, options.height,
                       true, true, !options.headless))
    {
        LogLine("LawnTV: failed to create the window");
        return 1;
    }

    LawnApp* app = new LawnApp();
#ifdef _DEBUG
    if (options.debugKeys)
        app->HandleCmdLineParam("-tod", "");
#endif
    app->HardwareInit();
    app->Startup();

    Sexy::MemoryImage capture;

    if (Sexy::BgfxRenderDevice::sCurrent == nullptr)
    {
        LogLine("LawnTV: graphics startup failed; check SEXY_VIDEO_DRIVER and assets/shaders");
        app->mCloseRequest = true;
        app->WaitForLoadingThread();
        app->Shutdown();
        delete app;
        return 1;
    }
    app->Start();
    bool success = true;

    int frames = 0;
    while (!window.mClosed)
    {
        // SEXY_MAX_FPS is the frame cap SexyAppBase reads at construction.
        // The default is 60; an explicit zero leaves the loop uncapped.
        double frameStart = window.GetTime();

        if (!window.ProcessEvents())
            break;



        // UpdateAppStep returns false when the app has shut down. The bool it
        // writes means "a widget update ran", and the first call of a pair
        // only drains input, so the frame takes both calls.
        if (!app->UpdateAppStep(nullptr))
            break;
        if (!app->UpdateAppStep(nullptr))
            break;

        if (Sexy::BgfxRenderDevice::sCurrent != nullptr)
            success = PresentFrame(window, *Sexy::BgfxRenderDevice::sCurrent, capture, options, frames) && success;



        ++frames;
        if (options.frames > 0 && frames >= options.frames)
            break;

        // The intro runs at the movie rate. Other screens use the optional
        // SEXY_MAX_FPS cap and the window's vertical synchronization.
        int aMaxFps = app->mMaxFPS;
        if (MpegVideoIsPlaying() != 0)
            aMaxFps = 30;
        window.WaitForFrame(frameStart, aMaxFps);
    }

    LogLine("LawnTV: exited after %d frame(s)", frames);
    // Stop background loading before the graphics device is destroyed.
    app->mCloseRequest = true;
    app->WaitForLoadingThread();
    MpegVideoClose();
    app->Shutdown();
    delete app;
    return success ? 0 : 1;
}

int main(int argc, char** argv)
{
    LaunchOptions options = ParseCommandLine(argc, argv);
    ResolveAssetRoot(options);
    Sexy::SDLFile::SetAssetRoot(options.assetRoot);
    LogLine("LawnTV: asset root = %s", Sexy::SDLFile::GetAssetRoot().c_str());

    int result = Run(options);
    Sexy::ShutdownSDL();
    return result;
}

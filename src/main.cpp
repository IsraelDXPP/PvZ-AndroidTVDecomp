// main.cpp -- Windows entry point.
//
//   LawnTV.exe [--assets <dir>] [--width N] [--height N]
//              [--frames N] [--screenshot <file>] [--headless]
//
// F12 writes the offscreen frame to screenshots/shot_NNNN.png.
// --screenshot reads that same offscreen target, so it works with --headless.

#include "LawnApp.h"
#include "Platform/GLWindow.h"
#include "Platform/GL/GLRenderDevice.h"
#include "Platform/Screenshot.h"
#include "Platform/Win32File.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "SexyTypes.h"

#include <windows.h>
#include <shellapi.h>
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
    _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    va_end(args);

    OutputDebugStringA(buffer);
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
};

static std::string NarrowArgument(const wchar_t* text)
{
    if (text == nullptr)
        return {};
    int bytes = WideCharToMultiByte(CP_ACP, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (bytes <= 1)
        return {};
    std::string narrow(static_cast<size_t>(bytes - 1), '\0');
    WideCharToMultiByte(CP_ACP, 0, text, -1, &narrow[0], bytes, nullptr, nullptr);
    return narrow;
}

static bool TakeValue(const std::vector<std::string>& args, size_t& index, std::string& value)
{
    if (index + 1 >= args.size())
        return false;
    value = args[++index];
    return true;
}

static LaunchOptions ParseCommandLine()
{
    LaunchOptions options;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr)
        return options;

    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i)
        args.push_back(NarrowArgument(argv[i]));
    LocalFree(argv);

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
    }
    return options;
}

// Assets live next to the executable, so a double-click, a shell in another
// directory, and a debugger all resolve the same files.
static std::string ExecutableDirectory()
{
    char path[MAX_PATH] = {};
    DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return {};
    std::string directory(path, length);
    size_t slash = directory.find_last_of("\\/");
    if (slash == std::string::npos)
        return {};
    return directory.substr(0, slash);
}

static void ResolveAssetRoot(LaunchOptions& options)
{
    if (options.assetRoot != "assets")
        return;
    std::string directory = ExecutableDirectory();
    if (!directory.empty())
        options.assetRoot = directory + "/assets";
}

static void PresentFrame(Sexy::GLWindow& window, Sexy::GLRenderDevice& device,
                         Sexy::MemoryImage& capture, const LaunchOptions& options, int frameIndex)
{
    int width = window.mWidth > 0 ? window.mWidth : options.width;
    int height = window.mHeight > 0 ? window.mHeight : options.height;
    device.Resize(width, height);

    if (!device.ReadPixels(capture))
    {
        device.Present();
        return;
    }

    Sexy::ScreenshotHotkey::Poll(&capture, window.IsKeyDown(KEYCODE_F12));
    if (!options.screenshotPath.empty() &&
        (options.frames <= 0 || frameIndex + 1 >= options.frames))
    {
        bool saved = Sexy::Screenshot::Save(&capture, options.screenshotPath);
        LogLine("screenshot: %s (%dx%d) -> %s",
                saved ? "wrote" : "failed",
                capture.mWidth, capture.mHeight,
                options.screenshotPath.c_str());
    }

    device.Present();
}

static int Run(const LaunchOptions& options)
{
    Sexy::GLWindow window;
    if (!window.Create("Plants vs. Zombies TV", options.width, options.height,
                       true, true, !options.headless))
    {
        LogLine("LawnTV: failed to create the window");
        return 1;
    }

    LawnApp* app = new LawnApp();
    app->HardwareInit();
    app->Startup();
    app->Start();

    Sexy::MemoryImage capture;
    capture.Create(options.width, options.height);

    // The game draws into whichever device startup installed. A headless run
    // leaves that pointer empty, so this one fills in.
    Sexy::GLRenderDevice fallbackDevice;
    bool reportedDeviceFailure = false;

    int frames = 0;
    while (!window.mClosed)
    {
        if (!window.ProcessEvents())
            break;

        window.MakeCurrent();

        // UpdateAppStep returns false when the app has shut down. The bool it
        // writes means "a widget update ran", and the first call of a pair
        // only drains input, so the frame takes both calls.
        if (!app->UpdateAppStep(nullptr))
            break;
        if (!app->UpdateAppStep(nullptr))
            break;

        if (Sexy::GLRenderDevice::sCurrent == nullptr && !fallbackDevice.mInitialized)
        {
            if (!fallbackDevice.Init(options.width, options.height) && !reportedDeviceFailure)
            {
                LogLine("LawnTV: failed to create the render device");
                reportedDeviceFailure = true;
            }
        }
        if (Sexy::GLRenderDevice::sCurrent != nullptr)
            PresentFrame(window, *Sexy::GLRenderDevice::sCurrent, capture, options, frames);

        window.SwapBuffers();

        ++frames;
        if (options.frames > 0 && frames >= options.frames)
            break;
    }

    LogLine("LawnTV: exited after %d frame(s)", frames);
    app->Shutdown();
    delete app;
    return 0;
}

int main()
{
    LaunchOptions options = ParseCommandLine();
    ResolveAssetRoot(options);
    Sexy::Win32File::SetAssetRoot(options.assetRoot);
    LogLine("LawnTV: asset root = %s", Sexy::Win32File::GetAssetRoot().c_str());

    int result = Run(options);
    Sexy::ShutdownGLFW();
    return result;
}

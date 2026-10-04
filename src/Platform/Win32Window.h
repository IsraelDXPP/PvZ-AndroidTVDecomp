// Win32Window.h -- Windows window + blit target.
//
// This replaces the Android display stack. In the binary the equivalent role is
// played by Sexy::AndroidDisplay / Sexy::AndroidGraphicsDriver behind the
// Sexy::VideoDriver registry (registered as "Android", priority 10), which
// BaseAppDriver::CreateGraphics() selects via the SEXY_VIDEO_DRIVER
// environment variable.
//
// We keep the same shape -- a driver object that owns the native surface --
// so a future GPU backend can slot in without touching game code.

#ifndef __WIN32WINDOW_H__
#define __WIN32WINDOW_H__

#include "CoreTypes.h"
#include "MemoryImage.h"
#include <windows.h>

namespace Sexy
{

class Win32Window
{
public:
    HWND            mHWnd;
    HDC             mHDC;
    MemoryImage*    mScreenImage;   // 32-bit ARGB back buffer
    int             mWidth;
    int             mHeight;
    bool            mWindowed;
    bool            mActive;
    bool            mClosed;

    // Timing (the Android build drives frames from BaseAppDriver::DoMainLoop).
    LARGE_INTEGER   mPerfFreq;
    LARGE_INTEGER   mLastTime;
    LARGE_INTEGER   mStartTime;
    double          mFrameTime;     // seconds since previous frame

public:
    Win32Window();
    ~Win32Window();

    bool    Create(const char* theTitle, int theWidth, int theHeight, bool windowed);
    void    Destroy();

    // Pump the message queue; returns false when the app should exit.
    bool    ProcessMessages();

    // Present the back buffer to the window.
    void    Present();

    // Timing helpers.
    void    ResetTimer();
    double  GetTime() const;
    double  GetFrameTime() const { return mFrameTime; }

    MemoryImage* GetScreenImage() { return mScreenImage; }

    // Set by the window procedure when the client area changes.
    void    OnResize(int theWidth, int theHeight);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

} // namespace Sexy

#endif // __WIN32WINDOW_H__

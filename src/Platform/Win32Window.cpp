// Win32Window.cpp -- Win32 window creation, message pump and presentation.

#include "Win32Window.h"
#include "SexyTypes.h"
#include "AndroidShims.h"
#include <windows.h>
#include <cstring>

namespace Sexy
{

// Set while a Win32Window is being created so WndProc can find its instance.
static Win32Window* gCreatingWindow = nullptr;

Win32Window::Win32Window()
    : mHWnd(nullptr), mHDC(nullptr), mScreenImage(nullptr),
      mWidth(0), mHeight(0), mWindowed(true), mActive(true), mClosed(false),
      mFrameTime(0.0)
{
    ::QueryPerformanceFrequency(&mPerfFreq);
    ResetTimer();
}

Win32Window::~Win32Window()
{
    Destroy();
}

void Win32Window::ResetTimer()
{
    ::QueryPerformanceCounter(&mLastTime);
    mStartTime = mLastTime;
}

double Win32Window::GetTime() const
{
    LARGE_INTEGER now;
    ::QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - mStartTime.QuadPart) / (double)mPerfFreq.QuadPart;
}

bool Win32Window::Create(const char* theTitle, int theWidth, int theHeight, bool windowed)
{
    mWidth = theWidth;
    mHeight = theHeight;
    mWindowed = windowed;

    HINSTANCE hInst = ::GetModuleHandleA(nullptr);

    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = &Win32Window::WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = ::LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "LawnTVDecompWindow";

    if (!::RegisterClassExA(&wc))
    {
        DWORD err = ::GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS)
        {
            TOD_ERROR("RegisterClassEx failed (%lu)", err);
            return false;
        }
    }

    DWORD style   = windowed ? (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX)
                             : WS_POPUP;
    DWORD exStyle = WS_EX_APPWINDOW;

    RECT r = { 0, 0, theWidth, theHeight };
    ::AdjustWindowRectEx(&r, style, FALSE, exStyle);

    gCreatingWindow = this;

    mHWnd = ::CreateWindowExA(exStyle, wc.lpszClassName, theTitle, style,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              r.right - r.left, r.bottom - r.top,
                              nullptr, nullptr, hInst, nullptr);

    gCreatingWindow = nullptr;

    if (!mHWnd)
    {
        TOD_ERROR("CreateWindowEx failed (%lu)", ::GetLastError());
        return false;
    }

    mHDC = ::GetDC(mHWnd);

    // The back buffer is a 32-bit ARGB software surface -- the same pixel
    // format the reconstructed MemoryImage uses.
    mScreenImage = new MemoryImage();
    mScreenImage->Create(theWidth, theHeight);

    ::ShowWindow(mHWnd, SW_SHOW);
    ::UpdateWindow(mHWnd);

    ResetTimer();
    return true;
}

void Win32Window::Destroy()
{
    if (mScreenImage)
    {
        delete mScreenImage;
        mScreenImage = nullptr;
    }
    if (mHDC && mHWnd)
    {
        ::ReleaseDC(mHWnd, mHDC);
        mHDC = nullptr;
    }
    if (mHWnd)
    {
        ::DestroyWindow(mHWnd);
        mHWnd = nullptr;
    }
}

void Win32Window::OnResize(int theWidth, int theHeight)
{
    if (theWidth <= 0 || theHeight <= 0) return;
    if (theWidth == mWidth && theHeight == mHeight) return;

    mWidth = theWidth;
    mHeight = theHeight;

    if (mScreenImage)
        mScreenImage->Create(theWidth, theHeight);
}

bool Win32Window::ProcessMessages()
{
    MSG msg;
    while (::PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;

        ::TranslateMessage(&msg);
        ::DispatchMessageA(&msg);
    }

    if (mClosed)
        return false;

    // Frame timing: measure the interval since the last pump.
    LARGE_INTEGER now;
    ::QueryPerformanceCounter(&now);
    mFrameTime = (double)(now.QuadPart - mLastTime.QuadPart) / (double)mPerfFreq.QuadPart;
    mLastTime = now;

    // Clamp so a stall (debugger break, window drag) does not fast-forward
    // the simulation by seconds.
    if (mFrameTime > 0.25) mFrameTime = 0.25;

    return true;
}

void Win32Window::Present()
{
    if (!mHWnd || !mHDC || !mScreenImage || !mScreenImage->mBits) return;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = mScreenImage->mWidth;
    // Negative height => top-down DIB, matching the ARGB buffer's row order.
    bmi.bmiHeader.biHeight      = -mScreenImage->mHeight;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    ::StretchDIBits(mHDC,
                    0, 0, mWidth, mHeight,
                    0, 0, mScreenImage->mWidth, mScreenImage->mHeight,
                    mScreenImage->mBits, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Win32Window* self = gCreatingWindow;

    switch (msg)
    {
    case WM_CREATE:
        // Associate the instance with the window so later messages can find it.
        {
            CREATESTRUCTA* cs = (CREATESTRUCTA *)(intptr_t)lParam;
            ::SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        }
        return 0;

    case WM_SIZE:
        if (self)
            self->OnResize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_ACTIVATE:
        if (self)
            self->mActive = (LOWORD(wParam) != WA_INACTIVE);
        return 0;

    case WM_CLOSE:
        if (self) self->mClosed = true;
        ::DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND:
        return 1;   // we repaint the whole client area every frame

    default:
        break;
    }

    return ::DefWindowProcA(hWnd, msg, wParam, lParam);
}

} // namespace Sexy

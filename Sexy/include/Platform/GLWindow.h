// GLWindow.h -- GLFW window with an OpenGL 3.3 core context.
//
// Replaces the Android display stack. In the binary the equivalents are:
//
//     Sexy::AndroidDisplay          -- surface lifecycle, EGL context, key map
//     Sexy::AndroidGraphicsDriver   -- owns the GL context and the render device
//     Sexy::VideoDriver registry    -- "Android" driver, priority 10, chosen by
//                                      BaseAppDriver::CreateGraphics() from the
//                                      SEXY_VIDEO_DRIVER environment variable
//
// We keep that shape: GLWindow owns the native window + context, and hands the
// context to GLRenderDevice. A future driver-registry implementation can select
// between this and a software device without touching game code.

#ifndef __GLWINDOW_H__
#define __GLWINDOW_H__

#include "GLHeaders.h"
#include "CoreTypes.h"

struct GLFWwindow;

namespace Sexy
{

struct Event;

class GLWindow
{
public:
    GLFWwindow*     mWindow;
    int             mWidth;
    int             mHeight;
    bool            mWindowed;
    bool            mActive;
    bool            mClosed;
    bool            mVSync;

    // Frame timing (the Android build drove frames from
    // BaseAppDriver::DoMainLoop; we keep the same explicit per-frame step).
    double          mLastTime;
    double          mStartTime;
    double          mFrameTime;

public:
    GLWindow();
    ~GLWindow();

    // Create the window and a GL 3.3 core context. Returns false on failure.
    // `theVisible = false` creates a hidden window, which still provides a real
    // GL context -- used by the headless GL test so it can run unattended.
    bool    Create(const char* theTitle, int theWidth, int theHeight,
                   bool theWindowed = true, bool theVSync = true,
                   bool theVisible = true);
    void    Destroy();

    // Pump events; returns false when the app should exit.
    bool    ProcessEvents();

    // Swap buffers.
    void    SwapBuffers();

    void    MakeCurrent();

    void    SetVSync(bool theVSync);
    void    SetTitle(const char* theTitle);

    double  GetTime() const;
    double  GetFrameTime() const { return mFrameTime; }
    void    ResetTimer();

    // True when the given framework key is held.
    bool    IsKeyDown(int theKeyCode) const;

    // GLFW key callback. GetAsyncKeyState does not see keys while this
    // window has focus, so IsKeyDown reads the state this callback stores.
    // Press and release also become Event records. InputManager::Update
    // drains them the same way AndroidInput::HandleKeyEvents posted keys.
    static void KeyCallback(GLFWwindow* theWindow, int theKey, int theScancode,
                            int theAction, int theMods);
    static bool IsGlfwKeyDown(int theGlfwKey);
    bool    PopKeyEvent(Event& theEvent);

    // Mouse buttons. GLFW reports these through a separate API from keys, so
    // they cannot be reached via IsKeyDown.
    enum MouseButton { MOUSE_LEFT = 0, MOUSE_RIGHT = 1, MOUSE_MIDDLE = 2 };
    bool    IsMouseButtonDown(int theButton) const;

    // Cursor position in client coordinates.
    void    GetCursorPos(double& theX, double& theY) const;

    static  void    ErrorCallback(int theError, const char* theDescription);
};

// The window whose callback is filling the key queue. InputManager drains it
// after glfwPollEvents. A null window means no keyboard is attached.
GLWindow* ActiveWindow();

// GLFW is initialised lazily by GLWindow::Create and must be torn down once,
// after every window is gone. Call this at process exit.
void ShutdownGLFW();

} // namespace Sexy

#endif // __GLWINDOW_H__

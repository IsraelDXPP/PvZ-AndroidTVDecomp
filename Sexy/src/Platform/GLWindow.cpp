// GLWindow.cpp -- GLFW window and GL context management.

#include "GLWindow.h"
#include "SexyTypes.h"
#include "CoreTypes.h"

#include "AndroidShims.h"
#include "ReconTypes.h"

// Event lives in ReconTypes.h and is also pulled in as a global name. This
// file is the one place the queue is concrete, so the header only forward
// declares it.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

namespace Sexy
{

struct GLFrameTimer
{
    HANDLE mHandle;

    GLFrameTimer()
        : mHandle(CreateWaitableTimerExW(nullptr, nullptr, 0x2, TIMER_ALL_ACCESS))
    {
        // CREATE_WAITABLE_TIMER_HIGH_RESOLUTION is available since Windows 10 1803.
        if (mHandle == nullptr)
            mHandle = CreateWaitableTimerW(nullptr, FALSE, nullptr);
    }

    ~GLFrameTimer()
    {
        if (mHandle != nullptr)
            CloseHandle(mHandle);
    }
};

static bool gGLFWInitialized = false;
static bool gGlfwKeyDown[GLFW_KEY_LAST + 1];
static std::vector<Event> gGlfwKeyEvents;
static GLWindow* gActiveWindow = nullptr;

// AndroidDisplay::InitKeyMap @0x43EC28, with the GLFW key numbers in place of
// AGKeyCode. The stored values are the Windows virtual-key codes the game
// compares: left 0x25, up 0x26, right 0x27, down 0x28, Return 0x0D, Esc 0x1B,
// digits 0x30-0x39, letters 0x41-0x5A, Menu 0x12. Space is not in that table;
// Board::KeyDown still reads 0x20, so the port adds it.
//
// The numbers are written out. This file is inside namespace Sexy, and the
// Sexy::KeyCode enumerators do not match the global KEYCODE_* values the
// game compares.
static int KeyCodeFromGlfw(int theGlfwKey)
{
    if (theGlfwKey >= GLFW_KEY_A && theGlfwKey <= GLFW_KEY_Z)
        return 0x41 + (theGlfwKey - GLFW_KEY_A);
    if (theGlfwKey >= GLFW_KEY_0 && theGlfwKey <= GLFW_KEY_9)
        return 0x30 + (theGlfwKey - GLFW_KEY_0);

    switch (theGlfwKey)
    {
    case GLFW_KEY_ESCAPE:    return 0x1B;
    case GLFW_KEY_ENTER:     return 0x0D;
    case GLFW_KEY_KP_ENTER:  return 0x0D;
    case GLFW_KEY_UP:        return 0x26;
    case GLFW_KEY_DOWN:      return 0x28;
    case GLFW_KEY_LEFT:      return 0x25;
    case GLFW_KEY_RIGHT:     return 0x27;
    case GLFW_KEY_MENU:      return 0x12;
    case GLFW_KEY_SPACE:     return 0x20;
#ifdef _DEBUG
    case GLFW_KEY_BACKSPACE: return 0x08;
    case GLFW_KEY_TAB:       return 0x09;
    case GLFW_KEY_HOME:      return 0x24;
    case GLFW_KEY_END:       return 0x23;
    case GLFW_KEY_DELETE:    return 0x2E;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:   return 0x10;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL: return 0x11;
#endif
    default:                 return 0;
    }
}

GLWindow::GLWindow()
    : mWindow(nullptr), mWidth(0), mHeight(0), mWindowed(true),
      mActive(true), mClosed(false), mVSync(true),
      mLastTime(0.0), mStartTime(0.0), mFrameTime(0.0), mFrameTimer(nullptr)
{
}

GLWindow::~GLWindow()
{
    Destroy();
}

void GLWindow::KeyCallback(GLFWwindow*, int theKey, int, int theAction, int)
{
    if (theKey < 0 || theKey > GLFW_KEY_LAST)
        return;
    if (theAction == GLFW_PRESS || theAction == GLFW_REPEAT)
        gGlfwKeyDown[theKey] = true;
    else if (theAction == GLFW_RELEASE)
        gGlfwKeyDown[theKey] = false;
    else
        return;

    // AndroidInput::HandleKeyEvents posts every down, including a repeat
    // down. Release is the other kind. modifiers 1 is KeyDown, 2 is KeyUp.
    const int aCode = KeyCodeFromGlfw(theKey);
    if (aCode == 0)
        return;

    Event anEvent = {};
    anEvent.modifiers = theAction == GLFW_RELEASE ? 2 : 1;
    anEvent.lateModifiers = 0;
    anEvent.eventCodeMask = aCode;
    gGlfwKeyEvents.push_back(anEvent);
}

bool GLWindow::PopKeyEvent(Event& theEvent)
{
    if (gGlfwKeyEvents.empty())
        return false;
    theEvent = gGlfwKeyEvents.front();
    gGlfwKeyEvents.erase(gGlfwKeyEvents.begin());
    return true;
}

#ifdef _DEBUG
void GLWindow::CharacterCallback(GLFWwindow*, unsigned int theCodePoint)
{
    Event aCharacterEvent = {};
    aCharacterEvent.modifiers = Event::CHARACTER_INPUT;
    aCharacterEvent.eventCodeMask = (int)theCodePoint;
    gGlfwKeyEvents.push_back(aCharacterEvent);
}
#endif

bool GLWindow::IsGlfwKeyDown(int theGlfwKey)
{
    if (theGlfwKey < 0 || theGlfwKey > GLFW_KEY_LAST)
        return false;
    return gGlfwKeyDown[theGlfwKey];
}

void GLWindow::ErrorCallback(int theError, const char* theDescription)
{
    TOD_ERROR("GLFW error %d: %s", theError, theDescription ? theDescription : "?");
}

bool GLWindow::Create(const char* theTitle, int theWidth, int theHeight,
                      bool theWindowed, bool theVSync, bool theVisible)
{
    mWidth = theWidth;
    mHeight = theHeight;
    mWindowed = theWindowed;
    mVSync = theVSync;

    if (!gGLFWInitialized)
    {
        glfwSetErrorCallback(&GLWindow::ErrorCallback);
        if (!glfwInit())
        {
            TOD_ERROR("GLWindow: glfwInit failed");
            return false;
        }
        gGLFWInitialized = true;
    }

    // Desktop OpenGL 3.3 core. This is the closest widely-supported profile to
    // the GLES2 the Android build targeted: no fixed-function pipeline, shaders
    // mandatory, VAOs required. See GLHeaders.h for the GLES2 switch.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_VISIBLE, theVisible ? GLFW_TRUE : GLFW_FALSE);

    mWindow = glfwCreateWindow(theWidth, theHeight,
                               theTitle ? theTitle : "LawnTV",
                               nullptr, nullptr);
    if (!mWindow)
    {
        TOD_ERROR("GLWindow: glfwCreateWindow failed (GL 3.3 core unavailable?)");
        return false;
    }

    glfwSetKeyCallback(mWindow, &GLWindow::KeyCallback);
#ifdef _DEBUG
    glfwSetCharCallback(mWindow, &GLWindow::CharacterCallback);
#endif
    gActiveWindow = this;
    glfwMakeContextCurrent(mWindow);

    // glad loads the GL entry points for the *current* context, so this must
    // happen after MakeContextCurrent and before any gl* call.
    // glad 0.1.36 API: gladLoadGLLoader(GLADloadproc). It returns the GL
    // version as major*10+minor, or 0 on failure.
    int version = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    if (version == 0)
    {
        TOD_ERROR("GLWindow: gladLoadGLLoader failed");
        glfwDestroyWindow(mWindow);
        mWindow = nullptr;
        return false;
    }

    glfwSwapInterval(theVSync ? 1 : 0);

    TOD_TRACE("GLWindow: created %dx%d, GL %d.%d",
              theWidth, theHeight, version / 10, version % 10);
    TOD_TRACE("  renderer: %s", (const char*)glGetString(GL_RENDERER));
    TOD_TRACE("  vendor  : %s", (const char*)glGetString(GL_VENDOR));
    TOD_TRACE("  GLSL    : %s", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    ResetTimer();
    return true;
}

void GLWindow::Destroy()
{
    delete mFrameTimer;
    mFrameTimer = nullptr;
    if (gActiveWindow == this)
        gActiveWindow = nullptr;
    if (mWindow)
    {
        glfwDestroyWindow(mWindow);
        mWindow = nullptr;
    }
    gGlfwKeyEvents.clear();
    // GLFW itself is torn down in ShutdownGLFW(); leaving it initialised lets
    // a second window be created (the self-test opens one, closes it, and the
    // game may then open another).
}

void GLWindow::MakeCurrent()
{
    if (mWindow)
        glfwMakeContextCurrent(mWindow);
}

bool GLWindow::ProcessEvents()
{
    if (!mWindow) return false;

    glfwPollEvents();

    if (glfwWindowShouldClose(mWindow))
        mClosed = true;

    mActive = glfwGetWindowAttrib(mWindow, GLFW_FOCUSED) != 0;

    // Track drawable pixels. Presentation stretches the logical game FBO
    // across this area without changing widget coordinates.
    int w = 0, h = 0;
    glfwGetFramebufferSize(mWindow, &w, &h);
    if (w > 0 && h > 0 && (w != mWidth || h != mHeight))
    {
        mWidth = w;
        mHeight = h;
    }

    double now = glfwGetTime();
    mFrameTime = now - mLastTime;
    mLastTime = now;

    // Clamp so a stall (debugger, window drag) does not fast-forward the game.
    if (mFrameTime > 0.25) mFrameTime = 0.25;

    return !mClosed;
}

void GLWindow::SwapBuffers()
{
    if (mWindow)
        glfwSwapBuffers(mWindow);
}

void GLWindow::WaitForFrame(double theFrameStart, int theMaxFPS)
{
    if (theMaxFPS <= 0)
        return;

    double remaining = theFrameStart + 1.0 / theMaxFPS - GetTime();
    if (remaining <= 0.0)
        return;

    if (mFrameTimer == nullptr)
    {
        // Sleep can round the 16.67 ms frame budget to a whole scheduler tick.
        mFrameTimer = new GLFrameTimer();
    }
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -static_cast<LONGLONG>(remaining * 10000000.0);
    if (dueTime.QuadPart == 0)
        return;
    if (mFrameTimer->mHandle != nullptr && SetWaitableTimer(mFrameTimer->mHandle, &dueTime, 0, nullptr, nullptr, FALSE))
        WaitForSingleObject(mFrameTimer->mHandle, INFINITE);
    else
        Sleep(static_cast<DWORD>(remaining * 1000.0 + 1.0));
}

void GLWindow::SetVSync(bool theVSync)
{
    mVSync = theVSync;
    if (mWindow)
    {
        glfwMakeContextCurrent(mWindow);
        glfwSwapInterval(theVSync ? 1 : 0);
    }
}

void GLWindow::SetTitle(const char* theTitle)
{
    if (mWindow && theTitle)
        glfwSetWindowTitle(mWindow, theTitle);
}

void GLWindow::ResetTimer()
{
    mStartTime = glfwGetTime();
    mLastTime  = mStartTime;
}

double GLWindow::GetTime() const
{
    return glfwGetTime() - mStartTime;
}

bool GLWindow::IsKeyDown(int theKeyCode) const
{
    if (!mWindow) return false;

    // Map the framework's KeyCode onto GLFW's key numbering. Only the keys the
    // game actually queries are mapped; everything else reports up.
    int key = GLFW_KEY_UNKNOWN;
    if (theKeyCode >= KEYCODE_A && theKeyCode <= KEYCODE_Z)
        key = GLFW_KEY_A + (theKeyCode - KEYCODE_A);
    else if (theKeyCode >= KEYCODE_0 && theKeyCode <= KEYCODE_9)
        key = GLFW_KEY_0 + (theKeyCode - KEYCODE_0);
    else if (theKeyCode >= KEYCODE_F1 && theKeyCode <= KEYCODE_F12)
        key = GLFW_KEY_F1 + (theKeyCode - KEYCODE_F1);
    else
    {
        switch (theKeyCode)
        {
        case KEYCODE_ESC:      key = GLFW_KEY_ESCAPE; break;
        case KEYCODE_TAB:      key = GLFW_KEY_TAB; break;
        case KEYCODE_ENTER:    key = GLFW_KEY_ENTER; break;
        case KEYCODE_SHIFT:    key = GLFW_KEY_LEFT_SHIFT; break;
        case KEYCODE_CTRL:     key = GLFW_KEY_LEFT_CONTROL; break;
        case KEYCODE_ALT:      key = GLFW_KEY_LEFT_ALT; break;
        case KEYCODE_SPACE:    key = GLFW_KEY_SPACE; break;
        case KEYCODE_LEFT:     key = GLFW_KEY_LEFT; break;
        case KEYCODE_RIGHT:    key = GLFW_KEY_RIGHT; break;
        case KEYCODE_UP:       key = GLFW_KEY_UP; break;
        case KEYCODE_DOWN:     key = GLFW_KEY_DOWN; break;
        case KEYCODE_HOME:     key = GLFW_KEY_HOME; break;
        case KEYCODE_END:      key = GLFW_KEY_END; break;
        case KEYCODE_DELETE:   key = GLFW_KEY_DELETE; break;
        case KEYCODE_BACK:     key = GLFW_KEY_BACKSPACE; break;
        case KEYCODE_PAGEUP:   key = GLFW_KEY_PAGE_UP; break;
        case KEYCODE_PAGEDOWN: key = GLFW_KEY_PAGE_DOWN; break;
        case KEYCODE_INSERT:   key = GLFW_KEY_INSERT; break;
        default: return false;
        }
    }

    return key != GLFW_KEY_UNKNOWN &&
           glfwGetKey(mWindow, key) == GLFW_PRESS;
}

bool GLWindow::IsMouseButtonDown(int theButton) const
{
    if (!mWindow) return false;

    int btn = GLFW_MOUSE_BUTTON_LEFT;
    switch (theButton)
    {
    case MOUSE_LEFT:   btn = GLFW_MOUSE_BUTTON_LEFT;   break;
    case MOUSE_RIGHT:  btn = GLFW_MOUSE_BUTTON_RIGHT;  break;
    case MOUSE_MIDDLE: btn = GLFW_MOUSE_BUTTON_MIDDLE; break;
    default: return false;
    }

    return glfwGetMouseButton(mWindow, btn) == GLFW_PRESS;
}

void GLWindow::GetCursorPos(double& theX, double& theY) const
{
    theX = theY = 0.0;
    if (mWindow)
        glfwGetCursorPos(mWindow, &theX, &theY);
}

GLWindow* ActiveWindow()
{
    return gActiveWindow;
}

void ShutdownGLFW()
{
    if (gGLFWInitialized)
    {
        glfwTerminate();
        gGLFWInitialized = false;
    }
}

} // namespace Sexy

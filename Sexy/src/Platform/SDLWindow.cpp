#include "SDLWindow.h"
#include "SexyTypes.h"
#include "AndroidShims.h"
#include <algorithm>

namespace Sexy
{
namespace
{
SDLWindow *sActiveWindow = nullptr;

// AndroidDisplay::InitKeyMap @0x43EC28 uses the virtual-key values read by
// game code. SDL scancodes are converted once at the platform boundary.
int GameKeyFromScan(SDL_Scancode theScan)
{
    if (theScan >= SDL_SCANCODE_A && theScan <= SDL_SCANCODE_Z)
        return 0x41 + theScan - SDL_SCANCODE_A;
    if (theScan >= SDL_SCANCODE_1 && theScan <= SDL_SCANCODE_9)
        return 0x31 + theScan - SDL_SCANCODE_1;
    if (theScan >= SDL_SCANCODE_F1 && theScan <= SDL_SCANCODE_F12)
        return 0x70 + theScan - SDL_SCANCODE_F1;
    switch (theScan)
    {
        case SDL_SCANCODE_0:
            return 0x30;
        case SDL_SCANCODE_ESCAPE:
            return 0x1b;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            return 0x0d;
        case SDL_SCANCODE_SPACE:
            return 0x20;
        case SDL_SCANCODE_UP:
            return 0x26;
        case SDL_SCANCODE_DOWN:
            return 0x28;
        case SDL_SCANCODE_LEFT:
            return 0x25;
        case SDL_SCANCODE_RIGHT:
            return 0x27;
        case SDL_SCANCODE_TAB:
            return 0x09;
        case SDL_SCANCODE_BACKSPACE:
            return 0x08;
        case SDL_SCANCODE_HOME:
            return 0x24;
        case SDL_SCANCODE_END:
            return 0x23;
        case SDL_SCANCODE_DELETE:
            return 0x2e;
        case SDL_SCANCODE_INSERT:
            return 0x2d;
        case SDL_SCANCODE_PAGEUP:
            return 0x21;
        case SDL_SCANCODE_PAGEDOWN:
            return 0x22;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            return 0x10;
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return 0x11;
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_RALT:
        case SDL_SCANCODE_MENU:
            return 0x12;
        default:
            return 0;
    }
}
double ClockSeconds()
{
    return SDL_GetTicksNS() / 1000000000.0;
}
}

SDLWindow::~SDLWindow()
{
    Destroy();
}
bool SDLWindow::Create(
    const char *theTitle, int theWidth, int theHeight, bool theWindowed, bool theVSync, bool theVisible)
{
    Destroy();
    if (theWidth <= 0 || theHeight <= 0 || !SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return false;
    mVideoInitialized = true;
    // Keep the requested game canvas independent of the native surface. The
    // presentation pass scales this canvas when the fullscreen/window grows.
    mRenderWidth = theWidth;
    mRenderHeight = theHeight;
#if defined(SDL_PLATFORM_ANDROID)
    theWindowed = false;
    const SDL_DisplayMode *displayMode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    if (displayMode != nullptr)
    {
        theWidth = displayMode->w;
        theHeight = displayMode->h;
    }
#endif
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    if (!theVisible)
        flags |= SDL_WINDOW_HIDDEN;
    if (!theWindowed)
        flags |= SDL_WINDOW_FULLSCREEN;
    // bgfx owns its graphics context/device. SDL provides the native window.
    mWindow = SDL_CreateWindow(theTitle ? theTitle : "Lawn", theWidth, theHeight, flags);
    if (mWindow == nullptr)
    {
        TOD_ERROR("SDLWindow: %s", SDL_GetError());
        Destroy();
        return false;
    }
    mWidth = theWidth;
    mHeight = theHeight;
#if !defined(SDL_PLATFORM_ANDROID)
    SDL_GetWindowSizeInPixels(mWindow, &mWidth, &mHeight);
    mRenderWidth = mWidth;
    mRenderHeight = mHeight;
#endif
    // Android hides system bars asynchronously. Keep the display resolution
    // for the initial presentation surface instead of the previous inset size.
    mWindowed = theWindowed;
    mVSync = theVSync;
    mClosed = false;
    sActiveWindow = this;
#if !defined(SDL_PLATFORM_ANDROID)
    SDL_StartTextInput(mWindow);
#endif
    ResetTimer();
    return true;
}
void SDLWindow::Destroy()
{
    if (sActiveWindow == this)
        sActiveWindow = nullptr;
    if (mWindow)
        SDL_DestroyWindow(mWindow);
    mWindow = nullptr;
    mInputEvents.clear();
    mMouseButtons = 0;
    mMousePositionKnown = false;
    if (mVideoInitialized)
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    mVideoInitialized = false;
}
void SDLWindow::ProcessEvent(const SDL_Event &theEvent)
{
    if (theEvent.type == SDL_EVENT_QUIT || theEvent.type == SDL_EVENT_TERMINATING)
    {
        mClosed = true;
        return;
    }
    if (!mWindow)
        return;

    SDL_WindowID windowID = SDL_GetWindowID(mWindow);
    if (theEvent.type >= SDL_EVENT_WINDOW_FIRST && theEvent.type <= SDL_EVENT_WINDOW_LAST &&
        theEvent.window.windowID == windowID)
    {
        if (theEvent.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            mClosed = true;
        if (theEvent.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
            mActive = true;
        if (theEvent.type == SDL_EVENT_WINDOW_MOUSE_LEAVE)
        {
            mMousePositionKnown = false;
            Event exitEvent = {};
            exitEvent.modifiers = Event::MOUSE_EXIT;
            mInputEvents.push_back(exitEvent);
        }
        if (theEvent.type == SDL_EVENT_WINDOW_FOCUS_LOST)
        {
            mActive = false;
            mMousePositionKnown = false;
            Event cancelEvent = {};
            cancelEvent.modifiers = Event::MOUSE_CANCEL;
            mInputEvents.push_back(cancelEvent);
            mMouseButtons = 0;
            SDL_CaptureMouse(false);
        }
    }
    if ((theEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN || theEvent.type == SDL_EVENT_MOUSE_BUTTON_UP) &&
        theEvent.button.windowID == windowID &&
        (theEvent.button.button == SDL_BUTTON_LEFT || theEvent.button.button == SDL_BUTTON_RIGHT ||
            theEvent.button.button == SDL_BUTTON_MIDDLE))
    {
        const int clickCount = theEvent.button.button == SDL_BUTTON_RIGHT ? -1
            : theEvent.button.button == SDL_BUTTON_MIDDLE                 ? 3
                                                          : std::clamp(int(theEvent.button.clicks), 1, 2);
        if (theEvent.button.down)
            mMouseButtons |= SDL_BUTTON_MASK(theEvent.button.button);
        else
            mMouseButtons &= ~SDL_BUTTON_MASK(theEvent.button.button);
        SDL_CaptureMouse(mMouseButtons != 0);
        QueueMouseEvent(theEvent.button.down ? Event::MOUSE_DOWN : Event::MOUSE_UP,
            theEvent.button.x,
            theEvent.button.y,
            clickCount);
    }
    if (theEvent.type == SDL_EVENT_MOUSE_WHEEL && theEvent.wheel.windowID == windowID)
    {
        Event wheel = {};
        wheel.modifiers = Event::MOUSE_WHEEL;
        const float direction = theEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
        wheel.eventCodeMask = int(theEvent.wheel.x * direction * 100.0f);
        wheel.matchEvent = int(theEvent.wheel.y * direction * 100.0f);
        mInputEvents.push_back(wheel);
    }
    if ((theEvent.type == SDL_EVENT_KEY_DOWN || theEvent.type == SDL_EVENT_KEY_UP) && theEvent.key.windowID == windowID)
    {
        int code = GameKeyFromScan(theEvent.key.scancode);
        if (code)
        {
            Event key = {};
            key.modifiers = theEvent.key.down ? 1 : 2;
            key.eventCodeMask = code;
            mInputEvents.push_back(key);
        }
    }
    if (theEvent.type == SDL_EVENT_TEXT_INPUT && theEvent.text.windowID == windowID)
    {
        const char *cursor = theEvent.text.text;
        while (*cursor)
        {
            unsigned char lead = static_cast<unsigned char>(*cursor++);
            unsigned int codepoint = lead;
            int continuation = 0;
            if (lead >= 0xf0)
            {
                codepoint = lead & 7;
                continuation = 3;
            }
            else if (lead >= 0xe0)
            {
                codepoint = lead & 15;
                continuation = 2;
            }
            else if (lead >= 0xc0)
            {
                codepoint = lead & 31;
                continuation = 1;
            }
            while (continuation-- > 0 && *cursor)
                codepoint = (codepoint << 6) | (static_cast<unsigned char>(*cursor++) & 63);
            Event character = {};
            character.modifiers = Event::CHARACTER_INPUT;
            character.eventCodeMask = codepoint;
            mInputEvents.push_back(character);
        }
    }
}
void SDLWindow::UpdateFrame()
{
    if (!mWindow)
        return;
    // Sample SDL's current pointer state once per frame. Movement events can
    // arrive in bursts; their old positions must not drive the planting cursor.
    if (mActive && SDL_GetMouseFocus() == mWindow)
    {
        float mouseX = 0.0f;
        float mouseY = 0.0f;
        SDL_GetMouseState(&mouseX, &mouseY);
        if (!mMousePositionKnown || mouseX != mLastMouseX || mouseY != mLastMouseY)
            QueueMouseEvent(Event::MOUSE_MOVE, mouseX, mouseY);
        mLastMouseX = mouseX;
        mLastMouseY = mouseY;
        mMousePositionKnown = true;
    }
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(mWindow, &width, &height);
    if (width > 0 && height > 0)
    {
        mWidth = width;
        mHeight = height;
    }
    double now = ClockSeconds();
    mFrameTime = std::clamp(now - mLastTime, 0.0, 0.25);
    mLastTime = now;
}
bool SDLWindow::ProcessEvents()
{
    if (!mWindow)
        return false;
    // Legacy WaitForResult blocks the outer SDL callback while a dialog is
    // open. Keep its local pump until those waits become asynchronous.
    SDL_Event event;
    while (SDL_PollEvent(&event))
        ProcessEvent(event);
    UpdateFrame();
    return !mClosed;
}
void SDLWindow::QueueMouseEvent(int theKind, float theX, float theY, int theClickCount)
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(mWindow, &width, &height);
    if (width <= 0 || height <= 0)
        return;
    Event pointer = {};
    pointer.modifiers = theKind == Event::MOUSE_UP && (theX < 0 || theX >= width || theY < 0 || theY >= height)
        ? Event::MOUSE_CANCEL
        : theKind;
    pointer.modifierMask = 1 | 32; // Absolute coordinates scaled by InputManager to the render surface.
    pointer.eventCodeMask = int(theX);
    pointer.matchEvent = int(theY);
    pointer.standard = theClickCount;
    pointer.mScaleDivisorX = width;
    pointer.mScaleDivisorY = height;
    mInputEvents.push_back(pointer);
}
bool SDLWindow::PopInputEvent(Event &theEvent)
{
    if (mInputEvents.empty())
        return false;
    theEvent = mInputEvents.front();
    mInputEvents.pop_front();
    return true;
}
bool SDLWindow::IsKeyDown(int theKeyCode) const
{
    int count = 0;
    const bool *keys = SDL_GetKeyboardState(&count);
    for (int scan = 0; scan < count; ++scan)
        if (keys[scan] && GameKeyFromScan(static_cast<SDL_Scancode>(scan)) == theKeyCode)
            return true;
    return false;
}
bool SDLWindow::IsMouseButtonDown(int theButton) const
{
    Uint32 button = theButton == MOUSE_LEFT ? SDL_BUTTON_LEFT
        : theButton == MOUSE_RIGHT          ? SDL_BUTTON_RIGHT
                                            : SDL_BUTTON_MIDDLE;
    return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(button)) != 0;
}
void SDLWindow::GetCursorPos(double &theX, double &theY) const
{
    float x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    int width = 0, height = 0;
    SDL_GetWindowSize(mWindow, &width, &height);
    theX = width > 0 ? x * mWidth / width : x;
    theY = height > 0 ? y * mHeight / height : y;
}
void SDLWindow::WaitForFrame(double theFrameStart, int theMaxFPS)
{
    if (theMaxFPS <= 0)
        return;
    double remaining = theFrameStart + 1.0 / theMaxFPS - GetTime();
    if (remaining > 0.0)
        SDL_DelayPrecise(static_cast<Uint64>(remaining * 1000000000.0));
}
void SDLWindow::SetVSync(bool theVSync)
{
    mVSync = theVSync;
}
void SDLWindow::SetTitle(const char *theTitle)
{
    if (mWindow && theTitle)
        SDL_SetWindowTitle(mWindow, theTitle);
}
void SDLWindow::ResetTimer()
{
    mStartTime = mLastTime = ClockSeconds();
}
double SDLWindow::GetTime() const
{
    return ClockSeconds() - mStartTime;
}
SDLWindow *ActiveWindow()
{
    return sActiveWindow;
}
void ShutdownSDL()
{
    SDL_Quit();
}
}

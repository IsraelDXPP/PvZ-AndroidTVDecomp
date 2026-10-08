#include "SDLWindow.h"
#include "SexyTypes.h"
#include "AndroidShims.h"
#include <algorithm>

namespace Sexy {
namespace {
SDLWindow* sActiveWindow = nullptr;

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
    case SDL_SCANCODE_0: return 0x30;
    case SDL_SCANCODE_ESCAPE: return 0x1b;
    case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: return 0x0d;
    case SDL_SCANCODE_SPACE: return 0x20;
    case SDL_SCANCODE_UP: return 0x26;
    case SDL_SCANCODE_DOWN: return 0x28;
    case SDL_SCANCODE_LEFT: return 0x25;
    case SDL_SCANCODE_RIGHT: return 0x27;
    case SDL_SCANCODE_TAB: return 0x09;
    case SDL_SCANCODE_BACKSPACE: return 0x08;
    case SDL_SCANCODE_HOME: return 0x24;
    case SDL_SCANCODE_END: return 0x23;
    case SDL_SCANCODE_DELETE: return 0x2e;
    case SDL_SCANCODE_INSERT: return 0x2d;
    case SDL_SCANCODE_PAGEUP: return 0x21;
    case SDL_SCANCODE_PAGEDOWN: return 0x22;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return 0x10;
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return 0x11;
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: case SDL_SCANCODE_MENU: return 0x12;
    default: return 0;
    }
}
double ClockSeconds() { return SDL_GetTicksNS() / 1000000000.0; }
}

SDLWindow::~SDLWindow() { Destroy(); }
bool SDLWindow::Create(const char* theTitle, int theWidth, int theHeight,
                       bool theWindowed, bool theVSync, bool theVisible)
{
    Destroy();
    if (theWidth <= 0 || theHeight <= 0 || !SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return false;
    mVideoInitialized = true;
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    if (!theVisible) flags |= SDL_WINDOW_HIDDEN;
    if (!theWindowed) flags |= SDL_WINDOW_FULLSCREEN;
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
    SDL_GetWindowSizeInPixels(mWindow, &mWidth, &mHeight);
    mWindowed = theWindowed;
    mVSync = theVSync;
    mClosed = false;
    sActiveWindow = this;
#ifdef _DEBUG
    SDL_StartTextInput(mWindow);
#endif
    ResetTimer();
    return true;
}
void SDLWindow::Destroy()
{
    if (sActiveWindow == this) sActiveWindow = nullptr;
    if (mWindow) SDL_DestroyWindow(mWindow);
    mWindow = nullptr;
    mInputEvents.clear();
    mMouseButtons = 0;
    if (mVideoInitialized) SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    mVideoInitialized = false;
}
bool SDLWindow::ProcessEvents()
{
    if (!mWindow) return false;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT) mClosed = true;
        if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST
            && event.window.windowID == SDL_GetWindowID(mWindow))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) mClosed = true;
            if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) mActive = true;
            if (event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE)
            {
                Event exitEvent = {};
                exitEvent.modifiers = Event::MOUSE_EXIT;
                mInputEvents.push_back(exitEvent);
            }
            if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
            {
                mActive = false;
                Event cancelEvent = {};
                cancelEvent.modifiers = Event::MOUSE_CANCEL;
                mInputEvents.push_back(cancelEvent);
                mMouseButtons = 0;
                SDL_CaptureMouse(false);
            }
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION && event.motion.windowID == SDL_GetWindowID(mWindow))
            QueueMouseEvent(Event::MOUSE_MOVE, event.motion.x, event.motion.y);
        if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            && event.button.windowID == SDL_GetWindowID(mWindow)
            && (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT || event.button.button == SDL_BUTTON_MIDDLE))
        {
            const int clickCount = event.button.button == SDL_BUTTON_RIGHT ? -1
                : event.button.button == SDL_BUTTON_MIDDLE ? 3 : std::clamp(int(event.button.clicks), 1, 2);
            if (event.button.down)
                mMouseButtons |= SDL_BUTTON_MASK(event.button.button);
            else
                mMouseButtons &= ~SDL_BUTTON_MASK(event.button.button);
            SDL_CaptureMouse(mMouseButtons != 0);
            QueueMouseEvent(event.button.down ? Event::MOUSE_DOWN : Event::MOUSE_UP,
                            event.button.x, event.button.y, clickCount);
        }
        if (event.type == SDL_EVENT_MOUSE_WHEEL && event.wheel.windowID == SDL_GetWindowID(mWindow))
        {
            Event wheel = {};
            wheel.modifiers = Event::MOUSE_WHEEL;
            const float direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
            wheel.eventCodeMask = int(event.wheel.x * direction * 100.0f);
            wheel.matchEvent = int(event.wheel.y * direction * 100.0f);
            mInputEvents.push_back(wheel);
        }
        if ((event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
            && event.key.windowID == SDL_GetWindowID(mWindow))
        {
            int code = GameKeyFromScan(event.key.scancode);
            if (code)
            {
                Event key = {};
                key.modifiers = event.key.down ? 1 : 2;
                key.eventCodeMask = code;
                mInputEvents.push_back(key);
            }
        }
#ifdef _DEBUG
        if (event.type == SDL_EVENT_TEXT_INPUT && event.text.windowID == SDL_GetWindowID(mWindow))
        {
            const char* cursor = event.text.text;
            while (*cursor)
            {
                unsigned char lead = static_cast<unsigned char>(*cursor++);
                unsigned int codepoint = lead;
                int continuation = 0;
                if (lead >= 0xf0) { codepoint = lead & 7; continuation = 3; }
                else if (lead >= 0xe0) { codepoint = lead & 15; continuation = 2; }
                else if (lead >= 0xc0) { codepoint = lead & 31; continuation = 1; }
                while (continuation-- > 0 && *cursor)
                    codepoint = (codepoint << 6) | (static_cast<unsigned char>(*cursor++) & 63);
                Event character = {};
                character.modifiers = Event::CHARACTER_INPUT;
                character.eventCodeMask = codepoint;
                mInputEvents.push_back(character);
            }
        }
#endif
    }
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(mWindow, &width, &height);
    if (width > 0 && height > 0) { mWidth = width; mHeight = height; }
    double now = ClockSeconds();
    mFrameTime = std::clamp(now - mLastTime, 0.0, 0.25);
    mLastTime = now;
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
    pointer.modifiers = theKind == Event::MOUSE_UP
        && (theX < 0 || theX >= width || theY < 0 || theY >= height) ? Event::MOUSE_CANCEL : theKind;
    pointer.modifierMask = 1 | 32; // Absolute coordinates scaled by InputManager to the render surface.
    pointer.eventCodeMask = int(theX);
    pointer.matchEvent = int(theY);
    pointer.standard = theClickCount;
    pointer.mScaleDivisorX = width;
    pointer.mScaleDivisorY = height;
    mInputEvents.push_back(pointer);
}
bool SDLWindow::PopInputEvent(Event& theEvent)
{
    if (mInputEvents.empty()) return false;
    theEvent = mInputEvents.front();
    mInputEvents.pop_front();
    return true;
}
bool SDLWindow::IsKeyDown(int theKeyCode) const
{
    int count = 0;
    const bool* keys = SDL_GetKeyboardState(&count);
    for (int scan = 0; scan < count; ++scan)
        if (keys[scan] && GameKeyFromScan(static_cast<SDL_Scancode>(scan)) == theKeyCode) return true;
    return false;
}
bool SDLWindow::IsMouseButtonDown(int theButton) const
{
    Uint32 button = theButton == MOUSE_LEFT ? SDL_BUTTON_LEFT
        : theButton == MOUSE_RIGHT ? SDL_BUTTON_RIGHT : SDL_BUTTON_MIDDLE;
    return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(button)) != 0;
}
void SDLWindow::GetCursorPos(double& theX, double& theY) const
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
    if (theMaxFPS <= 0) return;
    double remaining = theFrameStart + 1.0 / theMaxFPS - GetTime();
    if (remaining > 0.0) SDL_DelayPrecise(static_cast<Uint64>(remaining * 1000000000.0));
}
void SDLWindow::SetVSync(bool theVSync) { mVSync = theVSync; }
void SDLWindow::SetTitle(const char* theTitle) { if (mWindow && theTitle) SDL_SetWindowTitle(mWindow, theTitle); }
void SDLWindow::ResetTimer() { mStartTime = mLastTime = ClockSeconds(); }
double SDLWindow::GetTime() const { return ClockSeconds() - mStartTime; }
SDLWindow* ActiveWindow() { return sActiveWindow; }
void ShutdownSDL() { SDL_Quit(); }
}

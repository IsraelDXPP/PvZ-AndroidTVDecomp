#pragma once

#include <SDL3/SDL.h>
#include <atomic>
#include <deque>
#include "ReconTypes.h"

namespace Sexy {

class SDLWindow
{
public:
    SDL_Window* mWindow = nullptr;
    int mWidth = 0;
    int mHeight = 0;
    bool mWindowed = true;
    bool mActive = true;
    // SDL may dispatch mobile termination events from another thread.
    std::atomic<bool> mClosed{false};
    bool mVSync = true;
    double mFrameTime = 0.0;

    enum MouseButton { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE };
    ~SDLWindow();
    bool Create(const char* theTitle, int theWidth, int theHeight,
                bool theWindowed = true, bool theVSync = true, bool theVisible = true);
    void Destroy();
    void ProcessEvent(const SDL_Event& theEvent);
    void UpdateFrame();
    // Compatibility pump for synchronous modal waits; the main loop uses callbacks.
    bool ProcessEvents();
    void WaitForFrame(double theFrameStart, int theMaxFPS);
    void SetVSync(bool theVSync);
    void SetTitle(const char* theTitle);
    double GetTime() const;
    double GetFrameTime() const { return mFrameTime; }
    void ResetTimer();
    bool IsKeyDown(int theKeyCode) const;
    bool PopInputEvent(Event& theEvent);
    bool IsMouseButtonDown(int theButton) const;
    void GetCursorPos(double& theX, double& theY) const;
private:
    bool mVideoInitialized = false;
    double mStartTime = 0.0;
    double mLastTime = 0.0;
    std::deque<Event> mInputEvents;
    unsigned int mMouseButtons = 0;
    void QueueMouseEvent(int theKind, float theX, float theY, int theClickCount = 0);
};

SDLWindow* ActiveWindow();
void ShutdownSDL();
}

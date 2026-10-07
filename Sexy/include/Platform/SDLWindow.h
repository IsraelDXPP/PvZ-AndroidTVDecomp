#pragma once

#include <SDL3/SDL.h>
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
    bool mClosed = false;
    bool mVSync = true;
    double mFrameTime = 0.0;

    enum MouseButton { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE };
    ~SDLWindow();
    bool Create(const char* theTitle, int theWidth, int theHeight,
                bool theWindowed = true, bool theVSync = true, bool theVisible = true);
    void Destroy();
    bool ProcessEvents();
    void WaitForFrame(double theFrameStart, int theMaxFPS);
    void SetVSync(bool theVSync);
    void SetTitle(const char* theTitle);
    double GetTime() const;
    double GetFrameTime() const { return mFrameTime; }
    void ResetTimer();
    bool IsKeyDown(int theKeyCode) const;
    bool PopKeyEvent(Event& theEvent);
    bool IsMouseButtonDown(int theButton) const;
    void GetCursorPos(double& theX, double& theY) const;
private:
    bool mVideoInitialized = false;
    double mStartTime = 0.0;
    double mLastTime = 0.0;
    std::deque<Event> mKeyEvents;
};

SDLWindow* ActiveWindow();
void ShutdownSDL();
}

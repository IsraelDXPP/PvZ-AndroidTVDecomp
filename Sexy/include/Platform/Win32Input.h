// Win32Input.h -- keyboard/mouse input for the Windows build.
//
// Replaces the Android input stack. In the binary this is Sexy::AndroidInput /
// Sexy::AndroidInputDriver plus the AndroidDisplay::InitKeyMap table; the TV
// build additionally routes everything through GamepadControls, which is why
// its mouse path was stripped (Board::UpdateCursor is a 2-byte stub in the
// binary, and cursor position comes only from mGamepadControls[i]->mCursorX/Y).
//
// On Windows we restore the direct mouse path: the window reports real cursor
// coordinates, which the app feeds into Board's cursor fields.

#ifndef __WIN32INPUT_H__
#define __WIN32INPUT_H__

#include "CoreTypes.h"
#include "ReconTypes.h"
#include <string>
#include <windows.h>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

class Win32Input
{
public:
    bool    mKeyDown[NUM_KEYCODES];
    bool    mMouseDown[3];
    int     mMouseX;
    int     mMouseY;
    int     mMouseWheel;
    bool    mHasFocus;

    // Text typed this frame (for edit widgets).
    std::string mTypedText;

public:
    Win32Input();

    void    Reset();

    // Translate a Win32 virtual key to the framework's KeyCode.
    static  int  TranslateKey(WPARAM vk, LPARAM lParam);

    // Called from the window procedure / message pump.
    void    OnKeyDown(int keyCode);
    void    OnKeyUp(int keyCode);
    void    OnChar(char ch);
    void    OnMouseMove(int x, int y);
    void    OnMouseDown(int button, int x, int y);
    void    OnMouseUp(int button, int x, int y);
    void    OnMouseWheel(int delta);

    bool    IsKeyDown(int keyCode) const
    {
        return keyCode > 0 && keyCode < NUM_KEYCODES && mKeyDown[keyCode];
    }
};

} // namespace Sexy

#endif // __WIN32INPUT_H__

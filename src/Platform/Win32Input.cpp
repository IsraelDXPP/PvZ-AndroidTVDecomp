// Win32Input.cpp -- Win32 virtual-key translation.

#include "Win32Input.h"
#include "ReconTypes.h"
#include <windows.h>
#include <cstring>

namespace Sexy
{

Win32Input::Win32Input()
{
    Reset();
}

void Win32Input::Reset()
{
    memset(mKeyDown, 0, sizeof(mKeyDown));
    memset(mMouseDown, 0, sizeof(mMouseDown));
    mMouseX = 0;
    mMouseY = 0;
    mMouseWheel = 0;
    mHasFocus = true;
    mTypedText.clear();
}

// Maps Win32 VK_* codes onto the framework's KeyCode enum. The original
// Android build performed the equivalent mapping from Android key codes in
// AndroidDisplay::InitKeyMap; the enumerator set is identical, only the source
// codes differ.
int Win32Input::TranslateKey(WPARAM vk, LPARAM lParam)
{
    // Letters and digits map 1:1 onto the enum's ordering.
    if (vk >= 'A' && vk <= 'Z')
        return KEYCODE_A + (int)(vk - 'A');
    if (vk >= '0' && vk <= '9')
        return KEYCODE_0 + (int)(vk - '0');
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
        return KEYCODE_NUMPAD0 + (int)(vk - VK_NUMPAD0);
    if (vk >= VK_F1 && vk <= VK_F12)
        return KEYCODE_F1 + (int)(vk - VK_F1);

    switch (vk)
    {
    case VK_ESCAPE:     return KEYCODE_ESC;
    case VK_TAB:        return KEYCODE_TAB;
    case VK_RETURN:     return KEYCODE_ENTER;
    case VK_SHIFT:      return KEYCODE_SHIFT;
    case VK_CONTROL:    return KEYCODE_CTRL;
    case VK_MENU:       return KEYCODE_ALT;
    case VK_SPACE:      return KEYCODE_SPACE;
    case VK_PRIOR:      return KEYCODE_PAGEUP;
    case VK_NEXT:       return KEYCODE_PAGEDOWN;
    case VK_END:        return KEYCODE_END;
    case VK_HOME:       return KEYCODE_HOME;
    case VK_LEFT:       return KEYCODE_LEFT;
    case VK_UP:         return KEYCODE_UP;
    case VK_RIGHT:      return KEYCODE_RIGHT;
    case VK_DOWN:       return KEYCODE_DOWN;
    case VK_INSERT:     return KEYCODE_INSERT;
    case VK_DELETE:     return KEYCODE_DELETE;
    case VK_BACK:       return KEYCODE_BACK;
    case VK_MULTIPLY:   return KEYCODE_MULTIPLY;
    case VK_ADD:        return KEYCODE_ADD;
    case VK_SUBTRACT:   return KEYCODE_SUBTRACT;
    case VK_DECIMAL:    return KEYCODE_DECIMAL;
    case VK_DIVIDE:     return KEYCODE_DIVIDE;

    // Gamepad-style buttons: the TV build's primary device. The VK_GAMEPAD_*
    // constants only exist in newer Windows SDKs, so guard them and fall back
    // to the classic XInput-style keyboard aliases when unavailable.
#ifdef VK_GAMEPAD_A
    case VK_GAMEPAD_A:              return KEYCODE_BUTTON_A;
    case VK_GAMEPAD_B:              return KEYCODE_BUTTON_B;
    case VK_GAMEPAD_X:              return KEYCODE_BUTTON_X;
    case VK_GAMEPAD_Y:              return KEYCODE_BUTTON_Y;
    case VK_GAMEPAD_LEFT_SHOULDER:  return KEYCODE_BUTTON_L1;
    case VK_GAMEPAD_RIGHT_SHOULDER: return KEYCODE_BUTTON_R1;
    case VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON:  return KEYCODE_BUTTON_THUMBL;
    case VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON: return KEYCODE_BUTTON_THUMBR;
    case VK_GAMEPAD_DPAD_UP:        return KEYCODE_DPAD_UP;
    case VK_GAMEPAD_DPAD_DOWN:      return KEYCODE_DPAD_DOWN;
    case VK_GAMEPAD_DPAD_LEFT:      return KEYCODE_DPAD_LEFT;
    case VK_GAMEPAD_DPAD_RIGHT:     return KEYCODE_DPAD_RIGHT;
#ifdef VK_GAMEPAD_START
    case VK_GAMEPAD_START:          return KEYCODE_BUTTON_START;
#endif
#ifdef VK_GAMEPAD_VIEW
    case VK_GAMEPAD_VIEW:           return KEYCODE_BUTTON_SELECT;
#endif
#ifdef VK_GAMEPAD_MENU
    case VK_GAMEPAD_MENU:           return KEYCODE_BUTTON_MODE;
#endif
#endif // VK_GAMEPAD_A

    default:            return KEYCODE_UNKNOWN;
    }
}

void Win32Input::OnKeyDown(int keyCode)
{
    if (keyCode > 0 && keyCode < NUM_KEYCODES)
        mKeyDown[keyCode] = true;
}

void Win32Input::OnKeyUp(int keyCode)
{
    if (keyCode > 0 && keyCode < NUM_KEYCODES)
        mKeyDown[keyCode] = false;
}

void Win32Input::OnChar(char ch)
{
    if ((unsigned char)ch >= 32)
        mTypedText.push_back(ch);
}

void Win32Input::OnMouseMove(int x, int y)
{
    mMouseX = x;
    mMouseY = y;
}

void Win32Input::OnMouseDown(int button, int x, int y)
{
    if (button >= 0 && button < 3) mMouseDown[button] = true;
    mMouseX = x;
    mMouseY = y;
}

void Win32Input::OnMouseUp(int button, int x, int y)
{
    if (button >= 0 && button < 3) mMouseDown[button] = false;
    mMouseX = x;
    mMouseY = y;
}

void Win32Input::OnMouseWheel(int delta)
{
    mMouseWheel += delta;
}

} // namespace Sexy

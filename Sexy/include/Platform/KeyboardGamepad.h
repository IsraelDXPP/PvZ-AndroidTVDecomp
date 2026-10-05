// KeyboardGamepad -- a keyboard that speaks the gamepad driver interface.
//
// No physical pad is connected, so this driver stands in for one. It reads
// LAWN_BUTTON_MAP and turns those keys into the same KEYCODE_GAMEPAD_* events
// XInput already posts. GamepadApp then sees one connected pad and follows
// the pad path. The physical pad driver stays silent when nothing is plugged in.

#pragma once

#include "IInputDriver.h"
#include "InputDriver.h"

namespace Sexy
{

class KeyboardGamepadDriver : public InputDriver
{
public:
    KeyboardGamepadDriver();
    virtual ~KeyboardGamepadDriver();

    virtual IInputDriver *Init();
    virtual int OnStart(Sexy::SexyAppBase *theApp, Sexy::InputManager *theManager);
    virtual int OnStop();

    IInputDriver *mInterface;
};

} // namespace Sexy

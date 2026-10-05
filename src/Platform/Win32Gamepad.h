// Win32Gamepad -- XInput and DirectInput behind Sexy::IInputDriver.
//
// XInput covers the four Xbox slots. DirectInput covers every other attached
// game controller, skipping the ones XInput already owns so a pad is not
// reported twice. Each connected pad is one device. Buttons become key events
// the game already understands; sticks are read back through GetAxisInfo.

#pragma once

#include "IInputDriver.h"
#include "InputDriver.h"

#include <windows.h>
#include <vector>

namespace Sexy
{

class Win32GamepadDriver : public InputDriver
{
public:
    Win32GamepadDriver();
    virtual ~Win32GamepadDriver();

    virtual IInputDriver *Init();
    virtual int OnStart(Sexy::SexyAppBase *theApp, Sexy::InputManager *theManager);
    virtual int OnStop();

    IInputDriver *mInterface;
};

IInputDriver *CreateWin32Gamepad(InputDriver *theOwner);

} // namespace Sexy

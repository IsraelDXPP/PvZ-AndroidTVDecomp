// Win32Gamepad -- XInput behind Sexy::IInputDriver.
//
// The four Xbox slots are the devices. A connected pad becomes key events the
// game already understands; sticks are read back through GetAxisInfo.

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

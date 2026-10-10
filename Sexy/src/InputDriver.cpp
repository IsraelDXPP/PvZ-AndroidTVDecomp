// InputDriver and IInputDriver -- the two bases the Windows gamepad driver
// derives from. Both destructors only rewrite the vptr; OnStart and OnStop
// are the hooks the manager calls around the driver's lifetime.

#include "InputDriver.h"
#include "IInputDriver.h"

Sexy::InputDriver::InputDriver(Sexy::String theName, int thePriority) :
    Driver(theName, thePriority)
{
}

Sexy::InputDriver::~InputDriver() {}

int Sexy::InputDriver::OnStart(Sexy::SexyAppBase *, Sexy::InputManager *)
{
    return 0;
}

int Sexy::InputDriver::OnStop()
{
    return 0;
}

Sexy::IInputDriver::~IInputDriver() {}

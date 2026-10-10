// InputDeviceListener is the three-slot interface a host registers with
// InputManager. It has no destructor slot and no data: the object is the
// vptr alone. GamepadApp uses it as a second base and replaces the first
// two slots; DeviceEventGenerated stays the empty default.
//
// The three bodies are empty, so they are inline. That keeps the class
// concrete, which is what the binary's table is.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_INPUTDEVICELISTENER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_INPUTDEVICELISTENER_H

#include "ReconTypes.h"

namespace Sexy
{

class InputDeviceListener
{
public:
    virtual int DeviceAdded(Sexy::IInputDriver *theDriver)
    {
        return 0;
    }
    virtual int DeviceRemoved(Sexy::IInputDriver *theDriver)
    {
        return 0;
    }
    virtual void DeviceEventGenerated(Sexy::IInputDriver *theDriver, Sexy::Event *theEvent) {}
};

} // namespace Sexy

using Sexy::InputDeviceListener;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_INPUTDEVICELISTENER_H

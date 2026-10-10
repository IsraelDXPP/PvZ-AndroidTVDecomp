// IInputDriver.h -- the input device interface.
//
// The type InputManager hands back from its driver list, and the type the
// device callbacks (InputDeviceListener::DeviceAdded and friends) take. Until
// now the tree only forward-declared it, so every call site spelled the calls as
// raw vtable-slot invocations.
//
// Slot list -- _ZTVN4Sexy12IInputDriverE @0x68D9D0. Slot k is at
// `vtable + 8 + 4*k` (offset-to-top at +0, typeinfo at +4); the ARM Thumb
// pointers have bit 0 set and are cleared before resolving a name. The table is
// 21 entries long, entry 22 being the next symbol.
//
// The right-hand numbers are the BINARY's byte offsets. Itanium spends two slots
// on a virtual destructor and MSVC spends one, so the declared order is one slot
// shorter than those numbers -- the same note Image.h and NativeDisplay.h carry.
// This class is only ever called through, never constructed here.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_IINPUTDRIVER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_IINPUTDRIVER_H

#include "ReconTypes.h"
#include "SexyString.h"
#include <vector>

namespace Sexy
{

class InputManager;
class InputDriver;

// Axis codes the game's stick table understands. Gamepad::UpdateStates maps a
// code through gGamepadAxisIndex: 0 and 1 are the left stick, 5 and 2 the right
// stick, 16 and 17 the hat. The value is already normalised to -1..1.
enum Axis : int
{
    AXIS_LEFT_X = 0,
    AXIS_LEFT_Y = 1,
    AXIS_RIGHT_X = 5,
    AXIS_RIGHT_Y = 2,
    AXIS_HAT_X = 16,
    AXIS_HAT_Y = 17,
};

// One sample GetAxisInfo hands back. UpdateStates reads the code and the
// value; the rest of the record is the driver's own.
struct InputAxisInfo
{
    int mCode;
    float mValue;
};

// One axis sample a screen reads while a stick is held. mValue is the
// position, already normalised to -1..1. mDeadZone is the driver's ignore band.
struct DriverAxisSample
{
    float mValue;
    float mDeadZone;
};

// Sexy::InputInfo -- the 28-byte device description GetInfo fills in.
//
// The layout comes from InputManager::LogInputDevices @0x3B6E48, whose body
// clears six words after the name and then prints three of them as the
// "keys / pointer / joystick" flags. The last word is seeded to -1. The flags
// are declared `int` rather than `bool` because the body reads and writes them a
// word at a time and the total has to stay 0x1C.
struct InputInfo
{
    String mName;
    int mHasPointer;
    int mHasKeys;
    int mHasTouch;
    int mHasMultiTouch;
    int mHasJoystick;
    int mDeviceIndex;
};

class IInputDriver
{
public:
    virtual ~IInputDriver(); //   0

    // The binary branches on r0 after this slot (InputManager::Add @0x3B7C58).
    virtual int Init() = 0; //   8
    virtual void Cleanup() = 0; //  12
    virtual void WillStop() = 0; //  16
    virtual bool HasEvent() = 0; //  20
    virtual bool GetEvent(Event &theEvent) = 0; //  24
    virtual int Connect() = 0; //  28
    virtual int Reconnect() = 0; //  32
    // InputManager::Update @0x3B7B08 returns whatever this slot leaves in r0.
    virtual int Update() = 0; //  36
    virtual int GetNumDevices() = 0; //  40
    virtual bool GetInfo(InputInfo &theInfo, int theDevice) = 0; //  44
    virtual bool IsGrabbed() = 0; //  48
    virtual void Grab(bool theGrab) = 0; //  52
    virtual bool GetProperty(const String &theName, void *theValue) = 0; //  56
    virtual bool IsEnabled() = 0; //  60
    virtual void Enable(bool theEnable) = 0; //  64
    virtual int GetNumAxes(int theDevice) = 0; //  68
    virtual bool GetAxisInfo(Axis theAxis, InputAxisInfo &theInfo,
        int theDevice) = 0; //  72
    virtual bool GetAxisInfo(std::vector<InputAxisInfo> &theInfos,
        int theDevice) = 0; //  76

    // The almanac scroll re-reads the held stick. The default is a centred
    // stick so a driver with no axes stops the scroll.
    virtual void GetAxisSample(int theDevice, DriverAxisSample &theSample, int theUnused)
    {
        (void)theDevice;
        (void)theUnused;
        theSample.mValue = 0.0f;
        theSample.mDeadZone = 1.0f;
    }

    // InputManager::Add @0x3B7C48 writes these two words into the driver object.
    // They sit after the vptr; the rest of the object is the concrete driver's.
    int mSlot; // +0x08
    int mPad0C; // +0x0C
    InputDriver *mOwner; // +0x10
};

} // namespace Sexy

// The global spellings, which the transcribed bodies use.
using Sexy::Axis;
using Sexy::IInputDriver;
using Sexy::InputAxisInfo;
using Sexy::InputInfo;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_IINPUTDRIVER_H

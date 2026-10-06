// Win32Gamepad.cpp -- poll XInput, push button events, and answer axis
// queries for Gamepad::UpdateStates.

#include "Win32Gamepad.h"

#include "InputManager.h"
#include "ReconHelpers.h"
#include "ReconTypes.h"
#include "SexyAppBase.h"

#include <xinput.h>

#include <cstring>

#pragma comment(lib, "xinput.lib")

namespace
{

const float kStickScale = 1.0f / 32767.0f;
const float kTriggerScale = 1.0f / 255.0f;
const SHORT kStickDeadZone = 8000;
// GamepadInputInterface::UpdateState @0x40E214 and Update @0x40E51C.
const float kDirectionDeadZone = 0.37499f;
const unsigned int kDirectionRepeatDelayMs = 250;
const unsigned int kDirectionRepeatIntervalMs = 100;

struct ButtonBinding
{
    WORD mMask;
    Sexy::KeyCode mCode;
};

const ButtonBinding kXInputButtons[] =
{
    { XINPUT_GAMEPAD_A,              Sexy::KEYCODE_GAMEPAD_A },
    { XINPUT_GAMEPAD_B,              Sexy::KEYCODE_GAMEPAD_B },
    { XINPUT_GAMEPAD_X,              Sexy::KEYCODE_GAMEPAD_X },
    { XINPUT_GAMEPAD_Y,              Sexy::KEYCODE_GAMEPAD_Y },
    { XINPUT_GAMEPAD_LEFT_SHOULDER,  Sexy::KEYCODE_GAMEPAD_TL },
    { XINPUT_GAMEPAD_RIGHT_SHOULDER, Sexy::KEYCODE_GAMEPAD_TR },
    { XINPUT_GAMEPAD_BACK,           Sexy::KEYCODE_GAMEPAD_SELECT },
    { XINPUT_GAMEPAD_START,          Sexy::KEYCODE_GAMEPAD_START },
    { XINPUT_GAMEPAD_LEFT_THUMB,     Sexy::KEYCODE_GAMEPAD_THUMBL },
    { XINPUT_GAMEPAD_RIGHT_THUMB,    Sexy::KEYCODE_GAMEPAD_THUMBR },
};

const int kXInputButtonCount = (int)(sizeof(kXInputButtons) / sizeof(kXInputButtons[0]));
const Sexy::KeyCode kDirectionKeys[] = {
    Sexy::KEYCODE_GAMEPAD_LEFT, Sexy::KEYCODE_GAMEPAD_UP,
    Sexy::KEYCODE_GAMEPAD_RIGHT, Sexy::KEYCODE_GAMEPAD_DOWN,
    Sexy::KEYCODE_GAMEPAD_DPAD_LEFT, Sexy::KEYCODE_GAMEPAD_DPAD_UP,
    Sexy::KEYCODE_GAMEPAD_DPAD_RIGHT, Sexy::KEYCODE_GAMEPAD_DPAD_DOWN
};
const int kDirectionCount = (int)(sizeof(kDirectionKeys) / sizeof(kDirectionKeys[0]));

float StickAxis(SHORT theValue)
{
    if (theValue > -kStickDeadZone && theValue < kStickDeadZone)
        return 0.0f;
    return (float)theValue * kStickScale;
}

} // namespace

// ---------------------------------------------------------------------------
// One XInput slot.
// ---------------------------------------------------------------------------
namespace Sexy
{

struct DirectionButtonState
{
    bool mDown;
    unsigned int mPressTick;
    unsigned int mRepeatCount;
};

struct PolledPad
{
    int   mUserIndex;
    bool  mConnected;
    WORD  mButtons;
    float mLeftX;
    float mLeftY;
    float mRightX;
    float mRightY;
    bool  mTriggerLeft;
    bool  mTriggerRight;
    DirectionButtonState mDirections[kDirectionCount];
    char  mName[128];
};

class Win32GamepadInterface : public IInputDriver
{
public:
    explicit Win32GamepadInterface(InputDriver *theOwner);
    virtual ~Win32GamepadInterface();

    virtual int Init();
    virtual void Cleanup();
    virtual void WillStop();
    virtual bool HasEvent();
    virtual bool GetEvent(Event &theEvent);
    virtual int Connect();
    virtual int Reconnect();
    virtual int Update();
    virtual int GetNumDevices();
    virtual bool GetInfo(InputInfo &theInfo, int theDevice);
    virtual bool IsGrabbed();
    virtual void Grab(bool theGrab);
    virtual bool GetProperty(const String &theName, void *theValue);
    virtual bool IsEnabled();
    virtual void Enable(bool theEnable);
    virtual int GetNumAxes(int theDevice);
    virtual bool GetAxisInfo(Axis theAxis, InputAxisInfo &theInfo, int theDevice);
    virtual bool GetAxisInfo(std::vector<InputAxisInfo> &theInfos, int theDevice);

    void PushButton(int theDevice, KeyCode theKey, bool theDown, unsigned int theModifiers = 0);
    void UpdateDirection(PolledPad &thePad, int theDirection, bool theDown, unsigned int theTick);
    void ApplyRumble();
    void PollXInput();

    bool mEnabled;
    bool mGrabbed;
    std::vector<PolledPad> mPads;
    std::vector<Event> mEvents;

    float mRumbleLow;
    float mRumbleHigh;
    int   mRumbleDevice;
    Sexy::SexyAppBase *mApp;
};

Win32GamepadInterface::Win32GamepadInterface(InputDriver *theOwner)
    : mEnabled(true)
    , mGrabbed(false)
    , mRumbleLow(0.0f)
    , mRumbleHigh(0.0f)
    , mRumbleDevice(-1)
    , mApp(nullptr)
{
    mSlot = 0;
    mPad0C = 0;
    mOwner = theOwner;
}

Win32GamepadInterface::~Win32GamepadInterface()
{
    Cleanup();
}

int Win32GamepadInterface::Init()
{
    mPads.clear();
    for (int i = 0; i < XUSER_MAX_COUNT; i++)
    {
        PolledPad aPad = {};
        aPad.mUserIndex = i;
        aPad.mConnected = false;
        strcpy_s(aPad.mName, "XInput");
        mPads.push_back(aPad);
    }
    return 1;
}

void Win32GamepadInterface::Cleanup()
{
    mPads.clear();
}

void Win32GamepadInterface::WillStop()
{
    Cleanup();
}

bool Win32GamepadInterface::HasEvent()
{
    return !mEvents.empty();
}

bool Win32GamepadInterface::GetEvent(Event &theEvent)
{
    if (mEvents.empty())
        return false;
    theEvent = mEvents.front();
    mEvents.erase(mEvents.begin());
    return true;
}

int Win32GamepadInterface::Connect()
{
    return 1;
}

int Win32GamepadInterface::Reconnect()
{
    Cleanup();
    return Init();
}

void Win32GamepadInterface::PushButton(int theDevice, KeyCode theKey, bool theDown, unsigned int theModifiers)
{
    Event anEvent = {};
    anEvent.modifiers = theDown ? 1 : 2;
    anEvent.modifierMask = theModifiers;
    anEvent.lateModifiers = mSlot;
    anEvent.eventType = (TMLongCard)theDevice;
    anEvent.eventCodeMask = (TMLongCard)theKey;
    mEvents.push_back(anEvent);
    logfi("Pad map %s: device %d -> pad 0x%X", theDown ? "down" : "up", theDevice, theKey);
}

void Win32GamepadInterface::UpdateDirection(PolledPad &thePad, int theDirection,
                                          bool theDown, unsigned int theTick)
{
    DirectionButtonState &aButton = thePad.mDirections[theDirection];
    if (aButton.mDown != theDown)
    {
        PushButton(thePad.mUserIndex, kDirectionKeys[theDirection], theDown, 8);
        aButton.mDown = theDown;
        aButton.mPressTick = theTick;
        aButton.mRepeatCount = theDown ? 1 : 0;
        return;
    }
    if (!theDown)
        return;

    // TV counts the initial press as one before scheduling held directions.
    const unsigned int aHeldMs = theTick - aButton.mPressTick;
    if (aHeldMs > kDirectionRepeatDelayMs
        && aButton.mRepeatCount < (aHeldMs - kDirectionRepeatDelayMs) / kDirectionRepeatIntervalMs)
    {
        PushButton(thePad.mUserIndex, kDirectionKeys[theDirection], true, 8 | 128);
        ++aButton.mRepeatCount;
    }
}

void Win32GamepadInterface::PollXInput()
{
    const unsigned int aTick = Sexy::GetTickCount();
    for (unsigned int i = 0; i < mPads.size(); i++)
    {
        PolledPad &aPad = mPads[i];
        XINPUT_STATE aState;
        memset(&aState, 0, sizeof(aState));
        const DWORD aResult = XInputGetState((DWORD)aPad.mUserIndex, &aState);
        const bool aConnected = aResult == ERROR_SUCCESS;
        if (aConnected != aPad.mConnected)
        {
            aPad.mConnected = aConnected;
            // GamepadApp only marks a slot connected from DeviceAdded. A pad
            // that arrives after the driver starts, or that was already in
            // when the first notice went out, has to raise that notice again.
            if (aConnected && mOwner != nullptr && mApp != nullptr && mApp->mInputManager != nullptr)
                mApp->mInputManager->DispatchEvent(this, DEVICE_ADDED, nullptr);
        }
        if (!aConnected)
        {
            for (int aDirection = 0; aDirection < kDirectionCount; ++aDirection)
                UpdateDirection(aPad, aDirection, false, aTick);
            aPad.mButtons = 0;
            aPad.mLeftX = aPad.mLeftY = aPad.mRightX = aPad.mRightY = 0.0f;
            aPad.mTriggerLeft = aPad.mTriggerRight = false;
            continue;
        }

        const WORD aButtons = aState.Gamepad.wButtons;
        for (int b = 0; b < kXInputButtonCount; b++)
        {
            const bool aWas = (aPad.mButtons & kXInputButtons[b].mMask) != 0;
            const bool aNow = (aButtons & kXInputButtons[b].mMask) != 0;
            if (aWas != aNow)
                PushButton((int)i, kXInputButtons[b].mCode, aNow);
        }

        const bool aLeftTrigger = aState.Gamepad.bLeftTrigger > 30;
        const bool aRightTrigger = aState.Gamepad.bRightTrigger > 30;
        if (aLeftTrigger != aPad.mTriggerLeft)
            PushButton((int)i, KEYCODE_GAMEPAD_TL2, aLeftTrigger);
        if (aRightTrigger != aPad.mTriggerRight)
            PushButton((int)i, KEYCODE_GAMEPAD_TR2, aRightTrigger);

        aPad.mButtons = aButtons;
        aPad.mTriggerLeft = aLeftTrigger;
        aPad.mTriggerRight = aRightTrigger;
        aPad.mLeftX = StickAxis(aState.Gamepad.sThumbLX);
        aPad.mLeftY = -StickAxis(aState.Gamepad.sThumbLY);
        aPad.mRightX = StickAxis(aState.Gamepad.sThumbRX);
        aPad.mRightY = -StickAxis(aState.Gamepad.sThumbRY);

        // HandleAxisChanged @0x40D55C emits stick directions separately from
        // d-pad directions; the game still reads the analog axes for movement.
        const bool aDirections[kDirectionCount] = {
            aPad.mLeftX < -kDirectionDeadZone,
            aPad.mLeftY < -kDirectionDeadZone,
            aPad.mLeftX > kDirectionDeadZone,
            aPad.mLeftY > kDirectionDeadZone,
            (aButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0,
            (aButtons & XINPUT_GAMEPAD_DPAD_UP) != 0,
            (aButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0,
            (aButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0
        };
        for (int aDirection = 0; aDirection < kDirectionCount; ++aDirection)
            UpdateDirection(aPad, aDirection, aDirections[aDirection], aTick);

        if ((int)i == mRumbleDevice)
        {
            XINPUT_VIBRATION aVibration;
            aVibration.wLeftMotorSpeed = (WORD)(mRumbleLow * 65535.0f);
            aVibration.wRightMotorSpeed = (WORD)(mRumbleHigh * 65535.0f);
            XInputSetState((DWORD)aPad.mUserIndex, &aVibration);
        }
    }
}

int Win32GamepadInterface::Update()
{
    PollXInput();

    if (mOwner == nullptr)
        return (int)mEvents.size();

    // The events are delivered by the manager's own queue. The owner does not
    // hold the manager, so the caller (InputManager::Update) is expected to
    // drain GetEvent. Returning the pending count keeps the manager's result.
    return (int)mEvents.size();
}

int Win32GamepadInterface::GetNumDevices()
{
    // Count live pads. An empty XInput slot is not a connected device, and
    // reporting the four slots made every launch look like a gamepad.
    int aCount = 0;
    for (unsigned int i = 0; i < mPads.size(); i++)
    {
        if (mPads[i].mConnected)
            ++aCount;
    }
    return aCount;
}

bool Win32GamepadInterface::GetInfo(InputInfo &theInfo, int theDevice)
{
    if (theDevice < 0 || theDevice >= (int)mPads.size())
        return false;
    if (!mPads[theDevice].mConnected)
        return false;

    theInfo.mName = mPads[theDevice].mName;
    theInfo.mHasPointer = 0;
    theInfo.mHasKeys = 1;
    theInfo.mHasTouch = 0;
    theInfo.mHasMultiTouch = 0;
    theInfo.mHasJoystick = 1;
    theInfo.mDeviceIndex = theDevice;
    return true;
}

bool Win32GamepadInterface::IsGrabbed()
{
    return mGrabbed;
}

void Win32GamepadInterface::Grab(bool theGrab)
{
    mGrabbed = theGrab;
}

bool Win32GamepadInterface::GetProperty(const String &, void *)
{
    return false;
}

bool Win32GamepadInterface::IsEnabled()
{
    return mEnabled;
}

void Win32GamepadInterface::Enable(bool theEnable)
{
    mEnabled = theEnable;
}

int Win32GamepadInterface::GetNumAxes(int)
{
    return 4;
}

bool Win32GamepadInterface::GetAxisInfo(Axis theAxis, InputAxisInfo &theInfo, int theDevice)
{
    std::vector<InputAxisInfo> aAll;
    if (!GetAxisInfo(aAll, theDevice))
        return false;
    for (unsigned int i = 0; i < aAll.size(); i++)
    {
        if (aAll[i].mCode == (int)theAxis)
        {
            theInfo = aAll[i];
            return true;
        }
    }
    return false;
}

bool Win32GamepadInterface::GetAxisInfo(std::vector<InputAxisInfo> &theInfos, int theDevice)
{
    theInfos.clear();
    if (theDevice < 0 || theDevice >= (int)mPads.size() || !mPads[theDevice].mConnected)
        return false;

    const PolledPad &aPad = mPads[theDevice];
    InputAxisInfo anAxis;
    anAxis.mCode = AXIS_LEFT_X;
    anAxis.mValue = aPad.mLeftX;
    theInfos.push_back(anAxis);
    anAxis.mCode = AXIS_LEFT_Y;
    anAxis.mValue = aPad.mLeftY;
    theInfos.push_back(anAxis);
    anAxis.mCode = AXIS_RIGHT_X;
    anAxis.mValue = aPad.mRightX;
    theInfos.push_back(anAxis);
    anAxis.mCode = AXIS_RIGHT_Y;
    anAxis.mValue = aPad.mRightY;
    theInfos.push_back(anAxis);
    return true;
}

void Win32GamepadInterface::ApplyRumble()
{
}

// ---------------------------------------------------------------------------
// The factory entry. Init builds the interface the manager then owns.
// ---------------------------------------------------------------------------

Win32GamepadDriver::Win32GamepadDriver()
    : InputDriver(Sexy::String("Win32Gamepad"), 0)
    , mInterface(nullptr)
{
}

Win32GamepadDriver::~Win32GamepadDriver()
{
    mInterface = nullptr;
}

IInputDriver *Win32GamepadDriver::Init()
{
    Win32GamepadInterface *anInterface = new Win32GamepadInterface(this);
    if (anInterface->Init() == 0)
    {
        delete anInterface;
        return nullptr;
    }
    mInterface = anInterface;
    return anInterface;
}

int Win32GamepadDriver::OnStart(Sexy::SexyAppBase *theApp, Sexy::InputManager *)
{
    if (mInterface != nullptr)
        mInterface->mApp = theApp;
    return 0;
}

int Win32GamepadDriver::OnStop()
{
    mInterface = nullptr;
    return InputDriver::OnStop();
}

IInputDriver *CreateWin32Gamepad(InputDriver *theOwner)
{
    Win32GamepadInterface *anInterface = new Win32GamepadInterface(theOwner);
    return anInterface;
}

} // namespace Sexy

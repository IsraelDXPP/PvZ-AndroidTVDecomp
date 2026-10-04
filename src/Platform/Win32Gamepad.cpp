// Win32Gamepad.cpp -- poll XInput and DirectInput, push button events, and
// answer axis queries for Gamepad::UpdateStates.

#include "Win32Gamepad.h"

#include "InputManager.h"
#include "ReconTypes.h"

#include <dinput.h>
#include <xinput.h>

#include <cstring>

#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace
{

const float kStickScale = 1.0f / 32767.0f;
const float kTriggerScale = 1.0f / 255.0f;
const SHORT kStickDeadZone = 8000;

// Numeric values of Sexy::KeyCode. windows.h pulls in winuser.h, whose
// KEYCODE_* macros hide the enumerators, so the table stores the numbers.
const int kPadA = 0x130;
const int kPadB = 0x131;
const int kPadX = 0x133;
const int kPadY = 0x134;
const int kPadTL = 0x136;
const int kPadTR = 0x137;
const int kPadTL2 = 0x138;
const int kPadTR2 = 0x139;
const int kPadSelect = 0x13A;
const int kPadStart = 0x13B;
const int kPadThumbL = 0x13D;
const int kPadThumbR = 0x13E;
const int kPadDpadLeft = 0x234;
const int kPadDpadUp = 0x235;
const int kPadDpadRight = 0x236;
const int kPadDpadDown = 0x237;

struct ButtonBinding
{
    WORD mMask;
    int  mCode;
};

const ButtonBinding kXInputButtons[] =
{
    { XINPUT_GAMEPAD_A,              kPadA },
    { XINPUT_GAMEPAD_B,              kPadB },
    { XINPUT_GAMEPAD_X,              kPadX },
    { XINPUT_GAMEPAD_Y,              kPadY },
    { XINPUT_GAMEPAD_LEFT_SHOULDER,  kPadTL },
    { XINPUT_GAMEPAD_RIGHT_SHOULDER, kPadTR },
    { XINPUT_GAMEPAD_BACK,           kPadSelect },
    { XINPUT_GAMEPAD_START,          kPadStart },
    { XINPUT_GAMEPAD_LEFT_THUMB,     kPadThumbL },
    { XINPUT_GAMEPAD_RIGHT_THUMB,    kPadThumbR },
    { XINPUT_GAMEPAD_DPAD_UP,        kPadDpadUp },
    { XINPUT_GAMEPAD_DPAD_DOWN,      kPadDpadDown },
    { XINPUT_GAMEPAD_DPAD_LEFT,      kPadDpadLeft },
    { XINPUT_GAMEPAD_DPAD_RIGHT,     kPadDpadRight },
};

const int kXInputButtonCount = (int)(sizeof(kXInputButtons) / sizeof(kXInputButtons[0]));

float StickAxis(SHORT theValue)
{
    if (theValue > -kStickDeadZone && theValue < kStickDeadZone)
        return 0.0f;
    return (float)theValue * kStickScale;
}

// XInput devices answer this property. DirectInput skips them.
bool IsXInputDevice(const GUID &theProduct)
{
    // Xbox pads advertise VID 0x045E and a known PID range. The product GUID's
    // first dword packs those two ids, which is enough to skip a pad XInput
    // already owns.
    const DWORD aVidPid = theProduct.Data1;
    const WORD aVendor = (WORD)(aVidPid & 0xFFFF);
    const WORD aProduct = (WORD)((aVidPid >> 16) & 0xFFFF);
    if (aVendor != 0x045E)
        return false;
    if (aProduct >= 0x028E && aProduct <= 0x02A1)
        return true;
    if (aProduct >= 0x02D1 && aProduct <= 0x02FF)
        return true;
    if (aProduct == 0x0719)
        return true;
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
// One polled pad. XInput slots and DirectInput devices share this record so
// the event and axis paths do not care which API produced the sample.
// ---------------------------------------------------------------------------
namespace Sexy
{

struct PolledPad
{
    bool  mXInput;
    int   mUserIndex;
    bool  mConnected;
    WORD  mButtons;
    float mLeftX;
    float mLeftY;
    float mRightX;
    float mRightY;
    bool  mTriggerLeft;
    bool  mTriggerRight;

    IDirectInputDevice8 *mDevice;
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

    void PushButton(int theDevice, int theKey, bool theDown);
    void ApplyRumble();
    void PollXInput();
    void PollDirectInput();
    void EnumerateDirectInput();

    bool mEnabled;
    bool mGrabbed;
    IDirectInput8 *mDirectInput;
    std::vector<PolledPad> mPads;
    std::vector<Event> mEvents;

    float mRumbleLow;
    float mRumbleHigh;
    int   mRumbleDevice;
};

Win32GamepadInterface::Win32GamepadInterface(InputDriver *theOwner)
    : mEnabled(true)
    , mGrabbed(false)
    , mDirectInput(nullptr)
    , mRumbleLow(0.0f)
    , mRumbleHigh(0.0f)
    , mRumbleDevice(-1)
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
        aPad.mXInput = true;
        aPad.mUserIndex = i;
        aPad.mConnected = false;
        aPad.mDevice = nullptr;
        strcpy_s(aPad.mName, "XInput");
        mPads.push_back(aPad);
    }
    EnumerateDirectInput();
    return 1;
}

void Win32GamepadInterface::Cleanup()
{
    for (unsigned int i = 0; i < mPads.size(); i++)
    {
        if (mPads[i].mDevice != nullptr)
        {
            mPads[i].mDevice->Unacquire();
            mPads[i].mDevice->Release();
            mPads[i].mDevice = nullptr;
        }
    }
    mPads.clear();
    if (mDirectInput != nullptr)
    {
        mDirectInput->Release();
        mDirectInput = nullptr;
    }
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

void Win32GamepadInterface::PushButton(int theDevice, int theKey, bool theDown)
{
    Event anEvent = {};
    anEvent.modifiers = theDown ? 1 : 2;
    anEvent.lateModifiers = mSlot;
    anEvent.eventType = (TMLongCard)theDevice;
    anEvent.eventCodeMask = (TMLongCard)theKey;
    mEvents.push_back(anEvent);
    if (mOwner != nullptr)
    {
        // The manager is reached through the app the owner was started with.
        // Events queued here are also drained by Update into PushEvent, so a
        // missing manager only delays them until the next poll.
    }
}

void Win32GamepadInterface::PollXInput()
{
    for (unsigned int i = 0; i < mPads.size(); i++)
    {
        PolledPad &aPad = mPads[i];
        if (!aPad.mXInput)
            continue;

        XINPUT_STATE aState;
        memset(&aState, 0, sizeof(aState));
        const DWORD aResult = XInputGetState((DWORD)aPad.mUserIndex, &aState);
        const bool aConnected = aResult == ERROR_SUCCESS;
        if (aConnected != aPad.mConnected)
            aPad.mConnected = aConnected;
        if (!aConnected)
        {
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
            PushButton((int)i, kPadTL2, aLeftTrigger);
        if (aRightTrigger != aPad.mTriggerRight)
            PushButton((int)i, kPadTR2, aRightTrigger);

        aPad.mButtons = aButtons;
        aPad.mTriggerLeft = aLeftTrigger;
        aPad.mTriggerRight = aRightTrigger;
        aPad.mLeftX = StickAxis(aState.Gamepad.sThumbLX);
        aPad.mLeftY = -StickAxis(aState.Gamepad.sThumbLY);
        aPad.mRightX = StickAxis(aState.Gamepad.sThumbRX);
        aPad.mRightY = -StickAxis(aState.Gamepad.sThumbRY);

        if ((int)i == mRumbleDevice)
        {
            XINPUT_VIBRATION aVibration;
            aVibration.wLeftMotorSpeed = (WORD)(mRumbleLow * 65535.0f);
            aVibration.wRightMotorSpeed = (WORD)(mRumbleHigh * 65535.0f);
            XInputSetState((DWORD)aPad.mUserIndex, &aVibration);
        }
    }
}

namespace
{

struct EnumContext
{
    Win32GamepadInterface *mInterface;
};

BOOL CALLBACK EnumGameControllers(const DIDEVICEINSTANCE *theInstance, VOID *theContext)
{
    EnumContext *aContext = (EnumContext *)theContext;
    if (IsXInputDevice(theInstance->guidProduct))
        return DIENUM_CONTINUE;

    IDirectInputDevice8 *aDevice = nullptr;
    if (FAILED(aContext->mInterface->mDirectInput->CreateDevice(
            theInstance->guidInstance, &aDevice, nullptr)))
        return DIENUM_CONTINUE;

    if (FAILED(aDevice->SetDataFormat(&c_dfDIJoystick)) ||
        FAILED(aDevice->SetCooperativeLevel(GetForegroundWindow(), DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)) ||
        FAILED(aDevice->Acquire()))
    {
        aDevice->Release();
        return DIENUM_CONTINUE;
    }

    PolledPad aPad = {};
    aPad.mXInput = false;
    aPad.mUserIndex = -1;
    aPad.mConnected = true;
    aPad.mDevice = aDevice;
    strncpy_s(aPad.mName, theInstance->tszProductName, sizeof(aPad.mName) - 1);
    aContext->mInterface->mPads.push_back(aPad);
    return DIENUM_CONTINUE;
}

} // namespace

void Win32GamepadInterface::EnumerateDirectInput()
{
    if (mDirectInput == nullptr)
    {
        if (FAILED(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION,
                                      IID_IDirectInput8, (void **)&mDirectInput, nullptr)))
            return;
    }

    EnumContext aContext;
    aContext.mInterface = this;
    mDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumGameControllers, &aContext, DIEDFL_ATTACHEDONLY);
}

void Win32GamepadInterface::PollDirectInput()
{
    for (unsigned int i = 0; i < mPads.size(); i++)
    {
        PolledPad &aPad = mPads[i];
        if (aPad.mXInput || aPad.mDevice == nullptr)
            continue;

        DIJOYSTATE aState;
        memset(&aState, 0, sizeof(aState));
        HRESULT aResult = aPad.mDevice->Poll();
        if (FAILED(aResult))
        {
            aResult = aPad.mDevice->Acquire();
            if (FAILED(aResult))
            {
                aPad.mConnected = false;
                continue;
            }
            aPad.mDevice->Poll();
        }
        if (FAILED(aPad.mDevice->GetDeviceState(sizeof(aState), &aState)))
        {
            aPad.mConnected = false;
            continue;
        }
        aPad.mConnected = true;

        // Buttons 0..13 follow the same order as the XInput table so a generic
        // pad still produces the keys MapFromkeyCode understands.
        const int aKeys[14] =
        {
            kPadA, kPadB, kPadX, kPadY,
            kPadTL, kPadTR, kPadSelect, kPadStart,
            kPadThumbL, kPadThumbR,
            kPadDpadUp, kPadDpadDown,
            kPadDpadLeft, kPadDpadRight,
        };
        WORD aButtons = 0;
        for (int b = 0; b < 14; b++)
        {
            const bool aNow = (aState.rgbButtons[b] & 0x80) != 0;
            const bool aWas = (aPad.mButtons & (1u << b)) != 0;
            if (aNow)
                aButtons = (WORD)(aButtons | (1u << b));
            if (aNow != aWas)
                PushButton((int)i, aKeys[b], aNow);
        }

        const bool aLeftTrigger = aState.lZ > 30;
        const bool aRightTrigger = aState.lRz > 30;
        if (aLeftTrigger != aPad.mTriggerLeft)
            PushButton((int)i, kPadTL2, aLeftTrigger);
        if (aRightTrigger != aPad.mTriggerRight)
            PushButton((int)i, kPadTR2, aRightTrigger);

        aPad.mButtons = aButtons;
        aPad.mTriggerLeft = aLeftTrigger;
        aPad.mTriggerRight = aRightTrigger;

        const LONG aCenter = 32767;
        auto AxisOf = [](LONG theValue) -> float
        {
            const int aDelta = (int)theValue - 32767;
            if (aDelta > -kStickDeadZone && aDelta < kStickDeadZone)
                return 0.0f;
            return (float)aDelta * kStickScale;
        };
        (void)aCenter;
        aPad.mLeftX = AxisOf(aState.lX);
        aPad.mLeftY = AxisOf(aState.lY);
        aPad.mRightX = AxisOf(aState.lRx);
        aPad.mRightY = AxisOf(aState.lRy);
    }
}

int Win32GamepadInterface::Update()
{
    PollXInput();
    PollDirectInput();

    if (mOwner == nullptr)
        return (int)mEvents.size();

    // The events are delivered by the manager's own queue. The owner does not
    // hold the manager, so the caller (InputManager::Update) is expected to
    // drain GetEvent. Returning the pending count keeps the manager's result.
    return (int)mEvents.size();
}

int Win32GamepadInterface::GetNumDevices()
{
    int aCount = 0;
    for (unsigned int i = 0; i < mPads.size(); i++)
    {
        if (mPads[i].mConnected)
            ++aCount;
    }
    // The original recognises a gamepad only when the driver reports more
    // than three devices. Four XInput slots satisfy that even before a pad
    // is plugged in, so the count is the slot count, not the live count.
    return (int)mPads.size() > aCount ? (int)mPads.size() : aCount;
}

bool Win32GamepadInterface::GetInfo(InputInfo &theInfo, int theDevice)
{
    if (theDevice < 0 || theDevice >= (int)mPads.size())
        return false;
    if (!mPads[theDevice].mConnected && !mPads[theDevice].mXInput)
        return false;

    theInfo.mName = mPads[theDevice].mName;
    theInfo.mHasPointer = 0;
    theInfo.mHasKeys = 1;
    theInfo.mHasTouch = 1;
    theInfo.mHasMultiTouch = 0;
    theInfo.mHasJoystick = 1;
    theInfo.mDeviceIndex = theDevice;
    return mPads[theDevice].mConnected || mPads[theDevice].mXInput;
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

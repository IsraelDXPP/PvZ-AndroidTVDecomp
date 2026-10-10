// Gamepad polls the bound input driver, stores button and axis state, and
// forwards button and axis events to the listener's first three virtuals.

#include "Gamepad.h"

#include "IInputDriver.h"
#include "InputManager.h"
#include "ReconData.h"
#include "SexyAppBase.h"

#include <cmath>

namespace
{

const int kAxisCount = 6;
const int kButtonCount = 20;
const float kAxisScale = 32768.0f;
const float kStickDeadZone = 9000.0f;
const int kAxisMoveThreshold = 255;
const unsigned int kIdleTimeoutMs = 20000;
const int kGamepadLogTag = 0x656D6167;

// Driver axis codes. Bit 0, 5 and 16 are the positive half; bit 1, 2 and 17
// are the negative half.
const unsigned int kAxisPositiveMask = 0x10021;
const unsigned int kAxisNegativeMask = 0x20006;

class GamepadListenerView
{
public:
    virtual void OnButtonDown(int theButton, int theGamepadIndex, unsigned int theFlags) = 0;
    virtual void OnButtonUp(int theButton, int theGamepadIndex, unsigned int theFlags) = 0;
    virtual void OnGameAxisMove(int theAxis, int theGamepadIndex, int theValue) = 0;
};

GamepadListenerView *AsListener(void *theListener)
{
    return static_cast<GamepadListenerView *>(theListener);
}

float AxisRawToUnit(int theRaw)
{
    return (float)theRaw / kAxisScale;
}

int AxisUnitToRaw(float theValue)
{
    return (int)(theValue * kAxisScale);
}

float RampStickComponent(int theRawX, int theRawY, bool theUseX)
{
    float aX = (float)theRawX;
    float aY = (float)theRawY;
    float aLength = sqrtf(aX * aX + aY * aY);
    if (aLength < kStickDeadZone)
        return 0.0f;

    float aOutside = aLength - kStickDeadZone;
    if (aLength != 0.0f)
    {
        aX /= aLength;
        aY /= aLength;
    }

    float aComponent = theUseX ? aX : aY;
    float aOther = theUseX ? aY : aX;
    float aRamped = aComponent * aOutside;
    float aRampedLength = sqrtf(aRamped * aRamped + (aOther * aOutside) * (aOther * aOutside));
    if (aRampedLength != 0.0f)
        aRamped /= aRampedLength;
    return aRamped;
}

} // namespace

Sexy::Gamepad::~Gamepad() {}

bool Sexy::Gamepad::IsConnected()
{
    return mIsConnected;
}

void Sexy::Gamepad::SetWeight(int theAxis, int theWeight)
{
    (void)theAxis;
    (void)theWeight;
}

int Sexy::Gamepad::GetWeight(int theAxis)
{
    (void)theAxis;
    return 0;
}

void Sexy::Gamepad::SetPadScale(int theAxis, float theScale)
{
    (void)theAxis;
    (void)theScale;
}

float Sexy::Gamepad::GetPadScale(int theAxis)
{
    (void)theAxis;
    return 0.0f;
}

bool Sexy::Gamepad::IsButtonDown(int theButton)
{
    if (theButton < 0 || theButton >= kButtonCount)
        return false;
    return mButtonDown[theButton] != 0;
}

float Sexy::Gamepad::GetButtonPressure(int theButton)
{
    return IsButtonDown(theButton) ? 1.0f : 0.0f;
}

float Sexy::Gamepad::GetAxisXPosition()
{
    return AxisRawToUnit(mAxisRaw[0]);
}

float Sexy::Gamepad::GetAxisYPosition()
{
    return AxisRawToUnit(mAxisRaw[1]);
}

float Sexy::Gamepad::GetRightAxisXPosition()
{
    return AxisRawToUnit(mAxisRaw[4]);
}

float Sexy::Gamepad::GetRightAxisYPosition()
{
    return AxisRawToUnit(mAxisRaw[5]);
}

void Sexy::Gamepad::AddRumbleEffect(float theLow, float theHigh, float theDuration)
{
    mRumbleLow = theLow > 1.0f ? 1.0f : theLow;
    mRumbleHigh = theHigh > 1.0f ? 1.0f : theHigh;
    mRumbleDurationLow = theDuration;
    mRumbleDurationHigh = theDuration;
}

void Sexy::Gamepad::Reset() {}

float Sexy::Gamepad::GetAxisYPositionRamped()
{
    return RampStickComponent(mAxisRaw[0], mAxisRaw[1], false);
}

float Sexy::Gamepad::GetAxisXPositionRamped()
{
    return RampStickComponent(mAxisRaw[0], mAxisRaw[1], true);
}

int Sexy::Gamepad::MapTokeyCode(Sexy::GamepadButton theButton)
{
    if (theButton > GAMEPAD_BUTTON_DPAD_RIGHT)
        return 0;
    return gGamepadKeyCodes[theButton];
}

int Sexy::Gamepad::MapFromkeyCode(Sexy::KeyCode theKey)
{
    switch (theKey)
    {
        case KEYCODE_GAMEPAD_A:
            return GAMEPAD_BUTTON_A;
        case KEYCODE_GAMEPAD_B:
            return GAMEPAD_BUTTON_B;
        case KEYCODE_GAMEPAD_C:
            return GAMEPAD_BUTTON_START;
        case KEYCODE_GAMEPAD_X:
            return GAMEPAD_BUTTON_X;
        case KEYCODE_GAMEPAD_Y:
            return GAMEPAD_BUTTON_Y;
        case KEYCODE_GAMEPAD_Z:
            return GAMEPAD_BUTTON_SELECT;
        case KEYCODE_GAMEPAD_TL:
            return GAMEPAD_BUTTON_TL;
        case KEYCODE_GAMEPAD_TR:
            return GAMEPAD_BUTTON_TR;
        case KEYCODE_GAMEPAD_TL2:
            return GAMEPAD_BUTTON_TL2;
        case KEYCODE_GAMEPAD_TR2:
            return GAMEPAD_BUTTON_TR2;
        case KEYCODE_GAMEPAD_SELECT:
            return GAMEPAD_BUTTON_SELECT;
        case KEYCODE_GAMEPAD_START:
            return GAMEPAD_BUTTON_START;
        case KEYCODE_GAMEPAD_THUMBL:
            return GAMEPAD_BUTTON_THUMBL;
        case KEYCODE_GAMEPAD_THUMBR:
            return GAMEPAD_BUTTON_THUMBR;
        case KEYCODE_GAMEPAD_LEFT:
            return GAMEPAD_BUTTON_LEFT;
        case KEYCODE_GAMEPAD_UP:
            return GAMEPAD_BUTTON_UP;
        case KEYCODE_GAMEPAD_RIGHT:
            return GAMEPAD_BUTTON_RIGHT;
        case KEYCODE_GAMEPAD_DOWN:
            return GAMEPAD_BUTTON_DOWN;
        case KEYCODE_GAMEPAD_DPAD_LEFT:
            return GAMEPAD_BUTTON_DPAD_LEFT;
        case KEYCODE_GAMEPAD_DPAD_UP:
            return GAMEPAD_BUTTON_DPAD_UP;
        case KEYCODE_GAMEPAD_DPAD_RIGHT:
            return GAMEPAD_BUTTON_DPAD_RIGHT;
        case KEYCODE_GAMEPAD_DPAD_DOWN:
            return GAMEPAD_BUTTON_DPAD_DOWN;
        default:
            return -1;
    }
}

Sexy::Gamepad::Gamepad(Sexy::SexyAppBase *theApp, int theIndex) :
    mApp(theApp),
    mGamepadIndex(theIndex),
    mDeviceId(0),
    mListener(nullptr),
    mIsConnected(false),
    mStatus(STATUS_DISCONNECTED),
    mStatusChangeTick(0),
    mActiveCount(0),
    mRumbleLow(0.0f),
    mRumbleHigh(0.0f),
    mRumbleDurationLow(0.0f),
    mRumbleDurationHigh(0.0f)
{
    for (int i = 0; i < kAxisCount; ++i)
    {
        mAxisRaw[i] = 0;
        mAxisValue[i] = 0.0f;
    }
    for (int i = 0; i < kButtonCount; ++i)
        mButtonDown[i] = 0;
}

int Sexy::Gamepad::GetGamepadIndex()
{
    return mGamepadIndex;
}

void Sexy::Gamepad::SetGamepadIndex(int theIndex)
{
    mGamepadIndex = theIndex;
}

void Sexy::Gamepad::SetListener(void *theListener)
{
    mListener = theListener;
}

void *Sexy::Gamepad::GetListener()
{
    return mListener;
}

void Sexy::Gamepad::SetStatus(Status theStatus)
{
    bool aChanged = true;
    if (theStatus == STATUS_ACTIVE)
    {
        Status aPrevious = mStatus;
        mStatusChangeTick = Sexy::GetTickCount();
        if (aPrevious == STATUS_ACTIVE)
            aChanged = false;
        else
            ++mActiveCount;
    }
    else if (theStatus == mStatus)
    {
        aChanged = false;
    }

    if (aChanged)
    {
        Sexy::logtfi(kGamepadLogTag,
            "%d: status %d -> %d actives: %d",
            mGamepadIndex,
            (int)mStatus,
            (int)theStatus,
            mActiveCount);
    }
    mStatus = theStatus;
}

void Sexy::Gamepad::UpdateStates(float theDelta)
{
    (void)theDelta;
    if (mGamepadIndex == -1 || mApp == nullptr || mDeviceId == 0)
        return;

    Sexy::IInputDriver *aDriver = mApp->mInputManager->Find(mDeviceId >> 16);
    if (aDriver == nullptr)
    {
        SetStatus(STATUS_DISCONNECTED);
        mIsConnected = false;
        mDeviceId = 0;
        return;
    }

    std::vector<Sexy::InputAxisInfo> aEvents;
    if (!aDriver->GetAxisInfo(aEvents, mDeviceId & 65535))
        return;

    bool aMoved = false;
    for (unsigned int i = 0; i < aEvents.size(); ++i)
    {
        const Sexy::InputAxisInfo &anEvent = aEvents[i];
        const int aCode = anEvent.mCode;
        if (aCode < 0 || aCode > 17)
            continue;

        const bool aNegative = ((1u << aCode) & kAxisNegativeMask) != 0;
        if (!aNegative && ((1u << aCode) & kAxisPositiveMask) == 0)
            continue;

        // Code 0 has no table entry. Codes 1..17 index the map. An unsigned
        // compare of (code - 1) against 16 is what selects that range.
        int anIndex = 0;
        if (aCode >= 1)
            anIndex = gGamepadAxisIndex[aCode - 1];
        if (anIndex < 0 || anIndex >= kAxisCount)
            continue;

        const float aValue = aNegative ? -anEvent.mValue : anEvent.mValue;
        const int aNewRaw = AxisUnitToRaw(aValue);
        const int anOldRaw = mAxisRaw[anIndex];
        mAxisValue[anIndex] = aValue;
        mAxisRaw[anIndex] = aNewRaw;
        int aDelta = aNewRaw - anOldRaw;
        if (aDelta < 0)
            aDelta = -aDelta;
        if (aDelta > kAxisMoveThreshold)
            aMoved = true;
    }

    unsigned int aTick = Sexy::GetTickCount();
    if (aMoved)
    {
        SetStatus(STATUS_ACTIVE);
        mStatusChangeTick = Sexy::GetTickCount();
    }
    else if (mStatus == STATUS_ACTIVE && Sexy::TickCountDiff(aTick, mStatusChangeTick) > kIdleTimeoutMs)
    {
        SetStatus(STATUS_IDLE);
    }
}

bool Sexy::Gamepad::HandleButtonDown(int theButton, unsigned int theFlags)
{
    GamepadListenerView *aListener = AsListener(mListener);
    if (aListener == nullptr)
        return false;
    aListener->OnButtonDown(theButton, mGamepadIndex, theFlags);
    return true;
}

bool Sexy::Gamepad::HandleButtonUp(int theButton, unsigned int theFlags)
{
    GamepadListenerView *aListener = AsListener(mListener);
    if (aListener == nullptr)
        return false;
    aListener->OnButtonUp(theButton, mGamepadIndex, theFlags);
    return true;
}

bool Sexy::Gamepad::HandleGameAxisMove(int theAxis, int theValue)
{
    GamepadListenerView *aListener = AsListener(mListener);
    if (aListener == nullptr)
        return false;
    aListener->OnGameAxisMove(theAxis, mGamepadIndex, theValue);
    return true;
}

bool Sexy::Gamepad::CheckDeviceAdded(int theDeviceId)
{
    if (mDeviceId != 0)
        return false;
    mDeviceId = theDeviceId;
    mIsConnected = true;
    SetStatus(STATUS_CONNECTED);
    return true;
}

bool Sexy::Gamepad::CheckDeviceRemoved(int theDeviceId)
{
    if (mDeviceId != theDeviceId)
        return false;
    SetStatus(STATUS_DISCONNECTED);
    mIsConnected = false;
    mDeviceId = 0;
    mActiveCount = 0;
    for (int anAxis = 0; anAxis < kAxisCount; ++anAxis)
    {
        mAxisRaw[anAxis] = 0;
        mAxisValue[anAxis] = 0.0f;
    }
    for (int aButton = 0; aButton < kButtonCount; ++aButton)
        mButtonDown[aButton] = 0;
    return true;
}

void Sexy::Gamepad::CheckEvent(Sexy::Event const &theEvent)
{
    if (theEvent.modifiers == 1)
    {
        int aButton = MapFromkeyCode((Sexy::KeyCode)theEvent.eventCodeMask.mValue);
        if (aButton >= 0 && aButton < kButtonCount)
            mButtonDown[aButton] = 1;
        SetStatus(STATUS_ACTIVE);
    }
    else if (theEvent.modifiers == 2)
    {
        int aButton = MapFromkeyCode((Sexy::KeyCode)theEvent.eventCodeMask.mValue);
        if (aButton >= 0 && aButton < kButtonCount)
            mButtonDown[aButton] = 0;
        SetStatus(STATUS_ACTIVE);
    }
}

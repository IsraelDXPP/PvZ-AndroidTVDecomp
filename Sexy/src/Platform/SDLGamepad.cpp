// SDLGamepad.cpp -- poll SDL gamepad, push button events, and answer axis
// queries for Gamepad::UpdateStates.

#include "SDLGamepad.h"
#include "SDLFile.h"

#include "InputManager.h"
#include "ReconHelpers.h"
#include "ReconTypes.h"
#include "SexyAppBase.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <string>

#include <cstring>



namespace
{

const float kStickScale = 1.0f / 32767.0f;
const Sint16 kStickDeadZone = 8000;
// GamepadInputInterface::UpdateState @0x40E214 and Update @0x40E51C.
const float kDirectionDeadZone = 0.37499f;
const unsigned int kDirectionRepeatDelayMs = 250;
const unsigned int kDirectionRepeatIntervalMs = 100;

struct ButtonBinding
{
    SDL_GamepadButton mButton;
    Sexy::KeyCode mCode;
};

const ButtonBinding kButtons[] =
{
    { SDL_GAMEPAD_BUTTON_SOUTH,              Sexy::KEYCODE_GAMEPAD_A },
    { SDL_GAMEPAD_BUTTON_EAST,              Sexy::KEYCODE_GAMEPAD_B },
    { SDL_GAMEPAD_BUTTON_WEST,              Sexy::KEYCODE_GAMEPAD_X },
    { SDL_GAMEPAD_BUTTON_NORTH,              Sexy::KEYCODE_GAMEPAD_Y },
    { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  Sexy::KEYCODE_GAMEPAD_TL },
    { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, Sexy::KEYCODE_GAMEPAD_TR },
    { SDL_GAMEPAD_BUTTON_BACK,           Sexy::KEYCODE_GAMEPAD_SELECT },
    { SDL_GAMEPAD_BUTTON_START,          Sexy::KEYCODE_GAMEPAD_START },
    { SDL_GAMEPAD_BUTTON_LEFT_STICK,     Sexy::KEYCODE_GAMEPAD_THUMBL },
    { SDL_GAMEPAD_BUTTON_RIGHT_STICK,    Sexy::KEYCODE_GAMEPAD_THUMBR },
};

const int kButtonCount = (int)(sizeof(kButtons) / sizeof(kButtons[0]));
const Sexy::KeyCode kDirectionKeys[] = {
    Sexy::KEYCODE_GAMEPAD_LEFT, Sexy::KEYCODE_GAMEPAD_UP,
    Sexy::KEYCODE_GAMEPAD_RIGHT, Sexy::KEYCODE_GAMEPAD_DOWN,
    Sexy::KEYCODE_GAMEPAD_DPAD_LEFT, Sexy::KEYCODE_GAMEPAD_DPAD_UP,
    Sexy::KEYCODE_GAMEPAD_DPAD_RIGHT, Sexy::KEYCODE_GAMEPAD_DPAD_DOWN
};
const int kDirectionCount = (int)(sizeof(kDirectionKeys) / sizeof(kDirectionKeys[0]));

bool MapUnrecognizedGamepad(SDL_JoystickID theInstanceId)
{
    SDL_JoystickType aType = SDL_GetJoystickTypeForID(theInstanceId);
    if (aType != SDL_JOYSTICK_TYPE_UNKNOWN && aType != SDL_JOYSTICK_TYPE_GAMEPAD)
        return false;

    SDL_Joystick* aJoystick = SDL_OpenJoystick(theInstanceId);
    if (aJoystick == nullptr)
        return false;

    const int anAxisCount = SDL_GetNumJoystickAxes(aJoystick);
    const int aButtonCount = SDL_GetNumJoystickButtons(aJoystick);
    const int aHatCount = SDL_GetNumJoystickHats(aJoystick);
    if (aButtonCount < 4 || (anAxisCount < 2 && aHatCount == 0))
    {
        SDL_CloseJoystick(aJoystick);
        return false;
    }

    char aGuid[33];
    SDL_GUIDToString(SDL_GetJoystickGUID(aJoystick), aGuid, sizeof(aGuid));
    const char* aDeviceName = SDL_GetJoystickName(aJoystick);
    std::string aName = aDeviceName != nullptr ? aDeviceName : "HID Gamepad";
    std::replace(aName.begin(), aName.end(), ',', ' ');
    std::string aMapping = std::string(aGuid) + "," + aName + ",a:b0,b:b1,x:b2,y:b3,";
    if (anAxisCount >= 2)
        aMapping += "leftx:a0,lefty:a1,";
    if (anAxisCount >= 4)
        aMapping += "rightx:a2,righty:a3,";
    if (aButtonCount >= 6)
        aMapping += "leftshoulder:b4,rightshoulder:b5,";
    if (aButtonCount >= 10)
        aMapping += "lefttrigger:b6,righttrigger:b7,back:b8,start:b9,";
    else if (aButtonCount >= 8)
        aMapping += "back:b6,start:b7,";
    if (aButtonCount >= 12)
        aMapping += "leftstick:b10,rightstick:b11,";
    if (aHatCount > 0)
        aMapping += "dpup:h0.1,dpright:h0.2,dpdown:h0.4,dpleft:h0.8,";

    const bool aMapped = SDL_SetGamepadMapping(theInstanceId, aMapping.c_str());
    if (aMapped)
        logfi("HID gamepad fallback: %s guid=%s axes=%d buttons=%d hats=%d mapping=%s",
            aName.c_str(), aGuid, anAxisCount, aButtonCount, aHatCount, aMapping.c_str());
    else
        logfe("HID gamepad mapping failed: %s: %s", aName.c_str(), SDL_GetError());
    SDL_CloseJoystick(aJoystick);
    return aMapped;
}

float StickAxis(Sint16 theValue)
{
    if (theValue > -kStickDeadZone && theValue < kStickDeadZone)
        return 0.0f;
    return (float)theValue * kStickScale;
}

void LogGamepadState(SDL_Gamepad* theGamepad, int theDevice, const char* theReason)
{
    SDL_Joystick* aJoystick = SDL_GetGamepadJoystick(theGamepad);
    std::string anAxisValues;
    for (int anAxis = 0; anAxis < SDL_GetNumJoystickAxes(aJoystick); ++anAxis)
    {
        if (!anAxisValues.empty()) anAxisValues += ',';
        anAxisValues += std::to_string(SDL_GetJoystickAxis(aJoystick, anAxis));
    }
    std::string aHatValues;
    for (int aHat = 0; aHat < SDL_GetNumJoystickHats(aJoystick); ++aHat)
    {
        if (!aHatValues.empty()) aHatValues += ',';
        aHatValues += std::to_string(SDL_GetJoystickHat(aJoystick, aHat));
    }
    std::string aPressedButtons;
    for (int aButton = 0; aButton < SDL_GetNumJoystickButtons(aJoystick); ++aButton)
    {
        if (!SDL_GetJoystickButton(aJoystick, aButton)) continue;
        if (!aPressedButtons.empty()) aPressedButtons += ',';
        aPressedButtons += std::to_string(aButton);
    }
    char* aMapping = SDL_GetGamepadMapping(theGamepad);
    const char* aName = SDL_GetGamepadName(theGamepad);
    logfi("SDL gamepad state: device=%d name=%s reason=%s raw_axes=[%s] raw_hats=[%s] raw_buttons=[%s] mapping=%s",
        theDevice, aName != nullptr ? aName : "SDL Gamepad", theReason,
        anAxisValues.c_str(), aHatValues.c_str(), aPressedButtons.c_str(),
        aMapping != nullptr ? aMapping : "");
    SDL_free(aMapping);
}

} // namespace

// ---------------------------------------------------------------------------
// One SDL gamepad slot.
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
    Uint32 mButtons;
    SDL_Gamepad* mGamepad;
    SDL_JoystickID mInstanceId;
    float mLeftX;
    float mLeftY;
    float mRightX;
    float mRightY;
    bool  mTriggerLeft;
    bool  mTriggerRight;
    DirectionButtonState mDirections[kDirectionCount];
    bool mInitialStateLogged;
    std::string mName;
};

class SDLGamepadInterface : public IInputDriver
{
public:
    explicit SDLGamepadInterface(InputDriver *theOwner);
    virtual ~SDLGamepadInterface();

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
    void PollGamepads();
    void SyncGamepads();
    PolledPad* FindPad(int theDevice);
    bool mSubsystemInitialized = false;

    bool mEnabled;
    bool mGrabbed;
    std::vector<PolledPad> mPads;
    std::vector<Event> mEvents;

    float mRumbleLow;
    float mRumbleHigh;
    int   mRumbleDevice;
    Sexy::SexyAppBase *mApp;
};

SDLGamepadInterface::SDLGamepadInterface(InputDriver *theOwner)
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

SDLGamepadInterface::~SDLGamepadInterface()
{
    Cleanup();
}

int SDLGamepadInterface::Init()
{
    // The game also polls controllers from hidden windows and modal loops.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (const char* aMapping = Sexy::GetEnv(SDL_HINT_GAMECONTROLLERCONFIG))
        SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG, aMapping);
    const char* aMappingFile = Sexy::GetEnv(SDL_HINT_GAMECONTROLLERCONFIG_FILE);
    const std::string aFile = aMappingFile != nullptr ? aMappingFile : "gamecontrollerdb.txt";
    if (Sexy::SDLFile::FileExists(aFile))
        SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE, Sexy::SDLFile::ResolvePath(aFile).c_str());
    if (!mSubsystemInitialized) mSubsystemInitialized = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    if (!mSubsystemInitialized) return 0;
    SyncGamepads(); return 1;
}


void SDLGamepadInterface::Cleanup()
{
    for (auto& pad : mPads) SDL_CloseGamepad(pad.mGamepad);
    mPads.clear(); mEvents.clear();
    if (mSubsystemInitialized) SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    mSubsystemInitialized = false;
}


void SDLGamepadInterface::WillStop()
{
    Cleanup();
}

bool SDLGamepadInterface::HasEvent()
{
    return !mEvents.empty();
}

bool SDLGamepadInterface::GetEvent(Event &theEvent)
{
    if (mEvents.empty())
        return false;
    theEvent = mEvents.front();
    mEvents.erase(mEvents.begin());
    return true;
}

int SDLGamepadInterface::Connect()
{
    return 1;
}

int SDLGamepadInterface::Reconnect()
{
    Cleanup();
    return Init();
}

void SDLGamepadInterface::PushButton(int theDevice, KeyCode theKey, bool theDown, unsigned int theModifiers)
{
    Event anEvent = {};
    anEvent.modifiers = theDown ? 1 : 2;
    anEvent.modifierMask = theModifiers;
    anEvent.lateModifiers = mSlot;
    anEvent.eventType = (TMLongCard)theDevice;
    anEvent.eventCodeMask = (TMLongCard)theKey;
    mEvents.push_back(anEvent);
    const char* aAction = (theModifiers & 128) != 0 ? "repeat" : (theDown ? "down" : "up");
    logfi("Pad map %s: device %d -> pad 0x%X", aAction, theDevice, theKey);
}

void SDLGamepadInterface::UpdateDirection(PolledPad &thePad, int theDirection,
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

void SDLGamepadInterface::SyncGamepads()
{
    bool removed = false, added = false;
    unsigned int tick = static_cast<unsigned int>(SDL_GetTicks());
    for (auto iterator = mPads.begin(); iterator != mPads.end();)
    {
        if (SDL_GamepadConnected(iterator->mGamepad)) { ++iterator; continue; }
        for (int button = 0; button < kButtonCount; ++button)
            if (iterator->mButtons & (1u << button)) PushButton(iterator->mUserIndex, kButtons[button].mCode, false);
        if (iterator->mTriggerLeft) PushButton(iterator->mUserIndex, KEYCODE_GAMEPAD_TL2, false);
        if (iterator->mTriggerRight) PushButton(iterator->mUserIndex, KEYCODE_GAMEPAD_TR2, false);
        for (int direction = 0; direction < kDirectionCount; ++direction) UpdateDirection(*iterator, direction, false, tick);
        SDL_CloseGamepad(iterator->mGamepad); iterator = mPads.erase(iterator); removed = true;
    }
    int count = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&count);
    for (int index = 0; ids && index < count; ++index)
    {
        auto found = std::find_if(mPads.begin(), mPads.end(), [id = ids[index]](const PolledPad& pad) { return pad.mInstanceId == id; });
        if (found != mPads.end()) continue;
        // Keep SDL's known mappings (including Xbox) and only map unknown HID pads.
        if (!SDL_IsGamepad(ids[index]) && !MapUnrecognizedGamepad(ids[index])) continue;
        SDL_Gamepad* gamepad = SDL_OpenGamepad(ids[index]);
        if (!gamepad)
        {
            logfe("SDL_OpenGamepad failed: %s: %s", SDL_GetJoystickNameForID(ids[index]), SDL_GetError());
            continue;
        }
        PolledPad pad{};
        pad.mGamepad = gamepad; pad.mInstanceId = ids[index]; pad.mConnected = true;
        while (std::any_of(mPads.begin(), mPads.end(), [slot = pad.mUserIndex](const PolledPad& connected) {
            return connected.mUserIndex == slot;
        })) ++pad.mUserIndex;
        const char* name = SDL_GetGamepadName(gamepad); pad.mName = name ? name : "SDL Gamepad";
        logfi("SDL gamepad connected: device=%d instance=%u name=%s vendor=%04x product=%04x",
            pad.mUserIndex, pad.mInstanceId, pad.mName.c_str(),
            SDL_GetGamepadVendor(gamepad), SDL_GetGamepadProduct(gamepad));
        mPads.push_back(std::move(pad)); added = true;
    }
    SDL_free(ids);
    if (mApp && mApp->mInputManager)
    {
        if (removed) mApp->mInputManager->DispatchEvent(this, DEVICE_REMOVED, nullptr);
        if (added || (removed && !mPads.empty())) mApp->mInputManager->DispatchEvent(this, DEVICE_ADDED, nullptr);
    }
}
void SDLGamepadInterface::PollGamepads()
{
    if (!mEnabled || !mSubsystemInitialized) return;
    SDL_UpdateGamepads(); SyncGamepads();
    unsigned int tick = static_cast<unsigned int>(SDL_GetTicks());
    for (PolledPad& pad : mPads)
    {
        Uint32 buttons = 0;
        for (int button = 0; button < kButtonCount; ++button)
        {
            bool down = SDL_GetGamepadButton(pad.mGamepad, kButtons[button].mButton);
            if (down) buttons |= 1u << button;
            if (down != ((pad.mButtons & (1u << button)) != 0)) PushButton(pad.mUserIndex, kButtons[button].mCode, down);
        }
        bool leftTrigger = SDL_GetGamepadAxis(pad.mGamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 3855;
        bool rightTrigger = SDL_GetGamepadAxis(pad.mGamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 3855;
        if (leftTrigger != pad.mTriggerLeft) PushButton(pad.mUserIndex, KEYCODE_GAMEPAD_TL2, leftTrigger);
        if (rightTrigger != pad.mTriggerRight) PushButton(pad.mUserIndex, KEYCODE_GAMEPAD_TR2, rightTrigger);
        pad.mButtons = buttons; pad.mTriggerLeft = leftTrigger; pad.mTriggerRight = rightTrigger;
        pad.mLeftX = StickAxis(SDL_GetGamepadAxis(pad.mGamepad, SDL_GAMEPAD_AXIS_LEFTX));
        pad.mLeftY = StickAxis(SDL_GetGamepadAxis(pad.mGamepad, SDL_GAMEPAD_AXIS_LEFTY));
        pad.mRightX = StickAxis(SDL_GetGamepadAxis(pad.mGamepad, SDL_GAMEPAD_AXIS_RIGHTX));
        pad.mRightY = StickAxis(SDL_GetGamepadAxis(pad.mGamepad, SDL_GAMEPAD_AXIS_RIGHTY));
        // SDL reports downward-positive Y, matching the framework's axes.
        bool directions[] = {pad.mLeftX < -kDirectionDeadZone, pad.mLeftY < -kDirectionDeadZone,
            pad.mLeftX > kDirectionDeadZone, pad.mLeftY > kDirectionDeadZone,
            SDL_GetGamepadButton(pad.mGamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT), SDL_GetGamepadButton(pad.mGamepad, SDL_GAMEPAD_BUTTON_DPAD_UP),
            SDL_GetGamepadButton(pad.mGamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT), SDL_GetGamepadButton(pad.mGamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)};
        bool aDpadChanged = false;
        for (int aDirection = 4; aDirection < kDirectionCount; ++aDirection)
            aDpadChanged |= directions[aDirection] != pad.mDirections[aDirection].mDown;
        if (!pad.mInitialStateLogged)
        {
            LogGamepadState(pad.mGamepad, pad.mUserIndex, pad.mInitialStateLogged ? "dpad changed" : "initial");
            pad.mInitialStateLogged = true;
        }
        for (int direction = 0; direction < kDirectionCount; ++direction) UpdateDirection(pad, direction, directions[direction], tick);
        if (pad.mUserIndex == mRumbleDevice)
            SDL_RumbleGamepad(pad.mGamepad, static_cast<Uint16>(std::clamp(mRumbleLow, 0.0f, 1.0f) * 65535),
                static_cast<Uint16>(std::clamp(mRumbleHigh, 0.0f, 1.0f) * 65535), 100);
    }
}


int SDLGamepadInterface::Update()
{
    PollGamepads();

    if (mOwner == nullptr)
        return (int)mEvents.size();

    // The events are delivered by the manager's own queue. The owner does not
    // hold the manager, so the caller (InputManager::Update) is expected to
    // drain GetEvent. Returning the pending count keeps the manager's result.
    return (int)mEvents.size();
}

int SDLGamepadInterface::GetNumDevices()
{
    int aCount = 0;
    for (const PolledPad& aPad : mPads)
        aCount = std::max(aCount, aPad.mUserIndex + 1);
    return aCount;
}

PolledPad* SDLGamepadInterface::FindPad(int theDevice)
{
    auto aPad = std::find_if(mPads.begin(), mPads.end(), [theDevice](const PolledPad& pad) {
        return pad.mUserIndex == theDevice;
    });
    return aPad != mPads.end() ? &*aPad : nullptr;
}

bool SDLGamepadInterface::GetInfo(InputInfo &theInfo, int theDevice)
{
    const PolledPad* aPad = FindPad(theDevice);
    if (aPad == nullptr || !aPad->mConnected)
        return false;

    theInfo.mName = aPad->mName.c_str();
    theInfo.mHasPointer = 0;
    theInfo.mHasKeys = 1;
    theInfo.mHasTouch = 0;
    theInfo.mHasMultiTouch = 0;
    theInfo.mHasJoystick = 1;
    theInfo.mDeviceIndex = theDevice;
    return true;
}

bool SDLGamepadInterface::IsGrabbed()
{
    return mGrabbed;
}

void SDLGamepadInterface::Grab(bool theGrab)
{
    mGrabbed = theGrab;
}

bool SDLGamepadInterface::GetProperty(const String &, void *)
{
    return false;
}

bool SDLGamepadInterface::IsEnabled()
{
    return mEnabled;
}

void SDLGamepadInterface::Enable(bool theEnable)
{
    mEnabled = theEnable;
}

int SDLGamepadInterface::GetNumAxes(int)
{
    return 4;
}

bool SDLGamepadInterface::GetAxisInfo(Axis theAxis, InputAxisInfo &theInfo, int theDevice)
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

bool SDLGamepadInterface::GetAxisInfo(std::vector<InputAxisInfo> &theInfos, int theDevice)
{
    theInfos.clear();
    const PolledPad* aDevicePad = FindPad(theDevice);
    if (aDevicePad == nullptr || !aDevicePad->mConnected)
        return false;

    const PolledPad &aPad = *aDevicePad;
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

void SDLGamepadInterface::ApplyRumble()
{
}

// ---------------------------------------------------------------------------
// The factory entry. Init builds the interface the manager then owns.
// ---------------------------------------------------------------------------

SDLGamepadDriver::SDLGamepadDriver()
    : InputDriver(Sexy::String("SDLGamepad"), 0)
    , mInterface(nullptr)
{
}

SDLGamepadDriver::~SDLGamepadDriver()
{
    mInterface = nullptr;
}

IInputDriver *SDLGamepadDriver::Init()
{
    SDLGamepadInterface *anInterface = new SDLGamepadInterface(this);
    if (anInterface->Init() == 0)
    {
        delete anInterface;
        return nullptr;
    }
    mInterface = anInterface;
    return anInterface;
}

int SDLGamepadDriver::OnStart(Sexy::SexyAppBase *theApp, Sexy::InputManager *)
{
    if (mInterface != nullptr)
        mInterface->mApp = theApp;
    return 0;
}

int SDLGamepadDriver::OnStop()
{
    mInterface = nullptr;
    return InputDriver::OnStop();
}

IInputDriver *CreateSDLGamepad(InputDriver *theOwner)
{
    SDLGamepadInterface *anInterface = new SDLGamepadInterface(theOwner);
    return anInterface;
}

} // namespace Sexy

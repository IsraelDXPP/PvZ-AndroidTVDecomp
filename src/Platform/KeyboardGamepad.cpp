#include "KeyboardGamepad.h"

#include "ButtonGlyphs.h"
#include "GLWindow.h"
#include "InputDriver.h"
#include "InputManager.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"
#include "ReconTypes.h"
#include "SexyAppBase.h"

#include <GLFW/glfw3.h>

#include <cstring>
#include <vector>

namespace Sexy
{

const int kKeyboardPadA = 0x130;
const int kKeyboardPadB = 0x131;
const int kKeyboardPadX = 0x133;
const int kKeyboardPadY = 0x134;
const int kKeyboardPadTL = 0x136;
const int kKeyboardPadTR = 0x137;
const int kKeyboardPadTL2 = 0x138;
const int kKeyboardPadTR2 = 0x139;
const int kKeyboardPadStart = 0x13B;
const int kKeyboardPadLeft = 0x230;
const int kKeyboardPadUp = 0x231;
const int kKeyboardPadRight = 0x232;
const int kKeyboardPadDown = 0x233;

struct KeyboardBinding
{
    const char *mButton;
    int mPadCode;
    int mVirtualKey;
    bool mDown;
};

class KeyboardGamepadInterface : public IInputDriver
{
public:
    explicit KeyboardGamepadInterface(InputDriver *theOwner);
    virtual ~KeyboardGamepadInterface();

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

    void PushButton(int theKey, bool theDown);
    int GlfwKeyFromName(const char *theName);

    bool mEnabled;
    bool mGrabbed;
    bool mAnnounced;
    std::vector<Event> mEvents;
    std::vector<KeyboardBinding> mBindings;
    Sexy::SexyAppBase *mApp;
};

KeyboardGamepadInterface::KeyboardGamepadInterface(InputDriver *theOwner)
    : mEnabled(true)
    , mGrabbed(false)
    , mAnnounced(false)
    , mApp(nullptr)
{
    mSlot = 0;
    mPad0C = 0;
    mOwner = theOwner;
}

KeyboardGamepadInterface::~KeyboardGamepadInterface()
{
    Cleanup();
}

int KeyboardGamepadInterface::GlfwKeyFromName(const char *aName)
{
    if (aName == nullptr || aName[0] == '\0')
        return 0;
    if (std::strcmp(aName, "RETURN") == 0 || std::strcmp(aName, "ENTER") == 0)
        return GLFW_KEY_ENTER;
    if (std::strcmp(aName, "ESCAPE") == 0 || std::strcmp(aName, "ESC") == 0)
        return GLFW_KEY_ESCAPE;
    if (std::strcmp(aName, "SPACE") == 0)
        return GLFW_KEY_SPACE;
    if (std::strcmp(aName, "LEFT") == 0)
        return GLFW_KEY_LEFT;
    if (std::strcmp(aName, "RIGHT") == 0)
        return GLFW_KEY_RIGHT;
    if (std::strcmp(aName, "UP") == 0)
        return GLFW_KEY_UP;
    if (std::strcmp(aName, "DOWN") == 0)
        return GLFW_KEY_DOWN;
    if (aName[0] != '\0' && aName[1] == '\0')
    {
        char aLetter = aName[0];
        if (aLetter >= 'a' && aLetter <= 'z')
            aLetter = (char)(aLetter - 'a' + 'A');
        return GLFW_KEY_A + (aLetter - 'A');
    }
    return 0;
}

int KeyboardGamepadInterface::Init()
{
    mBindings.clear();
    const KeyboardBinding aDefaults[] = {
        { "<A>", kKeyboardPadA, 0, false },
        { "<B>", kKeyboardPadB, 0, false },
        { "<X>", kKeyboardPadX, 0, false },
        { "<Y>", kKeyboardPadY, 0, false },
        { "<L>", kKeyboardPadTL, 0, false },
        { "<R>", kKeyboardPadTR, 0, false },
        { "<LT>", kKeyboardPadTL2, 0, false },
        { "<RT>", kKeyboardPadTR2, 0, false },
        { "<S>", kKeyboardPadStart, 0, false },
        { "<LEFT>", kKeyboardPadLeft, GLFW_KEY_LEFT, false },
        { "<RIGHT>", kKeyboardPadRight, GLFW_KEY_RIGHT, false },
        { "<UP>", kKeyboardPadUp, GLFW_KEY_UP, false },
        { "<DOWN>", kKeyboardPadDown, GLFW_KEY_DOWN, false },
    };
    for (int aIndex = 0; aIndex < (int)(sizeof(aDefaults) / sizeof(aDefaults[0])); ++aIndex)
    {
        const char *aKeys[8];
        int aCount = ButtonKeyCaptions(aDefaults[aIndex].mButton,
                                       (int)std::strlen(aDefaults[aIndex].mButton), aKeys, 8);
        if (aCount == 0)
        {
            if (aDefaults[aIndex].mVirtualKey != 0)
                mBindings.push_back(aDefaults[aIndex]);
            continue;
        }
        for (int aKey = 0; aKey < aCount; ++aKey)
        {
            KeyboardBinding aBinding = aDefaults[aIndex];
            aBinding.mVirtualKey = GlfwKeyFromName(aKeys[aKey]);
            if (aBinding.mVirtualKey == 0)
                continue;
            mBindings.push_back(aBinding);
        }
    }
    return 1;
}

void KeyboardGamepadInterface::Cleanup()
{
    mEvents.clear();
    mBindings.clear();
    mAnnounced = false;
}

void KeyboardGamepadInterface::WillStop()
{
    Cleanup();
}

bool KeyboardGamepadInterface::HasEvent()
{
    return !mEvents.empty();
}

bool KeyboardGamepadInterface::GetEvent(Event &theEvent)
{
    if (mEvents.empty())
        return false;
    theEvent = mEvents.front();
    mEvents.erase(mEvents.begin());
    return true;
}

int KeyboardGamepadInterface::Connect()
{
    return 1;
}

int KeyboardGamepadInterface::Reconnect()
{
    Cleanup();
    return Init();
}

void KeyboardGamepadInterface::PushButton(int theKey, bool theDown)
{
    Event anEvent = {};
    anEvent.modifiers = theDown ? 1 : 2;
    anEvent.lateModifiers = mSlot;
    anEvent.eventType = (TMLongCard)0;
    anEvent.eventCodeMask = (TMLongCard)theKey;
    mEvents.push_back(anEvent);
}

int KeyboardGamepadInterface::Update()
{
    if (!mEnabled)
        return 0;

    // A live XInput pad owns the buttons. This driver only
    // speaks while that driver has nothing connected.
    if (mApp != nullptr && mApp->mInputManager != nullptr)
    {
        for (IInputDriver *aDriver : mApp->mInputManager->Drivers())
        {
            if (aDriver == nullptr || aDriver == this || aDriver->mOwner == nullptr)
                continue;
            if (aDriver->mOwner->mName == "Win32Gamepad" && aDriver->GetNumDevices() > 0)
                return 0;
        }
    }

    if (!mAnnounced && mOwner != nullptr && mApp != nullptr && mApp->mInputManager != nullptr)
    {
        mAnnounced = true;
        mApp->mInputManager->DispatchEvent(this, DEVICE_ADDED, nullptr);
    }

    int aGlfwKey = 0;
    bool aDown = false;
    while (GLWindow::PopKey(aGlfwKey, aDown))
    {
        for (unsigned int i = 0; i < mBindings.size(); ++i)
        {
            KeyboardBinding &aBinding = mBindings[i];
            if (aBinding.mVirtualKey != aGlfwKey || aBinding.mDown == aDown)
                continue;
            aBinding.mDown = aDown;
            PushButton(aBinding.mPadCode, aDown);
        }
    }
    return (int)mEvents.size();
}

int KeyboardGamepadInterface::GetNumDevices()
{
    return mAnnounced ? 1 : 0;
}

bool KeyboardGamepadInterface::GetInfo(InputInfo &theInfo, int theDevice)
{
    if (!mAnnounced || theDevice != 0)
        return false;
    theInfo.mName = "Keyboard";
    theInfo.mHasPointer = 0;
    theInfo.mHasKeys = 1;
    theInfo.mHasTouch = 0;
    theInfo.mHasMultiTouch = 0;
    theInfo.mHasJoystick = 1;
    theInfo.mDeviceIndex = 0;
    return true;
}

bool KeyboardGamepadInterface::IsGrabbed()
{
    return mGrabbed;
}

void KeyboardGamepadInterface::Grab(bool theGrab)
{
    mGrabbed = theGrab;
}

bool KeyboardGamepadInterface::GetProperty(const String &, void *)
{
    return false;
}

bool KeyboardGamepadInterface::IsEnabled()
{
    return mEnabled;
}

void KeyboardGamepadInterface::Enable(bool theEnable)
{
    mEnabled = theEnable;
}

int KeyboardGamepadInterface::GetNumAxes(int)
{
    return 0;
}

bool KeyboardGamepadInterface::GetAxisInfo(Axis, InputAxisInfo &, int)
{
    return false;
}

bool KeyboardGamepadInterface::GetAxisInfo(std::vector<InputAxisInfo> &theInfos, int)
{
    theInfos.clear();
    return false;
}

KeyboardGamepadDriver::KeyboardGamepadDriver()
    : InputDriver(Sexy::String("KeyboardGamepad"), 0)
    , mInterface(nullptr)
{
}

KeyboardGamepadDriver::~KeyboardGamepadDriver()
{
    mInterface = nullptr;
}

IInputDriver *KeyboardGamepadDriver::Init()
{
    KeyboardGamepadInterface *anInterface = new KeyboardGamepadInterface(this);
    if (anInterface->Init() == 0)
    {
        delete anInterface;
        return nullptr;
    }
    mInterface = anInterface;
    return anInterface;
}

int KeyboardGamepadDriver::OnStart(Sexy::SexyAppBase *theApp, Sexy::InputManager *)
{
    if (mInterface != nullptr)
        static_cast<KeyboardGamepadInterface *>(mInterface)->mApp = theApp;
    return 0;
}

int KeyboardGamepadDriver::OnStop()
{
    mInterface = nullptr;
    return InputDriver::OnStop();
}

} // namespace Sexy

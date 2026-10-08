// InputManager starts the registered input drivers, queues their events, and
// tells listeners when a device appears, leaves, or produces an event.

#include "InputManager.h"

#include "DriverFactory.h"
#include "InputDriver.h"
#include "InputDriverFactory.h"
#include "NativeDisplay.h"
#include "Platform/SDLWindow.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"


namespace {

const unsigned int kRelativeMouse = 2u;
const unsigned int kAbsoluteMouse = 1u;
const unsigned int kScaleMouse = 0x20u;
const unsigned int kHasTimestamp = 0x40u;

bool IsPointerEvent(int theKind)
{
    return theKind == 7 || theKind == 3 || theKind == 4;
}

void ClearInputInfo(Sexy::InputInfo& theInfo)
{
    theInfo.mName = "Unknown";
    theInfo.mHasPointer = 0;
    theInfo.mHasKeys = 0;
    theInfo.mHasTouch = 0;
    theInfo.mHasMultiTouch = 0;
    theInfo.mHasJoystick = 0;
    theInfo.mDeviceIndex = -1;
}

void CountDevice(Sexy::InputStatusInfo& theStatus, const Sexy::InputInfo& theInfo)
{
    if (theInfo.mHasPointer != 0)
        ++theStatus.mPointerCount;
    if (theInfo.mHasKeys != 0)
        ++theStatus.mKeyCount;
    if (theInfo.mHasTouch != 0 || theInfo.mHasMultiTouch != 0)
        ++theStatus.mTouchCount;
    if (theInfo.mHasJoystick != 0)
        ++theStatus.mJoystickCount;
}

const char* YesOrNo(int theFlag)
{
    return theFlag != 0 ? "yes" : "no";
}

void DumpOneDevice(int theSlot, const Sexy::InputInfo& theInfo)
{
    logfi("  0x%08x - '%s':", theSlot, theInfo.mName.c_str());
    logfi("\t\tkeys: %s pointer: %s joystick: %s.",
          YesOrNo(theInfo.mHasKeys),
          YesOrNo(theInfo.mHasPointer),
          YesOrNo(theInfo.mHasJoystick));
}

void StampEvent(Sexy::Event& theEvent)
{
    if (((unsigned int)theEvent.modifierMask & kHasTimestamp) != 0)
        return;
    theEvent.eventCode = static_cast<int>(SDL_GetTicks());
    theEvent.modifierMask = (int)(((unsigned int)theEvent.modifierMask) | kHasTimestamp);
}

} // namespace

Sexy::InputManager::InputManager(SexyAppBase* theApp, unsigned int theMaxEvents)
    : mApp(theApp),
      mInitialized(false),
      mMaxEventCount(theMaxEvents),
      mMouseX(0),
      mMouseY(0),
      mNextDriverSlot(0),
      mChangeCookie(0)
{
}

int Sexy::InputManager::IsInitialized()
{
    return mInitialized ? 1 : 0;
}

int Sexy::InputManager::Cleanup()
{
    Sexy::DriverFactory* aFactory = Sexy::InputDriverFactory::GetInputDriverFactory();
    if (aFactory != nullptr)
    {
        std::vector<Sexy::Driver*> aOwners = aFactory->Drivers();
        for (Sexy::Driver* aOwner : aOwners)
        {
            if (aOwner != nullptr && aOwner->IsDisabled() == 0)
                static_cast<Sexy::InputDriver*>(aOwner)->OnStop();
        }
    }

    std::vector<IInputDriver*> aOwned(mDrivers.begin(), mDrivers.end());
    for (IInputDriver* aDriver : aOwned)
    {
        if (aDriver != nullptr)
            aDriver->WillStop();
    }

    mEventLock.Lock();
    mDrivers.clear();
    for (IInputDriver* aDriver : aOwned)
        delete aDriver;
    mEvents.clear();
    mMouseX = 0;
    mMouseY = 0;
    mInitialized = false;
    return mEventLock.Unlock();
}

bool Sexy::InputManager::HasEvent()
{
    mEventLock.Lock();
    bool aHasEvent = !mEvents.empty();
    mEventLock.Unlock();
    return aHasEvent;
}

int Sexy::InputManager::PreprocessEvent(Event& theEvent)
{
    if (!IsPointerEvent(theEvent.modifiers))
        return 0;
    if (mApp == nullptr)
        return 0;

    int aLeft = 0;
    int aTop = 0;
    int aWidth = mApp->mWidth;
    int aHeight = mApp->mHeight;
    // SDL/bgfx has no DDInterface. Mouse events still enter in render-surface
    // coordinates; RemapMouse applies the stage's centering offset afterward.
    // Using the 800x600 stage here scales and clamps a 1280x720 window twice.
    if (mApp->mWidgetManager != nullptr
        && mApp->mWidgetManager->mMouseSourceRect.mWidth > 0
        && mApp->mWidgetManager->mMouseSourceRect.mHeight > 0)
    {
        Rect& aSource = mApp->mWidgetManager->mMouseSourceRect;
        aLeft = aSource.mX;
        aTop = aSource.mY;
        aWidth = aSource.mWidth;
        aHeight = aSource.mHeight;
    }
    else
    {
        aLeft = -mApp->mScreenBounds.mX;
        aTop = -mApp->mScreenBounds.mY;
    }

    unsigned int aMask = (unsigned int)theEvent.modifierMask;
    int aX = (int)theEvent.eventCodeMask;
    int aY = theEvent.matchEvent;
    theEvent.mSavedModifierMask = (int)aMask;
    theEvent.mSavedEventCodeMask = aX;
    theEvent.mSavedMatchEvent = aY;

    if ((aMask & kRelativeMouse) != 0)
    {
        aX += mMouseX;
        aY += mMouseY;
        mMouseX = aX;
        mMouseY = aY;
    }
    else if ((aMask & kAbsoluteMouse) != 0)
    {
        mMouseX = aX;
        mMouseY = aY;
        if ((aMask & kScaleMouse) != 0 &&
            (int)theEvent.mScaleDivisorX != 0 &&
            (int)theEvent.mScaleDivisorY != 0)
        {
            aX = (aWidth * aX) / (int)theEvent.mScaleDivisorX;
            aY = (aHeight * aY) / (int)theEvent.mScaleDivisorY;
            mMouseX = aX;
            mMouseY = aY;
        }
    }
    else
    {
        aX = mMouseX;
        aY = mMouseY;
    }

    if (aLeft > aX)
    {
        aX = aLeft;
        mMouseX = aX;
    }
    if (aTop > aY)
    {
        aY = aTop;
        mMouseY = aY;
    }
    int aRight = aLeft + aWidth;
    int aBottom = aTop + aHeight;
    if (aRight <= aX)
    {
        aX = aRight - 1;
        mMouseX = aX;
    }
    if (aBottom <= aY)
    {
        aY = aBottom - 1;
        mMouseY = aY;
    }

    theEvent.eventCodeMask = aX;
    theEvent.matchEvent = aY;
    theEvent.modifierMask = (int)((aMask & ~kRelativeMouse) | kAbsoluteMouse);
    return 0;
}

int Sexy::InputManager::PopEvent(Event& theEvent)
{
    mEventLock.Lock();
    theEvent.modifiers = 0;
    int aGotOne = 0;
    if (!mEvents.empty())
    {
        aGotOne = 1;
        theEvent = mEvents.front();
        mEvents.pop_front();
    }
    mEventLock.Unlock();
    return aGotOne;
}

Sexy::InputManager* Sexy::InputManager::ConnectAll()
{
    for (IInputDriver* aDriver : mDrivers)
    {
        if (aDriver != nullptr)
            aDriver->Connect();
    }
    return this;
}

Sexy::InputManager* Sexy::InputManager::ReconnectAll()
{
    for (IInputDriver* aDriver : mDrivers)
    {
        if (aDriver != nullptr)
            aDriver->Reconnect();
    }
    return this;
}

Sexy::IInputDriver* Sexy::InputManager::Find(int theSlot)
{
    for (IInputDriver* aDriver : mDrivers)
    {
        if (aDriver != nullptr && aDriver->mSlot == theSlot)
            return aDriver;
    }
    return nullptr;
}

int Sexy::InputManager::Connect(int theSlot)
{
    IInputDriver* aDriver = Find(theSlot);
    if (aDriver == nullptr)
        return 0;
    return aDriver->Connect();
}

int Sexy::InputManager::Reconnect(int theSlot)
{
    IInputDriver* aDriver = Find(theSlot);
    if (aDriver == nullptr)
        return 0;
    return aDriver->Reconnect();
}

int Sexy::InputManager::GetCookie()
{
    return mChangeCookie;
}

void Sexy::InputManager::DumpStatus()
{
    logfi("Available input devices:\n");
    InputInfo aInfo;
    ClearInputInfo(aInfo);

    NativeDisplay* aDisplay = mApp != nullptr ? mApp->mPlatformDriverOrQueue : nullptr;
    if (aDisplay != nullptr && aDisplay->GetInputInfo(aInfo) != 0)
        DumpOneDevice(0, aInfo);

    for (IInputDriver* aDriver : mDrivers)
    {
        if (aDriver == nullptr)
            continue;
        int aCount = aDriver->GetNumDevices();
        for (int aIndex = 0; aIndex < aCount; ++aIndex)
        {
            ClearInputInfo(aInfo);
            if (aDriver->GetInfo(aInfo, aIndex))
                DumpOneDevice(aDriver->mSlot, aInfo);
        }
    }
}

int Sexy::InputManager::GetStatus(InputStatusInfo& theStatus)
{
    mEventLock.Lock();
    theStatus.mDriverCount = (int)mDrivers.size();
    theStatus.mDeviceCount = 0;
    theStatus.mPointerCount = 0;
    theStatus.mKeyCount = 0;
    theStatus.mTouchCount = 0;
    theStatus.mJoystickCount = 0;

    InputInfo aInfo;
    ClearInputInfo(aInfo);
    NativeDisplay* aDisplay = mApp != nullptr ? mApp->mPlatformDriverOrQueue : nullptr;
    if (aDisplay != nullptr && aDisplay->GetInputInfo(aInfo) != 0)
        CountDevice(theStatus, aInfo);

    for (IInputDriver* aDriver : mDrivers)
    {
        if (aDriver == nullptr)
            continue;
        int aCount = aDriver->GetNumDevices();
        theStatus.mDeviceCount += aCount;
        for (int aIndex = 0; aIndex < aCount; ++aIndex)
        {
            ClearInputInfo(aInfo);
            if (aDriver->GetInfo(aInfo, aIndex))
                CountDevice(theStatus, aInfo);
        }
    }
    return mEventLock.Unlock();
}

int Sexy::InputManager::Changed()
{
    mEventLock.Lock();
    ++mChangeCookie;
    return mEventLock.Unlock();
}

int Sexy::InputManager::GetMousePos(int& theX, int& theY)
{
    theX = mMouseX;
    theY = mMouseY;
    return 0;
}

int Sexy::InputManager::AddListener(InputDeviceListener* theListener)
{
    mDriverLock.Lock();
    mListeners.push_back(theListener);
    return mDriverLock.Unlock();
}

int Sexy::InputManager::RemoveListener(InputDeviceListener* theListener)
{
    mDriverLock.Lock();
    for (std::list<InputDeviceListener*>::iterator aFound = mListeners.begin();
         aFound != mListeners.end();
         ++aFound)
    {
        if (*aFound == theListener)
        {
            mListeners.erase(aFound);
            break;
        }
    }
    return mDriverLock.Unlock();
}

int Sexy::InputManager::DispatchEvent(IInputDriver* theDriver, DeviceEvent theKind, Event* theEvent)
{
    mDriverLock.Lock();
    for (InputDeviceListener* aListener : mListeners)
    {
        if (aListener == nullptr)
            continue;
        if (theKind == DEVICE_REMOVED)
            aListener->DeviceRemoved(theDriver);
        else if (theKind == DEVICE_ADDED)
            aListener->DeviceAdded(theDriver);
        else if (theKind == DEVICE_GENERATED)
            aListener->DeviceEventGenerated(theDriver, theEvent);
    }
    return mDriverLock.Unlock();
}

int Sexy::InputManager::Remove(IInputDriver* theDriver)
{
    if (theDriver == nullptr)
        return 0;

    DispatchEvent(theDriver, DEVICE_REMOVED, nullptr);

    mEventLock.Lock();
    bool aFound = false;
    for (std::list<IInputDriver*>::iterator aNode = mDrivers.begin();
         aNode != mDrivers.end();
         ++aNode)
    {
        if (*aNode == theDriver)
        {
            mDrivers.erase(aNode);
            aFound = true;
            break;
        }
    }
    mEventLock.Unlock();
    if (!aFound)
        return 0;

    delete theDriver;
    ++mChangeCookie;
    return 1;
}

int Sexy::InputManager::PushEvent(Event& theEvent, IInputDriver* theDriver)
{
    if (theDriver == nullptr && theEvent.lateModifiers > 0)
        theDriver = Find(theEvent.lateModifiers);

    mEventLock.Lock();
    if (theEvent.modifiers != 0)
    {
        unsigned int aQueued = (unsigned int)mEvents.size();
        if (aQueued == 0 || mMaxEventCount >= aQueued)
        {
            PreprocessEvent(theEvent);
            StampEvent(theEvent);
            mEventLock.Unlock();
            DispatchEvent(theDriver, DEVICE_GENERATED, &theEvent);
            mEventLock.Lock();
            mEvents.push_back(theEvent);
        }
    }
    return mEventLock.Unlock();
}

int Sexy::InputManager::Update()
{
    int aResult = 0;
    Event aEvent;
    aEvent.eventCode = -1;

    NativeDisplay* aDisplay = mApp != nullptr ? mApp->mPlatformDriverOrQueue : nullptr;
    while (aDisplay != nullptr)
    {
        aResult = aDisplay->GetEvent(aEvent) ? 1 : 0;
        if (aResult == 0)
            break;
        aEvent.lateModifiers = 0;
        PushEvent(aEvent, nullptr);
        aDisplay = mApp->mPlatformDriverOrQueue;
    }

    // The SDL window supplies keyboard and mouse events. AndroidDisplay::GetEvent is a stub;
    // the real keys arrived through AndroidInput::HandleKeyEvents. The port
    // posts the same shape here, with device 0 so a key is not a pad.
    SDLWindow* aWindow = ActiveWindow();
    while (aWindow != nullptr && aWindow->PopInputEvent(aEvent))
    {
        aEvent.lateModifiers = 0;
        PushEvent(aEvent, nullptr);
        aResult = 1;
    }

    for (IInputDriver* aDriver : mDrivers)
    {
        if (aDriver == nullptr)
            continue;
        aResult = aDriver->Update();
        Event aDriverEvent;
        while (aDriver->GetEvent(aDriverEvent))
            PushEvent(aDriverEvent, aDriver);
    }
    return aResult;
}

Sexy::InputManager* Sexy::InputManager::PushEvents(std::list<Event>& theEvents, IInputDriver* theDriver)
{
    if (theEvents.empty())
        return this;

    if (theDriver == nullptr && theEvents.front().lateModifiers > 0)
        theDriver = Find(theEvents.front().lateModifiers);

    mEventLock.Lock();
    unsigned int aQueued = (unsigned int)mEvents.size();
    unsigned int aIncoming = (unsigned int)theEvents.size();
    if (aQueued + aIncoming < mMaxEventCount)
    {
        for (Event& anEvent : theEvents)
        {
            if (anEvent.modifiers == 0)
                continue;
            PreprocessEvent(anEvent);
            StampEvent(anEvent);
            mEventLock.Unlock();
            DispatchEvent(theDriver, DEVICE_GENERATED, &anEvent);
            mEventLock.Lock();
            mEvents.push_back(anEvent);
        }
    }
    mEventLock.Unlock();
    return this;
}

int Sexy::InputManager::Add(IInputDriver* theDriver, InputDriver* theOwner, bool theConnectNow)
{
    if (theDriver == nullptr)
        return 0;

    mEventLock.Lock();
    theDriver->mSlot = mNextDriverSlot + 1;
    theDriver->mOwner = theOwner;
    if (theDriver->Init() == 0)
    {
        mEventLock.Unlock();
        return 0;
    }

    ++mNextDriverSlot;
    mDrivers.push_back(theDriver);
    if (theConnectNow)
        theDriver->Connect();
    ++mChangeCookie;
    mEventLock.Unlock();
    DispatchEvent(theDriver, DEVICE_ADDED, nullptr);
    return 1;
}

int Sexy::InputManager::Init()
{
    Cleanup();

    Sexy::DriverFactory* aFactory = Sexy::InputDriverFactory::GetInputDriverFactory();
    std::vector<Sexy::Driver*> aOwners;
    if (aFactory != nullptr)
        aOwners = aFactory->Drivers();

    for (Sexy::Driver* aOwner : aOwners)
    {
        if (aOwner == nullptr || aOwner->IsDisabled() != 0)
            continue;
        Sexy::InputDriver* aInput = static_cast<Sexy::InputDriver*>(aOwner);
        IInputDriver* aBuilt = aInput->Init();
        if (aBuilt != nullptr && Add(aBuilt, aInput, false) == 0)
            delete aBuilt;
    }

    ConnectAll();

    for (Sexy::Driver* aOwner : aOwners)
    {
        if (aOwner == nullptr || aOwner->IsDisabled() != 0)
            continue;
        static_cast<Sexy::InputDriver*>(aOwner)->OnStart(mApp, this);
    }

    mInitialized = true;
    return 0;
}

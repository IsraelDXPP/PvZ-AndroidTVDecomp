// InputManager owns the connected input drivers, the queued events, and the
// listeners that hear devices come and go. The app asks it to start the
// registered drivers, to pull events each frame, and to report the mouse.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_INPUTMANAGER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_INPUTMANAGER_H

#include "CritSect.h"
#include "IInputDriver.h"
#include "InputDeviceListener.h"
#include "ReconTypes.h"

#include <list>
#include <vector>

namespace Sexy
{

class SexyAppBase;
class InputDriver;

class InputStatusInfo
{
public:
    int mPointerCount;
    int mKeyCount;
    int mJoystickCount;
    int mTouchCount;
    int mDeviceCount;
    int mDriverCount;

    InputStatusInfo() :
        mPointerCount(0),
        mKeyCount(0),
        mJoystickCount(0),
        mTouchCount(0),
        mDeviceCount(0),
        mDriverCount(0)
    {
    }
};

enum DeviceEvent
{
    DEVICE_ADDED = 0,
    DEVICE_REMOVED = 1,
    DEVICE_GENERATED = 2
};

class InputManager
{
public:
    InputManager(SexyAppBase *theApp, unsigned int theMaxEvents);

    int IsInitialized();
    int Cleanup();
    bool HasEvent();
    int PreprocessEvent(Event &theEvent);
    int PopEvent(Event &theEvent);
    InputManager *ConnectAll();
    InputManager *ReconnectAll();
    IInputDriver *Find(int theSlot);
    int Connect(int theSlot);
    int Reconnect(int theSlot);
    int GetCookie();
    void DumpStatus();
    int GetStatus(InputStatusInfo &theStatus);
    int Changed();
    int GetMousePos(int &theX, int &theY);
    int AddListener(InputDeviceListener *theListener);
    int RemoveListener(InputDeviceListener *theListener);
    int DispatchEvent(IInputDriver *theDriver, DeviceEvent theKind, Event *theEvent);
    int Remove(IInputDriver *theDriver);
    int PushEvent(Event &theEvent, IInputDriver *theDriver);
    int Update();
    InputManager *PushEvents(std::list<Event> &theEvents, IInputDriver *theDriver);
    int Add(IInputDriver *theDriver, InputDriver *theOwner, bool theConnectNow);
    int Init();
    // Drivers currently registered. GamepadApp asks each one again on the
    // first frame, because the startup DeviceAdded ran before its queue existed.
    const std::list<IInputDriver *> &Drivers() const
    {
        return mDrivers;
    }

    SexyAppBase *mApp;
    bool mInitialized;
    unsigned int mMaxEventCount;
    int mMouseX;
    int mMouseY;
    int mNextDriverSlot;
    int mChangeCookie;

private:
    std::list<IInputDriver *> mDrivers;
    std::list<Event> mEvents;
    std::list<InputDeviceListener *> mListeners;
    CritSect mDriverLock;
    CritSect mEventLock;
};

} // namespace Sexy

using Sexy::InputManager;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_INPUTMANAGER_H

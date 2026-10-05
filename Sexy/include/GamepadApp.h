// GamepadApp is the application object that owns gamepads and the atlas cache.
// It extends SexyCommonApp and also listens for input devices, so a device
// that arrives or leaves is recorded and drained on the next frame.
//
// The listener is a second base. The constructor registers that sub-object
// with InputManager, and DeviceAdded / DeviceRemoved on this class replace
// the listener's empty defaults. GamepadConnected and GamepadDisconnected
// are this class's own additions.
//
// After the two bases: four gamepad slots, the keyboard-simulation flag, the
// first-pad flag, the device-event lock, the atlas parser, the atlas lock,
// the pending device-event list, the atlas image cache, and the two flags
// CheckGamepad and the second pad read.
//
// Member order and method signatures match the previous declaration. The
// pending-event list and the atlas cache are standard containers.

#pragma once

#include "ReconTypes.h"
#include "AtlasParser.h"
#include "CritSect.h"
#include "Gamepad.h"
#include "InputDeviceListener.h"
#include "InputManager.h"
#include "SexyAppBase.h"
#include "SexyCommonApp.h"
#include "SharedImageRef.h"
#include <list>
#include <map>
#include <string>

namespace Sexy {

class IInputDriver;

class GamepadApp : public SexyCommonApp, public Sexy::InputDeviceListener
{
public:
    virtual ~GamepadApp();

    virtual void UpdateFrames();
    virtual Sexy::Image *GetImage(Sexy::String const& theName, bool thePurge);
    virtual Sexy::Image *GetImageWithAttrs(Sexy::String const& theName, Sexy::String const& theVariant,
                                           Sexy::String const& theAttributes, bool thePurge);
    virtual bool ProcessMessage(Sexy::Event & theEvent);
    virtual int DeviceAdded(Sexy::IInputDriver *theDriver);
    virtual int DeviceRemoved(Sexy::IInputDriver *theDriver);
    virtual void GamepadConnected();
    virtual void GamepadDisconnected();

    Sexy::Gamepad *mGamepads[4];
    bool           mSimulateGamepadWithKeyboard;
    bool           mGamePad1IsOn;
    CritSect       mDeviceEventCritSect;
    AtlasParser    mAtlasParser;
    CritSect       mAtlasCritSect;
    bool           mHasGamepad;
    bool           mGamePad2IsOn;

    Gamepad *GetGamepad(int theIndex);
    int GetGamepadIndex(int theSlot, int theDevice);
    GamepadApp* SwapGamepadId(int theFirst, int theSecond);
    int MapToButtonEvent(Sexy::Event const* theEvent, int & theButton, int & thePadIndex,
                         unsigned int & theRepeat);
    int CheckGamepad();
    int HasGamepad();
    int LoadAtlasMap(Sexy::String const& thePath);
    GamepadApp();
    Image *GetImageFromAtlas(Sexy::String const& theName);
    int ProcessDeviceEvents();

    struct PendingDeviceEvent
    {
        bool mAdded;
        int  mDeviceId;
    };

    std::list<PendingDeviceEvent> mPendingDeviceEvents;
    bool mPendingEventsReady;
    std::map<Sexy::String, SharedImageRef> mAtlasImages;
};

} // namespace Sexy

using Sexy::GamepadApp;

// GamepadApp -- the gamepad-aware application object.
//
// Purpose rewrite. The pending device queue is a list of arrivals and
// departures, drained once a frame. The atlas cache is a map from the atlas
// file to the shared image it was cut from. Device ids are the driver's slot,
// and a bound pad stores that slot in the high half of mDeviceId.

#include "GamepadApp.h"

#include "AtlasParser.h"
#include "CritSect.h"
#include "Gamepad.h"
#include "IInputDriver.h"
#include "Image.h"
#include "InputManager.h"
#include "MemoryImage.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"
#include "SharedImageRef.h"

#include <list>
#include <map>

namespace Sexy
{

// Keyboard stand-in for pad 0. The codes are the Win32 virtual keys the
// original switch compared against.
int ButtonFromKeyboard(KeyCode theKey)
{
    switch (theKey)
    {
        case KEYCODE_RETURN: return GAMEPAD_BUTTON_START;
        case KEYCODE_ESCAPE: return GAMEPAD_BUTTON_SELECT;
        case KEYCODE_LEFT:   return GAMEPAD_BUTTON_DPAD_LEFT;
        case KEYCODE_UP:     return GAMEPAD_BUTTON_DPAD_UP;
        case KEYCODE_RIGHT:  return GAMEPAD_BUTTON_DPAD_RIGHT;
        case KEYCODE_DOWN:   return GAMEPAD_BUTTON_DPAD_DOWN;
        case (KeyCode)'A':   return GAMEPAD_BUTTON_A;
        case (KeyCode)'S':   return GAMEPAD_BUTTON_B;
        case (KeyCode)'D':   return GAMEPAD_BUTTON_TL;
        case (KeyCode)'W':   return GAMEPAD_BUTTON_Y;
        case (KeyCode)'X':   return GAMEPAD_BUTTON_X;
        case (KeyCode)'C':   return GAMEPAD_BUTTON_TR2;
        case (KeyCode)'E':   return GAMEPAD_BUTTON_TL2;
        case (KeyCode)'F':   return GAMEPAD_BUTTON_TR;
        default:             return -1;
    }
}

} // namespace Sexy

void Sexy::GamepadApp::GamepadConnected()
{
}

void Sexy::GamepadApp::GamepadDisconnected()
{
}

int Sexy::GamepadApp::DeviceRemoved(Sexy::IInputDriver *theDriver)
{
    PendingDeviceEvent anEvent;
    anEvent.mDeviceId = theDriver->mSlot;
    anEvent.mAdded = false;

    mDeviceEventCritSect.Lock();
    mPendingDeviceEvents.push_back(anEvent);
    logfi("DeviceRemoved: %d", theDriver->mSlot);
    return mDeviceEventCritSect.Unlock();
}

int Sexy::GamepadApp::DeviceAdded(Sexy::IInputDriver *theDriver)
{
    PendingDeviceEvent anEvent;
    anEvent.mDeviceId = theDriver->mSlot;
    anEvent.mAdded = true;

    mDeviceEventCritSect.Lock();
    mPendingDeviceEvents.push_back(anEvent);
    logfi("DeviceAdded: %d", theDriver->mSlot);
    return mDeviceEventCritSect.Unlock();
}

int Sexy::GamepadApp::ProcessDeviceEvents()
{
    mDeviceEventCritSect.Lock();
    std::list<PendingDeviceEvent> aPending;
    aPending.swap(mPendingDeviceEvents);
    mDeviceEventCritSect.Unlock();

    for (const PendingDeviceEvent &anEvent : aPending)
    {
        const int aDeviceId = anEvent.mDeviceId;
        if (!anEvent.mAdded)
        {
            for (int i = 0; i < 4; i++)
                mGamepads[i]->CheckDeviceRemoved(aDeviceId << 16);
            continue;
        }

        IInputDriver *aDriver = mInputManager->Find(aDeviceId);
        if (aDriver == nullptr)
            continue;

        InputInfo aInfo;
        aInfo.mHasPointer = 0;
        aInfo.mHasKeys = 0;
        aInfo.mHasTouch = 0;
        aInfo.mHasMultiTouch = 0;
        aInfo.mHasJoystick = 0;
        aInfo.mDeviceIndex = -1;
        if (!aDriver->GetInfo(aInfo, 0))
            continue;

        logfi("Input device added: %s", aInfo.mName.c_str());
        // A stick (or the axis flag the original tested) and more than three
        // devices is what the original treated as a gamepad.
        if ((aInfo.mHasJoystick != 0 || aInfo.mHasTouch != 0) && aDriver->GetNumDevices() > 3)
        {
            for (int i = 0; i < 4; i++)
            {
                if (mGamepads[i]->CheckDeviceAdded(aDeviceId << 16) != 0)
                    break;
            }
            logfi("Recognized gamepad: %d:0", aDeviceId);
        }
    }
    return 0;
}

Sexy::Gamepad *Sexy::GamepadApp::GetGamepad(int theIndex)
{
    if (theIndex > 3)
        return nullptr;
    return mGamepads[theIndex];
}

int Sexy::GamepadApp::GetGamepadIndex(int theSlot, int theDevice)
{
    const int aDeviceKey = theDevice | (theSlot << 16);
    if (aDeviceKey == 0)
        return -1;

    for (int i = 0; i < 4; i++)
    {
        if (mGamepads[i]->mDeviceId == aDeviceKey)
            return i;
    }
    return -1;
}

Sexy::GamepadApp *Sexy::GamepadApp::SwapGamepadId(int theFirst, int theSecond)
{
    if (theFirst != theSecond && theFirst <= 3 && theSecond <= 3)
    {
        Gamepad *aFirst = mGamepads[theFirst];
        Gamepad *aSecond = mGamepads[theSecond];
        aFirst->SetGamepadIndex(theSecond);
        aSecond->SetGamepadIndex(theFirst);
        mGamepads[theFirst] = aSecond;
        mGamepads[theSecond] = aFirst;
    }
    return this;
}

int Sexy::GamepadApp::MapToButtonEvent(Sexy::Event const *theEvent, int &theButton,
                                       int &thePadIndex, unsigned int &theRepeat)
{
    const int aPadIndex = GetGamepadIndex((int)theEvent->lateModifiers, (int)theEvent->eventType);
    if (aPadIndex != -1)
    {
        Gamepad *aPad = mGamepads[aPadIndex];
        const int aButton = aPad->MapFromkeyCode((KeyCode)(unsigned int)theEvent->eventCodeMask);
        if (aButton == -1)
            return 0;

        theButton = aButton;
        thePadIndex = aPadIndex;
        theRepeat = ((unsigned int)theEvent->modifierMask & 0x80) != 0 ? 1u : 0u;
        return 1;
    }

    if (!mSimulateGamepadWithKeyboard || !mGamePad1IsOn)
        return 0;

    const int aButton = ButtonFromKeyboard((KeyCode)(unsigned int)theEvent->eventCodeMask);
    if (aButton == -1)
        return 0;

    theButton = aButton;
    thePadIndex = 0;
    return 1;
}

int Sexy::GamepadApp::CheckGamepad()
{
    for (int i = 0; i < 4; i++)
    {
        if (mGamepads[i]->IsConnected())
            return 1;
    }
    return 0;
}

int Sexy::GamepadApp::HasGamepad()
{
    if (mHasGamepad)
        return mGamePad2IsOn;
    return mHasGamepad;
}

int Sexy::GamepadApp::LoadAtlasMap(Sexy::String const &thePath)
{
    mAtlasCritSect.Lock();
    const int aResult = mAtlasParser.Load(thePath);
    mAtlasCritSect.Unlock();
    return aResult;
}

Sexy::GamepadApp::GamepadApp()
{
    for (int i = 0; i < 4; i++)
        mGamepads[i] = new Gamepad(this, i);

    if (mInputManager != nullptr)
        mInputManager->AddListener(static_cast<InputDeviceListener *>(this));

    mSimulateGamepadWithKeyboard = GetEnvOption("LAWN_SIMULATE_GAMEPAD", false) != 0;
    mGamePad1IsOn = false;
    mHasGamepad = false;
    mGamePad2IsOn = GetEnvOption("LAWN_GAMEPAD_MODE", false) != 0;
}

Sexy::Image *Sexy::GamepadApp::GetImageFromAtlas(Sexy::String const &theName)
{
    mAtlasCritSect.Lock();

    String aLower;
    StringToLower(&aLower, theName.c_str());

    bool aHasExtension = false;
    for (unsigned int anIndex = 0; anIndex < theName.size(); ++anIndex)
    {
        if (theName[anIndex] == '.')
        {
            aHasExtension = true;
            break;
        }
    }

    Atlas *anAtlas = mAtlasParser.Find(aLower);
    if (anAtlas == nullptr && !aHasExtension)
    {
        String aPngName = aLower;
        aPngName += ".png";
        anAtlas = mAtlasParser.Find(aPngName);
    }
    if (anAtlas == nullptr)
    {
        mAtlasCritSect.Unlock();
        return nullptr;
    }

    Image *anImage = nullptr;
    std::map<String, SharedImageRef>::iterator aCached = mAtlasImages.find(anAtlas->mPath);
    if (aCached != mAtlasImages.end())
        anImage = aCached->second;

    if (anImage == nullptr)
    {
        const String aPath = anAtlas->mPath;
        mAtlasCritSect.Unlock();

        String aVariant;
        SharedImageRef aLoaded = SexyAppBase::GetSharedImage(aPath, aVariant, nullptr, true, false);

        mAtlasCritSect.Lock();
        anImage = aLoaded;
        if (anImage == nullptr)
        {
            mAtlasCritSect.Unlock();
            return nullptr;
        }

        mAtlasImages[aPath] = aLoaded;
        anImage->SetPixelFormatHint((PixelFormat)mAtlasParser.GetPixelFormat(aPath));
        anImage->Finalize();
    }

    MemoryImage *aMemory = anImage->AsMemoryImage();
    Image *aSubImage = nullptr;
    if (aMemory != nullptr)
        aSubImage = aMemory->CreateSubImage(anAtlas->mX, anAtlas->mY, anAtlas->mWidth, anAtlas->mHeight);

    mAtlasCritSect.Unlock();
    return aSubImage;
}

void Sexy::GamepadApp::UpdateFrames()
{
    ProcessDeviceEvents();

    for (int i = 0; i < 4; i++)
        mGamepads[i]->UpdateStates(0.0f);

    const bool aConnected = CheckGamepad() != 0;
    if (mHasGamepad != aConnected)
    {
        mHasGamepad = aConnected;
        if (mHasGamepad)
            GamepadConnected();
        else
            GamepadDisconnected();
    }

    SexyCommonApp::UpdateFrames();
}

bool Sexy::GamepadApp::ProcessMessage(Sexy::Event &theEvent)
{
    if (theEvent.modifiers - 1 <= 1)
    {
        const int aDeviceKey = (int)theEvent.lateModifiers << 16;
        if (aDeviceKey != 0)
        {
            for (int i = 0; i < 4; i++)
            {
                Gamepad *aPad = mGamepads[i];
                if (aPad->mDeviceId == aDeviceKey)
                    aPad->CheckEvent(theEvent);
            }
        }

        // KEYCODE_GAMEPAD_MODE (0xE1) and the code beside it toggle the
        // keyboard stand-in on a key-up.
        if (mSimulateGamepadWithKeyboard && theEvent.modifiers == 2 &&
            (unsigned int)theEvent.eventCodeMask - 225 <= 1)
            mGamePad1IsOn = !mGamePad1IsOn;
    }

    return SexyAppBase::ProcessMessage(theEvent);
}

Sexy::Image *Sexy::GamepadApp::GetImage(Sexy::String const &theName, bool thePurge)
{
    Image *anImage = GetImageFromAtlas(theName);
    if (anImage == nullptr)
        return SexyAppBase::GetImage(theName, thePurge);

    anImage->mFileName = theName;
    return anImage;
}

Sexy::Image *Sexy::GamepadApp::GetImageWithAttrs(Sexy::String const &theName,
                                                Sexy::String const &theVariant,
                                                Sexy::String const &theAttrs, bool thePurge)
{
    Image *anImage = GetImageFromAtlas(theVariant);
    if (anImage == nullptr)
        return SexyAppBase::GetImageWithAttrs(theName, theVariant, theAttrs, thePurge);

    anImage->mFileName = theName;
    return anImage;
}

Sexy::GamepadApp::~GamepadApp()
{
    for (int i = 0; i < 4; ++i)
        delete mGamepads[i];
}

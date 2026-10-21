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

#include <cstdio>
#include <list>
#include <map>
#include <set>

namespace Sexy
{

} // namespace Sexy

void Sexy::GamepadApp::GamepadConnected() {}

void Sexy::GamepadApp::GamepadDisconnected() {}

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
    // DeviceAdded can fire from InputManager::Init, which runs before this
    // object's list constructor. Those notices are dropped. A pad already
    // attached at startup is asked for again on the first frame.
    if (!mPendingEventsReady)
    {
        mPendingEventsReady = true;
        if (mInputManager != nullptr)
        {
            for (IInputDriver *aDriver : mInputManager->Drivers())
            {
                if (aDriver != nullptr)
                    DeviceAdded(aDriver);
            }
        }
    }

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
            {
                Gamepad *aPad = mGamepads[i];
                if ((aPad->mDeviceId >> 16) == aDeviceId)
                {
                    IInputDriver *aDriver = mInputManager->Find(aDeviceId);
                    InputInfo aInfo{};
                    if (aDriver == nullptr || !aDriver->GetInfo(aInfo, aPad->mDeviceId & 65535) ||
                        aInfo.mHasJoystick == 0)
                        aPad->CheckDeviceRemoved(aPad->mDeviceId);
                }
            }
            continue;
        }

        IInputDriver *aDriver = mInputManager->Find(aDeviceId);
        if (aDriver == nullptr)
            continue;

        // SDL exposes multiple controllers through one driver. Keep its device
        // index in the low word so button events and axis polling use the same pad.
        const int aDeviceCount = aDriver->GetNumDevices();
        for (int aIndex = 0; aIndex < aDeviceCount && aIndex < 65536; ++aIndex)
        {
            InputInfo aInfo{};
            if (!aDriver->GetInfo(aInfo, aIndex) || aInfo.mHasJoystick == 0)
                continue;
            const int aDeviceKey = (aDeviceId << 16) | aIndex;
            if (GetGamepadIndex(aDeviceId, aIndex) != -1)
                continue;
            for (int i = 0; i < 4; i++)
            {
                if (mGamepads[i]->CheckDeviceAdded(aDeviceKey))
                {
                    logfi("Recognized gamepad: %d:%d: %s", aDeviceId, aIndex, aInfo.mName.c_str());
                    break;
                }
            }
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

int Sexy::GamepadApp::MapToButtonEvent(
    Sexy::Event const *theEvent, int &theButton, int &thePadIndex, unsigned int &theRepeat)
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

    return 0;
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

    mGamePad1IsOn = GetEnvOption("LAWN_SIMULATE_GAMEPAD", false) != 0;
    mHasGamepad = false;
    mGamePad2IsOn = GetEnvOption("LAWN_GAMEPAD_MODE", true) != 0;
    mPendingEventsReady = false;
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
            // Report each sheet once; every image cut from it fails the same way.
            static std::set<String> sReported;
            if (sReported.insert(aPath).second)
                std::fprintf(stderr, "atlas: cannot load sheet %s (needed for %s)\n", aPath.c_str(), theName.c_str());
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
    if (aSubImage == nullptr)
        std::fprintf(stderr, "atlas: cannot cut %s from sheet %s\n", theName.c_str(), anAtlas->mPath.c_str());

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
        if (theEvent.lateModifiers != 0)
        {
            const int aPadIndex = GetGamepadIndex(theEvent.lateModifiers, theEvent.eventType);
            if (aPadIndex != -1)
                mGamepads[aPadIndex]->CheckEvent(theEvent);
        }
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

Sexy::Image *Sexy::GamepadApp::GetImageWithAttrs(
    Sexy::String const &theName, Sexy::String const &theVariant, Sexy::String const &theAttrs, bool thePurge)
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

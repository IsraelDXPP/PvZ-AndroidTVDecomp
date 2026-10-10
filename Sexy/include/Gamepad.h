// One connected gamepad: button bits, six axes, and a connect/idle/active
// status. The TV build does not keep a weight table or a pad scale.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_GAMEPAD_H
#define LAWNPROJECTTV_SEXY_INCLUDE_GAMEPAD_H

#include "ReconTypes.h"
#include "InputManager.h"

namespace Sexy
{

class Gamepad
{
public:
    virtual ~Gamepad();

    virtual bool IsConnected();
    virtual void SetWeight(int theAxis, int theWeight);
    virtual int GetWeight(int theAxis);
    virtual void SetPadScale(int theAxis, float theScale);
    virtual float GetPadScale(int theAxis);
    virtual bool IsButtonDown(int theButton);
    virtual float GetButtonPressure(int theButton);
    virtual float GetAxisXPosition();
    virtual float GetAxisYPosition();
    virtual float GetAxisXPositionRamped();
    virtual float GetAxisYPositionRamped();
    virtual float GetRightAxisXPosition();
    virtual float GetRightAxisYPosition();
    virtual void UpdateStates(float theDelta);
    virtual void Reset();
    virtual void AddRumbleEffect(float theLow, float theHigh, float theDuration);

    // 0 disconnected, 1 connected, 2 idle after 20 seconds without input,
    // 3 active after a button or an axis move.
    enum Status
    {
        STATUS_DISCONNECTED = 0,
        STATUS_CONNECTED = 1,
        STATUS_IDLE = 2,
        STATUS_ACTIVE = 3,
    };

    Gamepad(Sexy::SexyAppBase *theApp, int theIndex);

    int MapTokeyCode(Sexy::GamepadButton theButton);
    int MapFromkeyCode(Sexy::KeyCode theKey);
    int GetGamepadIndex();
    void SetGamepadIndex(int theIndex);
    void SetListener(void *theListener);
    void *GetListener();
    void SetStatus(Status theStatus);
    bool HandleButtonDown(int theButton, unsigned int theFlags);
    bool HandleButtonUp(int theButton, unsigned int theFlags);
    bool HandleGameAxisMove(int theAxis, int theValue);
    bool CheckDeviceAdded(int theDeviceId);
    bool CheckDeviceRemoved(int theDeviceId);
    void CheckEvent(Sexy::Event const &theEvent);

    SexyAppBase *mApp;
    int mGamepadIndex;
    int mDeviceId;
    // Points at the controls object. The first three virtuals are button down,
    // button up, and axis move. The concrete type lives in the game layer.
    void *mListener;
    bool mIsConnected;
    int mAxisRaw[6];
    float mAxisValue[6];
    unsigned char mButtonDown[20];
    Status mStatus;
    unsigned int mStatusChangeTick;
    int mActiveCount;
    float mRumbleLow;
    float mRumbleHigh;
    float mRumbleDurationLow;
    float mRumbleDurationHigh;
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_GAMEPAD_H

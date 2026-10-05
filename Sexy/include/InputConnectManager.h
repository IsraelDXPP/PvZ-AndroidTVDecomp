// InputConnectManager -- the gamepad-input bridge the app owns.
//
// One object, 0x38 bytes, with two listener lists. The constructor wires both
// sentinels so AddListener can splice a node in front of them.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class InputConnectListener;
class InputManager;

class InputConnectManager
{
public:
    virtual ~InputConnectManager();

    void* mListenerNext;        // the listener list's sentinel
    void* mListenerPrev;        // +0x08
    unsigned char mCritSect[8]; // +0x0C
    void* mSecondNext;          // +0x14  the second sentinel
    void* mSecondPrev;          // +0x18
    int mReconnectPending;      // +0x1C
    int mConnectedCount;        // +0x20
    int mConnectingCount;       // +0x24
    int mReserved;              // +0x28
    int mReconnectDelayLong;    // +0x2C  20000 ms
    unsigned char mFlags[4];    // +0x30  the second byte is the constructor's bool
    int mReconnectDelay;        // +0x34  3000 ms

    InputConnectManager(bool theAutoReconnect);
    void AddListener(InputConnectListener* theListener);
    void RemoveListener(InputConnectListener* theListener);
    void Start(InputManager* theInput);
    void Stop();
    void Update();
};

} // namespace Sexy

// Win32GameCenter.h -- the local game centre for the Windows port.
//
// There is no store and no account server here. This object stands in for
// both: one fixed player, already signed in, and a recharge path that always
// succeeds. BuyMoney asks HasFeature(FEATURE_PURCHASE) and then Purchase; each
// call adds that pack's coins to the current user and tells the listener the
// payment went through. There is no daily cap and no anti-addiction gate.
// FEATURE_EXIT stays off so SdkExit still uses the game's own quit dialog.
//
// The object stays 0x24 bytes: that is the AndroidGameCenter allocation the
// factory replaces, and SetListener writes the listener at +0x08.

#pragma once

#include "Listener.h"

namespace Sexy {

// The one account this port signs in. LawnApp::GetUserName reads the same
// id through PlatformIntegration, so the buy dialog and the centre agree.
const char* FixedGameCenterUserId();
const char* FixedGameCenterUserName();

class Win32GameCenter : public IGameCenter
{
public:
    Win32GameCenter();
    virtual ~Win32GameCenter();

    virtual void Update();
    virtual void Pause();
    virtual void Resume();
    virtual bool HasFeature(Feature theFeature);
    virtual void Show(Sexy::String const& thePage);
    virtual int Purchase(Payment const& thePayment);
    virtual int GetProducts();
    virtual int QueryProducts();
    virtual IGameCenter* getUserName(IGameCenter* a2);
    virtual void Init();
    virtual int SetListener(Listener* a2);

    // +0x04 is the name the base constructor seeds and this one replaces.
    Sexy::String mName;
    // +0x08: SetListener's store. LawnApp::Init hands over its own listener.
    Listener* mListener;
    // +0x0C: the platform callback the constructor allocates and never calls
    // from this side of the bridge.
    void* mPlatformCallback;
    // +0x10, +0x14: the two strings the constructor seeds empty.
    Sexy::String mUserId;
    Sexy::String mSessionId;
    // +0x18..+0x1A: the three flag bytes the constructor clears.
    bool mLoggedIn;
    bool mQuerying;
    bool mPurchasing;
    // +0x1C, +0x20: the constructor's empty list head, both links pointing at
    // the head itself.
    void* mPendingHead;
    void* mPendingTail;
};

static_assert(sizeof(Win32GameCenter) == 0x24, "Win32GameCenter must be 0x24 bytes");

} // namespace Sexy

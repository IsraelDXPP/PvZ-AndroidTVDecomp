// Native::GameCenter -- reconstructed from libGameMain.so.
//
// No base class and no vtable for GameCenter itself: every body in GameCenter.cpp
// is a PLT stub that tail-calls the same-named function in the platform's native
// library, and none of them touches `this` (the file contains no `this`-relative
// access at all). So nothing is declared `virtual` on GameCenter.
//
// SIZE 0x08.
//   No body reads a member, so the extent can only come from the transcriber's
//   allocation record. The two words are almost certainly the native handle and the
//   registered listener, but nothing in this tree proves which is which.
//   TODO(recon): name the two words once a GameCenter body is transcribed; the
//   native import list (`__imp__ZN6Native10GameCenter...`) shows setListener takes a
//   `Listener *`, so one of them holds it.
//
// NESTED Listener. It IS polymorphic: the anonymous
// `(anonymous namespace)::AndroidGameCenterListener` derives from it
// (`_ZTIN12_GLOBAL__N_125AndroidGameCenterListenerE` @0x691018 records
// `_ZTIN6Native10GameCenter8ListenerE` @0x787A84 as its base), so the four callbacks
// are vtable slots. The inventory sizes them at 12, 2, 2 and 2 bytes -- i.e. three
// of them are empty inline no-ops. No virtual destructor is present: the inventory
// has no `Listener::~Listener` symbol, so adding one would shift all four slots.
// NOTE: nothing in this tree derives from Listener yet, so the declarations below
// are all that is needed; the bodies belong with the platform layer.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_GAMECENTER_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_GAMECENTER_H

#include "ReconTypes.h"

namespace Native
{

class GameCenter
{
public:
    // Native::GameCenter::Listener -- the callback interface. The leading
    // `GameCenter *` every callback takes is the SENDER, not the receiver: the
    // mangled names carry it explicitly
    // (`_ZN6Native10GameCenter8Listener7onEventEPS0_iiRKSs`) on top of the implicit
    // `Listener *this`.
    class Listener
    {
    public:
        virtual int onEvent(GameCenter *theSender, int theEvent, int theCode, Sexy::String const &theText);
        virtual void onPay(GameCenter *theSender, int theCode);
        virtual void onPayInfo(GameCenter *theSender, int theCode, Sexy::String const &theText);
        virtual void onInitFinished(GameCenter *theSender);
    };

    // ---- members, absolute offsets ----
    void *mNativeHandle; // +0x00  TODO(recon): unproven, see the size note
    Listener *mListener; // +0x04  setListener's parameter

    // setListener(Native::GameCenter::Listener*) -- the inventory records the
    // parameter; the transcription had dropped it.
    int setListener(Listener *theListener);
    int create();
    int getFeatures();
    int getName();
    int getUserTag();
    int generateOrderId();
    int getCurrency();
    int canMakePurchase();
    int queryProducts();
    int isQueryingProducts();
    int getPurchaseFeatures();
    int getAntiAddictionState();
    int queryAntiAddiction();
    int setProperty();
    int getProperty();
    int hasProperty();
    int queryFriends();
    int queryInvitableFriends();
    int addFriends();
    int isLoggedIn();
    int getLoginState();
    int login(int theMode);
    int logout();
    int switchAccount();
    int getUserId();
    int getUserName();
    int getSessionId();
    int show();
    int hide();
    int recharge(Native::GameCenter_RechargeArgs const &theArgs);
    int purchase(Native::GameCenter_Payment const &thePayment);
    int inviteFriend();
    int sendMessage(Native::GameCenter_Message const &theMessage);
    int reportScore(Native::GameCenter_Score const &theScore);
    int getFriends();
    int getInvitableFriends();
};

} // namespace Native

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_GAMECENTER_H

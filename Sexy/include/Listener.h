// AdProvider and IGameCenter, the two platform callback types this header
// owns. AdProvider is the payment and ad channel the app holds. IGameCenter
// is the store and account interface, with Listener nested inside it.
//
// Listener has twelve slots and no destructor. The defaults do nothing.
// The two-argument onPay drops the order id and forwards to the one-argument
// slot.
//
// Native game-center and share-manager listeners are separate types. Their
// bodies live in Listener.cpp and are declared by GameCenter.h and
// ShareManager.h.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class SexyCommonApp;

class AdProvider
{
public:
    AdProvider(Sexy::SexyCommonApp * theApp);

    virtual ~AdProvider();
    virtual void show(int theGravity);
    virtual void hide();
    virtual void disable();
    virtual int  getADType();
    virtual bool haveAppOffer();
    virtual void showAppOffer(Sexy::String const& theOffer, int theX, int theY, int theWidth, int theHeight);
    virtual void showItemPay(Sexy::String const& theItem, int theCount, Sexy::String const& theName,
                             Sexy::String const& theDescription, float thePrice, int theFlags);
    virtual void queryBalance(Sexy::String const& theAccount);
    virtual void goToLogin(bool theForce);
    virtual void getUserName(Sexy::String& theName);
    virtual void getNickname(Sexy::String& theName);
    virtual void update();
    virtual void recharge();

    Sexy::SexyCommonApp *mApp = nullptr;
    Sexy::String mChannel;
    bool mDisabled = false;
};

class IGameCenter
{
public:
    static void * Create(SexyCommonApp * theApp);

    virtual void Update() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;

    enum Feature
    {
        FEATURE_PURCHASE = 16,
        FEATURE_EXIT = 64,
    };
    virtual bool HasFeature(Feature theFeature) = 0;
    virtual void Show(Sexy::String const& thePage) = 0;

    struct Payment
    {
        Sexy::String mProductId;
        float mPrice;
        Sexy::String mDescription;
        Sexy::String mUserName;
        Sexy::String mAccount;
    };

    virtual int Purchase(Payment const& thePayment) = 0;

    virtual int GetProducts() = 0;
    virtual int QueryProducts() = 0;

    virtual Sexy::String getUserName() = 0;

    virtual void Init() = 0;

    class Listener
    {
    public:
        virtual void onLogin();
        virtual void onLogout();
        virtual void onLeave();
        virtual void onSessionInvalid();
        virtual void onProductsReceived();
        virtual void onQueryAntiAddiction();
        virtual void onQueryFriends();
        virtual void onQueryInvitableFriends();
        virtual void onInviteFriend(int theResult);
        virtual void onPay(int theResult);
        virtual void onPay(int theResult, Sexy::String const& theOrderId);
        virtual void onMessage();
    };

    virtual int SetListener(Listener * theListener) = 0;
};

} // namespace Sexy

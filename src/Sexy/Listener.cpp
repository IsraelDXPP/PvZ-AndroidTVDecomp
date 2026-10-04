// Listener. IGameCenter's twelve slots are empty defaults. The two-argument
// onPay drops the order id and forwards to the one-argument slot.
//
// The native game-center and share-manager listeners are separate types.
// onEvent forwards to the platform bridge; the rest do nothing.

#include "Listener.h"

#include "GameCenter.h"
#include "ShareManager.h"

int __imp__ZN6Native10GameCenter8Listener7onEventEPS0_iiRKSs(...);

int Native::GameCenter::Listener::onEvent(Native::GameCenter *theSender, int theCode, int theExtra, Sexy::String const &theText)
{
  return __imp__ZN6Native10GameCenter8Listener7onEventEPS0_iiRKSs(theSender, theCode, theExtra, theText);
}

void Sexy::IGameCenter::Listener::onLogin() {}

void Sexy::IGameCenter::Listener::onLogout() {}

void Sexy::IGameCenter::Listener::onLeave() {}

void Sexy::IGameCenter::Listener::onSessionInvalid() {}

void Sexy::IGameCenter::Listener::onProductsReceived() {}

void Sexy::IGameCenter::Listener::onQueryAntiAddiction() {}

void Sexy::IGameCenter::Listener::onQueryFriends() {}

void Sexy::IGameCenter::Listener::onQueryInvitableFriends() {}

void Sexy::IGameCenter::Listener::onInviteFriend(int theResult)
{
  (void)theResult;
}

void Sexy::IGameCenter::Listener::onPay(int theResult)
{
  (void)theResult;
}

void Sexy::IGameCenter::Listener::onPay(int theResult, Sexy::String const &theOrderId)
{
  (void)theOrderId;
  onPay(theResult);
}

void Sexy::IGameCenter::Listener::onMessage() {}

void Native::GameCenter::Listener::onPay(Native::GameCenter *theSender, int theResult)
{
  (void)theSender;
  (void)theResult;
}

void Native::GameCenter::Listener::onPayInfo(Native::GameCenter *theSender, int theResult, Sexy::String const &theInfo)
{
  (void)theSender;
  (void)theResult;
  (void)theInfo;
}

void Native::GameCenter::Listener::onInitFinished(Native::GameCenter *theSender)
{
  (void)theSender;
}

void Native::ShareManager::Listener::onComplete(Native::ShareManager *theSender, int theResult)
{
  (void)theSender;
  (void)theResult;
}

void Native::ShareManager::Listener::onCancel(Native::ShareManager *theSender, int theResult)
{
  (void)theSender;
  (void)theResult;
}

void Native::ShareManager::Listener::onError(Native::ShareManager *theSender, int theResult)
{
  (void)theSender;
  (void)theResult;
}

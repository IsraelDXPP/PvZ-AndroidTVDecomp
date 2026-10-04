// Win32GameCenter.cpp -- local game centre for the Windows port.
//
// One fixed player, already signed in. Purchase always credits the pack the
// game asked for and reports success. The coin file is the same one
// LawnApp::writeCoinNum / loadCoinNum use, so a recharge survives a restart.

#include "Win32GameCenter.h"

#include "LawnUser.h"
#include "ReconHelpers.h"

#include <cstdio>
#include <fstream>

namespace {

// The three packs BuyMoney names pvz50 / pvz100 / pvz200. The granted counts
// are LawnApp::setItemToCoinItems' mGetCoin column.
const int kPackCoins[3] = { 5, 10, 20 };

int CoinsForProduct(const Sexy::String& theProductId)
{
    if (theProductId == "pvz50")
        return kPackCoins[0];
    if (theProductId == "pvz100")
        return kPackCoins[1];
    if (theProductId == "pvz200")
        return kPackCoins[2];
    return kPackCoins[1];
}

void RememberCoins(int theBalance)
{
    char aText[32];
    sprintf(aText, "%d", theBalance);

    Sexy::String aPath;
    GetAppDataFolder(&aPath);
    aPath += "/CoinFile";

    std::ofstream aFile(aPath.c_str(), std::ios::out | std::ios::trunc);
    if (aFile)
        aFile << aText;
}

void CreditCoins(int theCoins)
{
    if (theCoins <= 0)
        return;

    LawnUser* aUser = LawnUser::getCurUser();
    if (aUser == nullptr)
        return;

    int aBalance = atoi(aUser->mCoinText.c_str()) + theCoins;
    char aText[32];
    sprintf(aText, "%d", aBalance);
    aUser->mCoinText = aText;
    RememberCoins(aBalance);
}

} // namespace

const char* Sexy::FixedGameCenterUserId()
{
    return "LawnPlayer";
}

const char* Sexy::FixedGameCenterUserName()
{
    return "LawnPlayer";
}

Sexy::Win32GameCenter::Win32GameCenter()
    : mName(FixedGameCenterUserName()),
      mListener(nullptr),
      mPlatformCallback(nullptr),
      mUserId(FixedGameCenterUserId()),
      mSessionId("local"),
      mLoggedIn(true),
      mQuerying(false),
      mPurchasing(false)
{
    mPendingHead = &mPendingHead;
    mPendingTail = &mPendingHead;
}

Sexy::Win32GameCenter::~Win32GameCenter()
{
}

void Sexy::Win32GameCenter::Update()
{
}

void Sexy::Win32GameCenter::Pause()
{
}

void Sexy::Win32GameCenter::Resume()
{
}

// Purchase is local and unlimited. Exit stays with the game's quit dialog.
bool Sexy::Win32GameCenter::HasFeature(Feature theFeature)
{
    if (theFeature == FEATURE_EXIT)
        return false;
    return theFeature == FEATURE_PURCHASE;
}

void Sexy::Win32GameCenter::Show(Sexy::String const&)
{
}

// Every pack succeeds. Coins land on the current user before the listener
// hears about it, so BuyMoney's later balance poll sees the new amount.
int Sexy::Win32GameCenter::Purchase(Payment const& thePayment)
{
    CreditCoins(CoinsForProduct(thePayment.mProductId));
    if (mListener != nullptr)
        mListener->onPay(0);
    return 1;
}

int Sexy::Win32GameCenter::GetProducts()
{
    if (mListener != nullptr)
        mListener->onProductsReceived();
    return 1;
}

int Sexy::Win32GameCenter::QueryProducts()
{
    mQuerying = false;
    if (mListener != nullptr)
        mListener->onProductsReceived();
    return 1;
}

// The getter returns the name by value. The caller's destination arrives as
// the parameter because that is how the ARM return works; the object written
// is the caller's own string.
Sexy::IGameCenter* Sexy::Win32GameCenter::getUserName(IGameCenter* a2)
{
    if (a2 != nullptr)
        *reinterpret_cast<Sexy::String*>(a2) = mName;
    return a2;
}

void Sexy::Win32GameCenter::Init()
{
    mLoggedIn = true;
    LawnUser* aUser = LawnUser::getCurUser();
    if (aUser != nullptr)
        aUser->mAccount = mUserId;
    if (mListener != nullptr)
        mListener->onLogin();
}

int Sexy::Win32GameCenter::SetListener(Listener* theListener)
{
    mListener = theListener;
    if (mListener != nullptr && mLoggedIn)
        mListener->onLogin();
    return 1;
}

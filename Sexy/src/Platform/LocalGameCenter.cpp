// LocalGameCenter.cpp -- local player and purchase service.
//
// One fixed player, already signed in. Purchase always credits the pack the
// game asked for and reports success. The coin file is the same one
// LawnApp::writeCoinNum / loadCoinNum use, so a recharge survives a restart.

#include "LocalGameCenter.h"

#include "LawnUser.h"
#include "ReconHelpers.h"
#include "SDLFile.h"

#include <cstdio>

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
    MkDir(aPath);
    aPath += "/CoinFile";

    Sexy::SDLFile::WriteOutputFile(aPath.c_str(), aText, SDL_strlen(aText));
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

Sexy::LocalGameCenter::LocalGameCenter()
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

Sexy::LocalGameCenter::~LocalGameCenter()
{
}

void Sexy::LocalGameCenter::Update()
{
}

void Sexy::LocalGameCenter::Pause()
{
}

void Sexy::LocalGameCenter::Resume()
{
}

// Purchase is local and unlimited. Exit stays with the game's quit dialog.
bool Sexy::LocalGameCenter::HasFeature(Feature theFeature)
{
    if (theFeature == FEATURE_EXIT)
        return false;
    return theFeature == FEATURE_PURCHASE;
}

void Sexy::LocalGameCenter::Show(Sexy::String const&)
{
}

// Every pack succeeds. Coins land on the current user before the listener
// hears about it, so BuyMoney's later balance poll sees the new amount.
int Sexy::LocalGameCenter::Purchase(Payment const& thePayment)
{
    CreditCoins(CoinsForProduct(thePayment.mProductId));
    if (mListener != nullptr)
        mListener->onPay(0);
    return 1;
}

int Sexy::LocalGameCenter::GetProducts()
{
    if (mListener != nullptr)
        mListener->onProductsReceived();
    return 1;
}

int Sexy::LocalGameCenter::QueryProducts()
{
    mQuerying = false;
    if (mListener != nullptr)
        mListener->onProductsReceived();
    return 1;
}

Sexy::String Sexy::LocalGameCenter::getUserName()
{
    return mName;
}

void Sexy::LocalGameCenter::Init()
{
    mLoggedIn = true;
    LawnUser* aUser = LawnUser::getCurUser();
    if (aUser != nullptr)
        aUser->mAccount = mUserId;
    if (mListener != nullptr)
        mListener->onLogin();
}

int Sexy::LocalGameCenter::SetListener(Listener* theListener)
{
    mListener = theListener;
    if (mListener != nullptr && mLoggedIn)
        mListener->onLogin();
    return 1;
}

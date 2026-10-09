// SexyCommonApp wires the platform services the TV shell expects and steps
// them once per frame. Window, input and drawing stay on SexyAppBase.

#include "SexyCommonApp.h"

#include "Listener.h"
#include "ReconData.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"

#include <cstdio>
#include <string>
#include <vector>

namespace Sexy {

class CommonAuthManager : public AuthManager
{
public:
    CommonAuthManager(SexyAppBase* theApp);
};

// The constructor has to live with this class. A same-named stub in another
// file does not inherit AuthManager, so its empty body never writes the vtable.
CommonAuthManager::CommonAuthManager(SexyAppBase* theApp)
    : AuthManager(theApp)
{
}

class PlatformIntegration
{
public:
    PlatformIntegration(SexyAppBase* theApp);
    virtual ~PlatformIntegration();
    virtual void Init();
    virtual void Cleanup();
    virtual Sexy::String GetHardwareID();
    virtual Sexy::String GetUserID();
    virtual Sexy::String GetUserName();
    virtual Sexy::String GetSecretKey();
    virtual Sexy::String GetRechargeLimit();

    Sexy::String mChannel;
    SexyAppBase* mApp;
};

class UpdateRemoteListener : public InputDeviceListener
{
public:
    UpdateRemoteListener();
    virtual ~UpdateRemoteListener();
    virtual void DeviceEventGenerated(IInputDriver* theDriver, Event* theEvent);
    Sexy::String mName;
};

UpdateRemoteListener::UpdateRemoteListener()
    : mName("unknownRemote")
{
}

UpdateRemoteListener::~UpdateRemoteListener()
{
}

void UpdateRemoteListener::DeviceEventGenerated(IInputDriver*, Event*)
{
}

class TrialTimeCheck
{
public:
    TrialTimeCheck(SexyCommonApp*, unsigned int theStartTick)
        : mStartTick(theStartTick)
    {
    }
    void Step(unsigned int) {}
    unsigned int mStartTick;
};

class IShareManager
{
public:
    static void* Create(SexyCommonApp* theApp);
    virtual ~IShareManager() {}
    virtual void Init() {}
    virtual void Pause() {}
    virtual void Resume() {}
    virtual void Update() {}
};

class INotificationManager
{
public:
    static void* Create(SexyCommonApp* theApp);
    virtual ~INotificationManager() {}
    virtual void Pause() {}
    virtual void Resume() {}
    virtual void Init() {}
};

class SPayGameInfo
{
public:
    class SPayItem
    {
    public:
        int mValue;
        Sexy::String mName;
    };

    Sexy::String mLabel;
    std::vector<SPayItem> mItems;
};

class PaymentManager
{
public:
    virtual SPayGameInfo GetPayInfo();
    virtual int GotoBuyGame(int theProduct);
    virtual int NotGoToBuy();
    virtual int GetStatus();
};

} // namespace Sexy

int MSleep(...);
int j_AGShowMoreGame();
int j_AGShow91MoreGame();
int j_AGGoToMarket(...);
int j_AGPostData(...);
int j_AGPostXMLFileData(...);
int j_AGShowMessageBox(...);

namespace {

Sexy::String HashText(const Sexy::String& theText)
{
    int aHash = 0;
    const char* aBytes = theText.c_str();
    unsigned int aLength = theText.size();
    for (unsigned int aIndex = 0; aIndex < aLength; ++aIndex)
        aHash = 31 * aHash + (unsigned char)aBytes[aIndex];

    char aDigits[16];
    sprintf(aDigits, "%d", aHash);
    return Sexy::String(aDigits);
}

bool NameIs(const Sexy::String& theName, const char* theLiteral)
{
    return theName == theLiteral;
}

} // namespace

Sexy::ConnectionManager::ConnectionManager()
    : mMulti(nullptr)
{
}

Sexy::ConnectionManager::~ConnectionManager()
{
}

int Sexy::ConnectionManager::Update(int)
{
    return 0;
}

void Sexy::SexyCommonApp::setErrorCode()
{
}

void Sexy::SexyCommonApp::showRechargeBoard()
{
}

int Sexy::SexyCommonApp::FullVersionHandle(bool theFull, void* theApp)
{
    return static_cast<SexyCommonApp*>(theApp)->setFullVersion(theFull);
}

int Sexy::SexyCommonApp::MessageBoxHandle(int theResult, void* theApp)
{
    return static_cast<SexyCommonApp*>(theApp)->messageBoxRes(theResult);
}

void Sexy::SexyCommonApp::AppEnteredBackground()
{
    if (mGameCenter != nullptr)
        mGameCenter->Pause();
    if (mShareManager != nullptr)
        mShareManager->Pause();
    if (mNotificationManager != nullptr)
        mNotificationManager->Pause();
}

void Sexy::SexyCommonApp::AppEnteredForeground()
{
    if (mGameCenter != nullptr)
        mGameCenter->Resume();
    if (mShareManager != nullptr)
        mShareManager->Resume();
    if (mNotificationManager != nullptr)
        mNotificationManager->Resume();
}

void Sexy::SexyCommonApp::DoGameTrialDialog(const SPayGameInfo&)
{
}

void Sexy::SexyCommonApp::CreateGameTrialDialog()
{
}

void Sexy::SexyCommonApp::CreateGotoBuyResultDialog()
{
}

void Sexy::SexyCommonApp::CreateWaitResultDialog()
{
}

void Sexy::SexyCommonApp::CreateWaitMessageDialog()
{
}

void Sexy::SexyCommonApp::CreateNetworkFaultDialog()
{
}

int Sexy::SexyCommonApp::setOfferRes(bool theResult)
{
    return logfd("setOfferRes: %d\n", theResult ? 1 : 0);
}

int Sexy::SexyCommonApp::setOfferFullOpenRes(bool theResult)
{
    return logfd("setOfferFullOpenRes: %d\n", theResult ? 1 : 0);
}

int Sexy::SexyCommonApp::setFullVersion(bool theFull)
{
    return logfd("setFullVersion: %d\n", theFull ? 1 : 0);
}

int Sexy::SexyCommonApp::setPayItemsRes(bool theResult, const char* theItemId, int theCount, const char* theChannel)
{
    return logfd("setPayItemsRes: %d %s %d %s\n", theResult ? 1 : 0, theItemId, theCount, theChannel);
}

int Sexy::SexyCommonApp::setQueryBalanceRes(bool theResult, int theBalance)
{
    return logfd("setQueryBalanceRes: %d %d\n", theResult ? 1 : 0, theBalance);
}

int Sexy::SexyCommonApp::adBarClick()
{
    return logfd("@@@@@@@@@@@@@adBarClick\n");
}

int Sexy::SexyCommonApp::setLoginRes(bool theResult, const char* theUser)
{
    return logfd("setLoginResRes: %d %s \n", theResult ? 1 : 0, theUser);
}

int Sexy::SexyCommonApp::messageBoxRes(int theResult)
{
    return logfd("messageBoxRes: %d\n", theResult);
}

int Sexy::SexyCommonApp::DoAuthenticate()
{
    if (mAuthManager == nullptr)
        return 1;
    if (mAuthManager->GetAuthState() == 0)
    {
        mAuthManager->SetAuthId(mAuthId);
        mAuthManager->Start();
    }
    while (mAuthManager->GetAuthState() <= 1)
        MSleep(1000);
    int aResult = mAuthManager->IsAuthenticaled();
    mAuthenticated = aResult != 0;
    return aResult;
}

void Sexy::SexyCommonApp::Init()
{
    if (mAuthManager != nullptr)
        mAuthManager->SetAuthId(mAuthId);
    if (mPlatformIntegration != nullptr)
        mPlatformIntegration->Init();
    if (mGameCenter != nullptr)
        mGameCenter->Init();
    if (mShareManager != nullptr)
        mShareManager->Init();
    if (mNotificationManager != nullptr)
        mNotificationManager->Init();
    if (mAdProvider == nullptr)
        mAdProvider = new AdProvider(this);
    SexyAppBase::Init();
}

Sexy::SexyCommonApp::SexyCommonApp()
    : mPlatformIntegration(nullptr),
      mGamePaidQueryStartTick(0),
      mPaymentManager(nullptr),
      mTrialTimeCheck(nullptr),
      mAdProvider(nullptr),
      mUpdateRemoteListener(nullptr),
      mSoundControllerWidget(nullptr),
      mGameCenter(nullptr),
      mShareManager(nullptr),
      mNotificationManager(nullptr),
      mAuthenticated(true),
      mAuthId(-1),
      mMarketOrAuthMode(2),
      mTestAdRecallMode(0),
      mTestAdRecallResult(false),
      mTestAdRecallCounter(0),
      mTestAdPayItemId("1"),
      mTestAdPayItemCount(2),
      mTestAdPayChannel("alipay"),
      mTrialCheckStarted(false),
      mTrialCheckDelayPassed(false)
{
    if (mAuthManager != nullptr)
        delete mAuthManager;
    mAuthManager = new CommonAuthManager(this);
    mGameCenter = static_cast<IGameCenter*>(IGameCenter::Create(this));
    mShareManager = static_cast<IShareManager*>(IShareManager::Create(this));
    mNotificationManager = static_cast<INotificationManager*>(INotificationManager::Create(this));
    mPlatformIntegration = new PlatformIntegration(this);
    AGSetFullVersionListener(&SexyCommonApp::FullVersionHandle, this);
    AGSetMessageBoxListener(&SexyCommonApp::MessageBoxHandle, this);

    UpdateRemoteListener* aRemote = new UpdateRemoteListener();
    mUpdateRemoteListener = aRemote;
    if (mInputManager != nullptr)
        mInputManager->AddListener(aRemote);

    mSoundControllerWidget = new SoundControllerWidget();
    mSoundControllerWidget->mZOrder = 1;
}

Sexy::IGameCenter* Sexy::SexyCommonApp::GetGameCenter()
{
    return mGameCenter;
}

Sexy::IShareManager* Sexy::SexyCommonApp::GetShareManager()
{
    return mShareManager;
}

Sexy::INotificationManager* Sexy::SexyCommonApp::GetNotificationManager()
{
    return mNotificationManager;
}

Sexy::PlatformIntegration* Sexy::SexyCommonApp::GetPlatformIntegration()
{
    return mPlatformIntegration;
}

int Sexy::SexyCommonApp::GotoBuyGame(int theProduct)
{
    if (mPaymentManager == nullptr)
        return 0;
    return mPaymentManager->GotoBuyGame(theProduct);
}

int Sexy::SexyCommonApp::NotGoToBuy()
{
    if (mPaymentManager == nullptr)
        return 0;
    return mPaymentManager->NotGoToBuy();
}

bool Sexy::SexyCommonApp::IsGamePaid()
{
    if (mPaymentManager == nullptr)
        return true;
    return mPaymentManager->GetStatus() == 0;
}

int Sexy::SexyCommonApp::GamePaidStatus()
{
    if (mPaymentManager == nullptr)
        return -1;
    int aStatus = mPaymentManager->GetStatus();
    if (aStatus < 0 || aStatus > 3)
        return -1;
    return gPaidStatusCode[aStatus];
}

int Sexy::SexyCommonApp::setTestAdRecall(SexyCommonApp_E_TESTAD_MODE theMode, bool theResult)
{
    int aLogged = logfi("setTestAdRecall: %d %d", (int)theMode, theResult ? 1 : 0);
    mTestAdRecallMode = (int)theMode;
    mTestAdRecallResult = theResult;
    mTestAdRecallCounter = 0;
    return aLogged;
}

void Sexy::SexyCommonApp::setTestAdPayItemsRecall(bool theResult, Sexy::String theItemId, int theCount, Sexy::String theChannel)
{
    logfi("setTestAdPayItemsRecall: %d %s %d %s", theResult ? 1 : 0, theItemId.c_str(), theCount, theChannel.c_str());
    mTestAdRecallResult = theResult;
    mTestAdRecallMode = 3;
    mTestAdRecallCounter = 0;
    mTestAdPayItemId = theItemId;
    mTestAdPayItemCount = theCount;
    mTestAdPayChannel = theChannel;
}

int Sexy::SexyCommonApp::updateTestRecall()
{
    if (mTestAdRecallMode == 0)
        return 0;
    if (++mTestAdRecallCounter <= 100)
        return 0;

    switch (mTestAdRecallMode)
    {
    case 1:
        setOfferRes(mTestAdRecallResult);
        break;
    case 2:
        setOfferFullOpenRes(mTestAdRecallResult);
        break;
    case 3:
        setPayItemsRes(mTestAdRecallResult, mTestAdPayItemId.c_str(), mTestAdPayItemCount, mTestAdPayChannel.c_str());
        break;
    case 4:
        setFullVersion(mTestAdRecallResult);
        break;
    default:
        break;
    }
    mTestAdRecallCounter = 0;
    mTestAdRecallMode = 0;
    return 0;
}

int Sexy::SexyCommonApp::showMoreGame()
{
    if (gMoreGameChannel == "xiaomi")
        return j_AGShow91MoreGame();
    return j_AGShowMoreGame();
}

void Sexy::SexyCommonApp::showRecommendGames()
{
}

void Sexy::SexyCommonApp::Cleanup()
{
    if (mInputManager != nullptr)
        mInputManager->RemoveListener(mUpdateRemoteListener);
    delete mUpdateRemoteListener;
    mUpdateRemoteListener = nullptr;

    SexyAppBase::Cleanup();

    delete mGameCenter;
    mGameCenter = nullptr;
    delete mShareManager;
    mShareManager = nullptr;
    delete mNotificationManager;
    mNotificationManager = nullptr;
    delete mPlatformIntegration;
    mPlatformIntegration = nullptr;
    showRecommendGames();
}

Sexy::SexyCommonApp::~SexyCommonApp()
{
    AGSetFullVersionListener(nullptr, nullptr);
    AGSetMessageBoxListener(nullptr, nullptr);
    Cleanup();
    delete mAdProvider;
    mAdProvider = nullptr;
    delete mPaymentManager;
    mPaymentManager = nullptr;
    delete mTrialTimeCheck;
    mTrialTimeCheck = nullptr;
    delete mSoundControllerWidget;
    mSoundControllerWidget = nullptr;
}

int Sexy::SexyCommonApp::goToMarket()
{
    return j_AGGoToMarket(this);
}

int Sexy::SexyCommonApp::postData(Sexy::String theKind, Sexy::String theBody)
{
    return j_AGPostData(theKind.c_str(), theBody.c_str());
}

void Sexy::SexyCommonApp::postData(SexyCommonApp_POST_TYPE theKind, Sexy::String theBody)
{
    const char* aKind = "game";
    if ((int)theKind == 1)
        aKind = "ad";
    postData(Sexy::String(aKind), theBody);
}

int Sexy::SexyCommonApp::postXMLFileData(Sexy::String thePath)
{
    return j_AGPostXMLFileData(thePath.c_str());
}

int Sexy::SexyCommonApp::showMessageBox(Sexy::String theText, Sexy::String theTitle, SexyCommonApp_E_MESSAGEBOX_TYPE theType, int theFlags)
{
    return j_AGShowMessageBox(theText.c_str(), theTitle.c_str(), (int)theType, theFlags);
}

Sexy::String Sexy::SexyCommonApp::getUserIdentifier()
{
    if (mPlatformIntegration == nullptr)
        return Sexy::String();
    return mPlatformIntegration->GetUserID();
}

Sexy::String Sexy::SexyCommonApp::getUserName()
{
    if (mPlatformIntegration == nullptr)
        return Sexy::String();
    return mPlatformIntegration->GetUserName();
}

Sexy::String Sexy::SexyCommonApp::getSecretKey()
{
    if (mPlatformIntegration == nullptr)
        return Sexy::String();
    return mPlatformIntegration->GetSecretKey();
}

Sexy::String Sexy::SexyCommonApp::getRechargeLimit()
{
    if (mPlatformIntegration == nullptr)
        return Sexy::String();
    return mPlatformIntegration->GetRechargeLimit();
}

int Sexy::SexyCommonApp::IsRemoteControllerEnabled()
{
    DriverFactory* aFactory = InputDriverFactory::GetInputDriverFactory();
    if (aFactory == nullptr)
        return 0;
    return aFactory->Find("RemoteInputInterface") != nullptr ? 1 : 0;
}

Sexy::String Sexy::SexyCommonApp::getCurrentRemoteName()
{
    if (mUpdateRemoteListener == nullptr)
        return Sexy::String("unknownRemote");
    return static_cast<UpdateRemoteListener*>(mUpdateRemoteListener)->mName;
}

Sexy::SexyCommonApp::GameInfoRecord Sexy::SexyCommonApp::getGameInfo()
{
    // Publisher/channel metadata is not used by the local SDL application.
    return GameInfoRecord{};
}

void Sexy::SexyCommonApp::CreateTrialDialog()
{
    SPayGameInfo aRecord;
    if (mPaymentManager != nullptr)
    {
        SPayGameInfo aInfo = mPaymentManager->GetPayInfo();
        aRecord.mLabel = aInfo.mLabel;
        aRecord.mItems = aInfo.mItems;
    }
    else
    {
        aRecord.mLabel = "Error:No Info";
    }
    DoGameTrialDialog(aRecord);
}

void Sexy::SexyCommonApp::HandleCmdLineParam(const Sexy::String& theName, const Sexy::String& theValue)
{
    if (NameIs(theName, "-user") || NameIs(theName, "-n") || NameIs(theName, "-userid"))
    {
        mUserId = theValue;
        return;
    }
    if (NameIs(theName, "-stbid"))
    {
        mStbId = theValue;
        mStbIdDerived = HashText(theValue);
        return;
    }
    if (NameIs(theName, "-s") || NameIs(theName, "-server"))
    {
        mServer = theValue;
        return;
    }
    if (NameIs(theName, "-u"))
    {
        mLoginUser = theValue;
        return;
    }
    if (NameIs(theName, "-p"))
    {
        mLoginPassword = theValue;
        return;
    }
    if (NameIs(theName, "-f"))
    {
        mLoginFlag = theValue;
        return;
    }
    if (NameIs(theName, "-spay"))
    {
        mSPay = theValue;
        return;
    }
    if (NameIs(theName, "-appsessionid"))
    {
        mAppSessionId = theValue;
        return;
    }
    if (NameIs(theName, "-globalareacode"))
    {
        mGlobalAreaCode = theValue;
        return;
    }
    SexyAppBase::HandleCmdLineParam(theName, theValue);
}

int Sexy::SexyCommonApp::OfferResHandle(bool theResult, void* theApp)
{
    return static_cast<SexyCommonApp*>(theApp)->setOfferRes(theResult);
}

int Sexy::SexyCommonApp::OfferFullOpenResHandle(bool theResult, void* theApp)
{
    return static_cast<SexyCommonApp*>(theApp)->setOfferFullOpenRes(theResult);
}

int Sexy::SexyCommonApp::PayItemsResHandle(bool theResult, const char* theItemId, int theCount, const char* theChannel, void* theApp)
{
    return static_cast<SexyCommonApp*>(theApp)->setPayItemsRes(theResult, theItemId, theCount, theChannel);
}

int Sexy::SexyCommonApp::QueryBalanceResHandle(bool theResult, int theBalance, void* theApp)
{
    return static_cast<SexyCommonApp*>(theApp)->setQueryBalanceRes(theResult, theBalance);
}

int Sexy::SexyCommonApp::AdBarClickHandle(void* theApp)
{
    return static_cast<SexyCommonApp*>(theApp)->adBarClick();
}

int Sexy::SexyCommonApp::LoginResHandle(bool theResult, const char* theUser, void* theApp)
{
    return static_cast<SexyCommonApp*>(theApp)->setLoginRes(theResult, theUser);
}

void Sexy::SexyCommonApp::UpdateFrames()
{
    if (mPaymentManager != nullptr)
    {
        int aPaidState = mPaymentManager->GetStatus();
        if (aPaidState == 3 && mTrialTimeCheck == nullptr)
        {
            mTrialCheckStarted = true;
            mTrialTimeCheck = new TrialTimeCheck(this, GetTickCount());
            logfd("The game is a trial version.\n");
        }
        else if (aPaidState == 2 &&
                 GetTickCount() >= mGamePaidQueryStartTick + 10000 &&
                 !mTrialCheckDelayPassed &&
                 mTrialTimeCheck == nullptr)
        {
            mTrialCheckStarted = true;
            mTrialTimeCheck = new TrialTimeCheck(this, GetTickCount());
            logfd("Failed to query game paid information from server.\n");
        }
        else if (aPaidState == 1 && mTrialTimeCheck == nullptr)
        {
            mTrialCheckStarted = true;
            mTrialTimeCheck = new TrialTimeCheck(this, GetTickCount());
            logfd("The game isn't paid.\n");
        }
        else if (aPaidState == 0)
        {
            logfd("The game is already paid.\n");
            delete mPaymentManager;
            mPaymentManager = nullptr;
            delete mTrialTimeCheck;
            mTrialTimeCheck = nullptr;
        }

        if (mTrialTimeCheck != nullptr)
            mTrialTimeCheck->Step(GetTickCount());
    }

    SexyAppBase::UpdateFrames();
    mConnectionManager.Update(0);
    updateTestRecall();
    if (mAdProvider != nullptr)
        mAdProvider->update();
    if (mSoundControllerWidget != nullptr)
        mSoundControllerWidget->Update();
    if (mShareManager != nullptr)
        mShareManager->Update();
}

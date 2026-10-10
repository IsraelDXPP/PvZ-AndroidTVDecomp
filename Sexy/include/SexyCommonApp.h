// SexyCommonApp is the TV application above SexyAppBase. It owns the platform
// services (game centre, share, notifications, ads, auth) and the command-line
// account fields LawnApp reads back.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SEXYCOMMONAPP_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SEXYCOMMONAPP_H

#include "ReconTypes.h"
#include "SoundControllerWidget.h"
#include "AuthManager.h"
#include "Buffer.h"
#include "DriverFactory.h"
#include "InputDriverFactory.h"
#include "InputManager.h"
#include "InputDeviceListener.h"
#include "SexyAppBase.h"

#include <vector>

namespace Sexy
{

class IGameCenter;
class IShareManager;
class INotificationManager;
class AdProvider;
class PaymentManager;
class PlatformIntegration;
class TrialTimeCheck;
class SoundControllerWidget;

class ConnectionManager
{
public:
    ConnectionManager();
    ~ConnectionManager();
    int Update(int theFrame);

private:
    void *mMulti;
};

class SPayGameInfo;

class SexyCommonApp : public SexyAppBase
{
public:
    virtual ~SexyCommonApp();
    virtual void UpdateFrames();
    virtual void AppEnteredBackground();
    virtual void AppEnteredForeground();
    virtual void HandleCmdLineParam(const Sexy::String &theName, const Sexy::String &theValue);
    virtual void Init();
    virtual void Cleanup();

    ConnectionManager mConnectionManager;
    PlatformIntegration *mPlatformIntegration;
    std::uint64_t mGamePaidQueryStartTick;
    PaymentManager *mPaymentManager;
    TrialTimeCheck *mTrialTimeCheck;
    AdProvider *mAdProvider;
    InputDeviceListener *mUpdateRemoteListener;
    SoundControllerWidget *mSoundControllerWidget;
    IGameCenter *mGameCenter;
    IShareManager *mShareManager;
    INotificationManager *mNotificationManager;
    bool mAuthenticated;
    int mAuthId;
    int mMarketOrAuthMode;
    Sexy::String mServer;
    Sexy::String mSPay;
    Sexy::String mUserId;
    Sexy::String mStbId;
    Sexy::String mStbIdDerived;
    Sexy::String mLoginUser;
    Sexy::String mLoginPassword;
    Sexy::String mLoginFlag;
    Sexy::String mAppSessionId;
    Sexy::String mGlobalAreaCode;
    int mTestAdRecallMode;
    bool mTestAdRecallResult;
    int mTestAdRecallCounter;
    Sexy::String mTestAdPayItemId;
    int mTestAdPayItemCount;
    Sexy::String mTestAdPayChannel;
    bool mTrialCheckStarted;
    bool mTrialCheckDelayPassed;

    void setErrorCode();
    void showRechargeBoard();
    static int FullVersionHandle(bool theFull, void *theApp);
    static int MessageBoxHandle(int theResult, void *theApp);
    void DoGameTrialDialog(const SPayGameInfo &theInfo);
    void CreateGameTrialDialog();
    void CreateGotoBuyResultDialog();
    void CreateWaitResultDialog();
    void CreateWaitMessageDialog();
    void CreateNetworkFaultDialog();
    int setOfferRes(bool theResult);
    int setOfferFullOpenRes(bool theResult);
    int setFullVersion(bool theFull);
    int setPayItemsRes(bool theResult, const char *theItemId, int theCount, const char *theChannel);
    int setQueryBalanceRes(bool theResult, int theBalance);
    int adBarClick();
    int setLoginRes(bool theResult, const char *theUser);
    int messageBoxRes(int theResult);
    int DoAuthenticate();
    SexyCommonApp();
    IGameCenter *GetGameCenter();
    IShareManager *GetShareManager();
    INotificationManager *GetNotificationManager();
    PlatformIntegration *GetPlatformIntegration();
    int GotoBuyGame(int theProduct);
    int NotGoToBuy();
    bool IsGamePaid();
    int GamePaidStatus();
    int setTestAdRecall(SexyCommonApp_E_TESTAD_MODE theMode, bool theResult);
    void setTestAdPayItemsRecall(bool theResult, Sexy::String theItemId, int theCount, Sexy::String theChannel);
    int updateTestRecall();
    int showMoreGame();
    void showRecommendGames();
    int goToMarket();
    int postData(Sexy::String theKind, Sexy::String theBody);
    void postData(SexyCommonApp_POST_TYPE theKind, Sexy::String theBody);
    int postXMLFileData(Sexy::String thePath);
    int showMessageBox(
        Sexy::String theText, Sexy::String theTitle, SexyCommonApp_E_MESSAGEBOX_TYPE theType, int theFlags);
    Sexy::String getUserIdentifier();
    Sexy::String getUserName();
    Sexy::String getSecretKey();
    Sexy::String getRechargeLimit();
    int IsRemoteControllerEnabled();
    Sexy::String getCurrentRemoteName();

    struct GameInfoRecord
    {
        Sexy::String mPlatform;
        Sexy::String mVendor;
        Sexy::String mProduct;
        Sexy::String mGameName;
        Sexy::String mGameVersion;
    };
    GameInfoRecord getGameInfo();
    void CreateTrialDialog();
    static int OfferResHandle(bool theResult, void *theApp);
    static int OfferFullOpenResHandle(bool theResult, void *theApp);
    static int PayItemsResHandle(
        bool theResult, const char *theItemId, int theCount, const char *theChannel, void *theApp);
    static int QueryBalanceResHandle(bool theResult, int theBalance, void *theApp);
    static int AdBarClickHandle(void *theApp);
    static int LoginResHandle(bool theResult, const char *theUser, void *theApp);
};

} // namespace Sexy

using Sexy::SexyCommonApp;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SEXYCOMMONAPP_H

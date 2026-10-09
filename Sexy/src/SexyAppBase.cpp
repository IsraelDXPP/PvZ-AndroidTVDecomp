// SexyAppBase -- the application object.
//
// Written from what the class is for. It owns the window size, the volumes,
// the property tables, the dialogs, the images and the platform drivers, and
// every method below does that one job. Strings are Sexy::String, so a literal
// is copied into a buffer the string owns; nothing here frees a literal.

#include "SexyAppBase.h"
#include "ReconData.h"
#include "ReconHelpers.h"
#include "Platform/SDLAppDriver.h"
#include "Platform/CrashDiagnostics.h"
#include "AuthManager.h"
#include "BaseAppDriver.h"
#include "Buffer.h"
#include "CritSect.h"
#include "DeviceImage.h"
#include "Dialog.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageLib.h"
#include "InputConnectManager.h"
#include "InputManager.h"
#include "InputDriverFactory.h"
#include "LogManager.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "NativeDisplay.h"
#include "PixelUtil.h"
#include "ResourceManager.h"
#include "ResourceRef.h"
#include "ServiceManager.h"
#include "SharedRenderTarget.h"
#include "SharedImage.h"
#include "SharedImageRef.h"
#include "SoundManager.h"
#include "Widget.h"
#include "WidgetManager.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>

namespace
{

// One shared-image record, keyed by the upper-cased name and file.
struct SharedImageKey
{
    Sexy::String mName;
    Sexy::String mFile;
};

bool operator<(const SharedImageKey& aLeft, const SharedImageKey& aRight)
{
    int aCompare = aLeft.mName.compare(aRight.mName);
    if (aCompare != 0)
        return aCompare < 0;
    return aLeft.mFile.compare(aRight.mFile) < 0;
}

static Sexy::String UpperCopy(const Sexy::String& theText)
{
    Sexy::String aCopy = theText;
    for (unsigned int i = 0; i < aCopy.size(); ++i)
        aCopy[i] = (char)std::toupper((unsigned char)aCopy[i]);
    return aCopy;
}

// A property key that is nothing but an integer (optional '-', decimal, or
// 0x hex). SetString records those in mPopLoc as well as the string table.
bool PropertyKeyIsInteger(const Sexy::String& theKey, int* theValue)
{
    const char* aText = theKey.c_str();
    unsigned int aLength = theKey.size();
    *theValue = 0;
    if (aLength == 0)
        return false;

    unsigned int aIndex = 0;
    bool aNegative = aText[0] == '-';
    if (aNegative)
        ++aIndex;
    if (aIndex >= aLength)
        return false;

    int aBase = 10;
    if (aText[aIndex] == '0' && aIndex + 1 < aLength
        && (aText[aIndex + 1] == 'x' || aText[aIndex + 1] == 'X'))
    {
        aIndex += 2;
        aBase = 16;
        if (aIndex >= aLength)
            return false;
    }

    unsigned int aNumber = 0;
    for (; aIndex < aLength; ++aIndex)
    {
        unsigned char aChar = (unsigned char)aText[aIndex];
        int aDigit = -1;
        if (aChar >= '0' && aChar <= '9')
            aDigit = aChar - '0';
        else if (aChar >= 'a' && aChar <= 'f')
            aDigit = aChar - 'a' + 10;
        else if (aChar >= 'A' && aChar <= 'F')
            aDigit = aChar - 'A' + 10;
        if (aDigit < 0 || aDigit >= aBase)
            return false;
        aNumber = aNumber * (unsigned int)aBase + (unsigned int)aDigit;
    }
    *theValue = aNegative ? -(int)aNumber : (int)aNumber;
    return true;
}

Sexy::Thread CurrentThread()
{
    return *Sexy::Thread::Self();
}

// A .tex file: "SEXYTEX", then a 40-byte header, then the pixels. Bit 0 of
// the flags means the pixels are deflated.
struct TexHeader
{
    int mSignature;
    int mWidth;
    int mHeight;
    int mFormat;
    int mFlags;
    int mReserved0;
    int mDataSize;
    int mReserved1;
    int mReserved2[2];
};

const int kBytesPerPixel[] = {0, 1, 4, 2, 2, 2, 4, 2, 2, 4, 2};

bool NameContains(const Sexy::String& theName, const char* theNeedle)
{
    const unsigned int aCount = 4;
    if (aCount > theName.size())
        return false;
    const char* aData = theName.c_str();
    unsigned int aStart = theName.size() - aCount;
    for (;;)
    {
        if (std::memcmp(aData + aStart, theNeedle, aCount) == 0)
            return true;
        if (aStart == 0)
            return false;
        --aStart;
    }
}

static unsigned char ClampByte(int theValue)
{
    if (theValue < 0)
        return 0;
    if (theValue > 255)
        return 255;
    return (unsigned char)theValue;
}

// HSL in 0..255, packed back as 0x00RRGGBB.
unsigned int HslToRgb(int theHue, int theSaturation, int theLightness)
{
    theHue = ((theHue % 256) + 256) % 256;
    int aSat = theSaturation;
    int aLight = theLightness;
    if (aSat < 0) aSat = 0;
    if (aSat > 255) aSat = 255;
    if (aLight < 0) aLight = 0;
    if (aLight > 255) aLight = 255;

    if (aSat == 0)
        return (unsigned int)((aLight << 16) | (aLight << 8) | aLight);

    int aSector = (theHue * 6) / 256;
    int aFraction = (theHue * 6) % 256;
    int aMax = aLight + ((255 - aLight) * aSat) / 255;
    int aMin = aLight - (aLight * aSat) / 255;
    int aRising = aMin + ((aMax - aMin) * aFraction) / 256;
    int aFalling = aMax - ((aMax - aMin) * aFraction) / 256;

    int aRed = aLight;
    int aGreen = aLight;
    int aBlue = aLight;
    switch (aSector % 6)
    {
        case 0: aRed = aMax; aGreen = aRising; aBlue = aMin; break;
        case 1: aRed = aFalling; aGreen = aMax; aBlue = aMin; break;
        case 2: aRed = aMin; aGreen = aMax; aBlue = aRising; break;
        case 3: aRed = aMin; aGreen = aFalling; aBlue = aMax; break;
        case 4: aRed = aRising; aGreen = aMin; aBlue = aMax; break;
        default: aRed = aMax; aGreen = aMin; aBlue = aFalling; break;
    }
    return (unsigned int)((ClampByte(aRed) << 16) | (ClampByte(aGreen) << 8) | ClampByte(aBlue));
}

} // namespace

namespace Sexy
{

IFileDriver* gFileDriver = nullptr;

// The loading overlay. It only has to exist so the widget manager can hold it.
// Anonymous so it does not collide with the framework's PreLoadWidget symbol.
namespace
{
class PreLoadWidget : public Widget
{
public:
    explicit PreLoadWidget(SexyAppBase* theApp)
    {
        (void)theApp;
    }
};
}

BaseAppDriver* BaseAppDriver::CreateAppDriver(SexyAppBase* theApp)
{
    return Lawn_CreateAppDriver(theApp);
}

IAppDriver* IAppDriver::CreateAppDriver(SexyAppBase* theApp)
{
    return Lawn_CreateAppDriver(theApp);
}

IResStreamsDriver* IResStreamsDriver::CreateResStreamsDriver()
{
    return nullptr;
}

IAudioDriver* IAudioDriver::CreateAudioDriver(SexyAppBase* theApp)
{
    return Lawn_CreateAudioDriver(theApp);
}

IHttpDriver* IHttpDriver::CreateHttpDriver(SexyAppBase* theApp)
{
    return Lawn_CreateHttpDriver(theApp);
}

// --------------------------------------------------------------------------
// Construction. Strings, maps, lists and the demo buffer construct themselves.
// What remains is the values the original constructor chose, then the drivers.
// --------------------------------------------------------------------------
SexyAppBase::SexyAppBase()
{
    gSexyAppBase = this;

    mRandSeed = 0;
    mInitialized = false;
    mOnlyAllowOneCopyToRun = true;

    mResourceManifestPath = "properties/resources.xml";
    mDefaultPropertiesPath = "properties/default.xml";
    mProdName = "Product";
    mTitle = "SexyApp";
    mDemoPrefix = "sexyapp";
    mDemoFileName = "sexyapp.dmo";

    mNoDefer = false;
    mFullScreenPageFlip = true;
    mTimeLoaded = Sexy::GetTickCount();
    mSEHOccured = false;

    mWidth = 640;
    mHeight = 480;
    mFullscreenBits = 16;
    mPreferredX = -1;
    mPreferredY = -1;
    mPreferredX2 = -1;
    mPreferredY2 = -1;
    mLayoutWidth = 0;
    mLayoutHeight = 0;
    mShutDown = false;
    mExitToTop = false;
    mCloseRequested = false;
    mWindowed = false;
    mIsScreenSaver = false;

    mPlatformWindow1 = nullptr;
    mPlatformWindow2 = nullptr;
    mPlatformDriverOrQueue = nullptr;
    mFileDriver = nullptr;
    mAppDriver = nullptr;
    mDDInterface = nullptr;
    mAudioDriver = nullptr;
    mResStreamsDriver = nullptr;
    mHttpDriver = nullptr;
    mMusicInterface = nullptr;

    mNonDrawCount = 0;
    mUpdateCount = 0;
    mDrawCount = 0;
    mUpdateAppState = 0;
    mUpdateAppDepth = 0;
    mUpdateDepth = 0;
    mAllowMonitorPowersave = true;
    mFrameTime = 10;
    mPendingUpdatesAcc = 0.0;
    mUpdateFTimeAcc = 0.0;
    mHasPendingDraw = false;
    mLastDrawWasEmpty = false;
    mIsDrawing = false;
    mSleepCount = 0;
    mUpdateMultiplier = 1.0;
    mUpdateFrameTime = 10;
    mPaused = false;
    mFastForwardToUpdateNum = 0;
    mFastForwardToMarker = false;
    mFastForwardStep = false;

    mSoundManager = nullptr;
    mCursorState = 0;
    mMouseIn = false;
    mRunning = false;
    mActive = true;
    mMinimized = false;
    mPhysMinimized = false;
    mIsDisabled = false;
    mLoaded = false;
    mYieldMainThread = false;
    mLoadingFailed = false;
    mAutoStartLoadingThread = true;
    mLoadingThreadStarted = false;
    mLoadingThreadCompleted = false;
    mCursorThreadRunning = false;
    mNumLoadingThreadTasks = 0;
    mCompletedLoadingThreadTasks = 0;

    mLastDrawTick = Sexy::GetTickCount();
    mNextDrawTick = mLastDrawTick;
    mMusicVolume = 0.85;
    mSfxVolume = 0.85;
    mCustomCursorsEnabled = false;
    mCustomCursorDirty = false;
    mDraggingCursor = nullptr;
    mPointerCursor = nullptr;
    mHandCursor = nullptr;
    mIsOpeningURL = false;
    mReadFromRegistry = false;
    mCmdLineParsed = false;
    mSkipSignatureChecks = false;
    mCtrlDown = false;
    mAltDown = false;
    mStepMode = 0;
    mCleanupSharedImages = false;
    mEnableMaximizeButton = false;
    mMuteCount = 0;
    mAutoMuteCount = 0;
    mDemoMute = false;
    mSysCursor = true;
    mHasFocus = true;
    mLastShutdownWasGraceful = true;
    mDemoSfxVolume = 0.0;
    mDemoMusicVolume = 0.0;
    mStandardWordWrap = true;
    mAllowExtendedChars = true;
    mMuteOnLostFocus = true;
    mCurHandleNum = 0;
    mMouseX = 0;
    mMouseY = 0;

    // The desktop loop advances one game tick per frame, including on displays
    // faster than 60 Hz. Keep the TV animation rate unless explicitly overridden.
    mMaxFPS = Sexy::GetEnvIntOption("SEXY_MAX_FPS", 60);
    mFPSTime = 0;
    mFPSStartTick = Sexy::GetTickCount();
    mFPSFlipCount = 0;
    mFPSCount = 0;
    mFPSDirtyCount = 0;
    mShowFPS = false;
    mShowFPSMode = 0;
    mScreenBltTime = 0;
    for (int i = 0; i < 8; ++i)
        mPerfCounters[i] = 0;
    mSyncRefreshRate = 100;
    mMinVidMemory3D = 6;
    mDrawTime = 0;
    mAlphaDisabled = false;
    mDebugKeysEnabled = false;
    mNoSoundNeeded = false;
    mWantFMod = false;
    mVSyncUpdates = false;
    mVSyncBroken = false;
    mVSyncBrokenCount = 0;
    mVSyncBrokenTestStartTick = 0;
    mVSyncBrokenTestUpdates = 0;
    mWaitForVSync = false;
    mUserChanged3DSetting = false;
    mAutoEnable3D = false;
    mTest3D = false;
    mSavedIs3D = false;
    mWriteToCache = true;
    mSoftVSyncWait = true;
    mIs3DAccelerated = true;
    mRecommendedVidMemory3D = 14;
    mRelaxUpdateBacklogCount = 0;
    mComposeToScreen = true;
    mWidescreenAware = false;
    mEnableWindowAspect = false;
    mWindowAspect.Set(4, 3);
    mPresentationAspect.Set(4, 3);
    mWideScreenAspect.Set(16, 10);
    mScreenHeight = 600;
    mScreenWidth = 800;
    mIsWideWindow = false;
    mDisplayOptions[0] = false;
    mKeepCompat = true;
    mDisplayOptions[1] = true;
    mDisplayOptions[2] = true;

    mPreLoadWidget = nullptr;
    mThreadedPreload = true;
    mPreLoadStarted = false;
    mSafeReload = Sexy::GetEnvOption("SEXY_SAFE_RELOAD", false) != 0;
    mLoadLocale = true;
    mCacheFonts = true;
    mDeferLoad = false;
    if (Sexy::GetEnvOption("SEXY_NO_THREADED_PRELOAD", false) != 0)
        mThreadedPreload = false;

    mLawnMouseMode = true;
    for (int aCursor = 0; aCursor < 13; ++aCursor)
        mCursorImages[aCursor] = nullptr;

    for (int i = 0; i < 256; ++i)
        mAdd8BitMaxTable[i] = (unsigned char)i;
    for (int i = 256; i < 512; ++i)
        mAdd8BitMaxTable[i] = 0xFF;

    mMusicPercent = 100.0f;
    mSfxPercent = 100.0f;
    mMasterPercent = 100;

    mPlayingDemoBuffer = false;
    mManualShutdown = false;
    mRecordingDemoBuffer = false;
    mLastDemoMouseX = 0;
    mLastDemoMouseY = 0;
    mLastDemoUpdateCnt = 0;
    mDemoLoadingComplete = false;
    mDemoLength = 0;
    mDemoCmdNum = 0;
    mDemoCmdBitPos = 0;
    mCursorNum = 0;
    mPreLoadFailed = false;
    mDemoNeedsCommand = true;
    mDemoCmdOrder = -1;

    mPointerHotX = 0;
    mPointerHotY = 0;
    mDragCursor = nullptr;
    mDragHotX = 0;
    mDragHotY = 0;
    mCursorHotY = 0;

    mRenderTargetPool = nullptr;
    mOwnedService = nullptr;
    mOwnedServiceExtra = nullptr;
    mAuthManager = nullptr;
    mWidgetManager = nullptr;
    mResourceManager = nullptr;
    mInputManager = nullptr;
    mInputConnectManager = nullptr;

    std::memset(&mParameterStorage, 0, sizeof(mParameterStorage));
    mParameterStorage.eventCode = -1;
    mRuntimeFlags[0] = false;
    mRuntimeFlags[1] = false;

    Sexy::SWTri_AddAllDrawTriFuncs(nullptr);

    Sexy::String aResourceFolder;
    Sexy::GetResourcesFolder(&aResourceFolder);
    if (!aResourceFolder.empty()
        && aResourceFolder.compare(".") != 0
        && aResourceFolder.compare("./") != 0)
    {
        p_addResource(aResourceFolder.c_str(), "native", 0);
    }

    InitFileDriver();

    mAppDriver = BaseAppDriver::CreateAppDriver(this);

    mResStreamsDriver = IResStreamsDriver::CreateResStreamsDriver();
    mAudioDriver = IAudioDriver::CreateAudioDriver(this);
    mHttpDriver = IHttpDriver::CreateHttpDriver(this);

    mAuthManager = new AuthManager(this);
    mWidgetManager = new WidgetManager(this);
    mResourceManager = new ResourceManager(this);
    mInputManager = new InputManager(this, 0x400u);
    mInputConnectManager = new InputConnectManager(false);
    mInputConnectManager->Start(mInputManager);
    mInputConnectManager->AddListener(static_cast<InputConnectListener*>(this));

    mPrimaryThreadId = 0;
    Sexy::Thread aThread = CurrentThread();
    mPrimaryThread.mThread = aThread.mThread;
    mPrimaryThread.mValid = aThread.mValid;
    mTabletPC = false;

    if (mAppDriver != nullptr)
        mAppDriver->InitAppDriver();
    if (gFileDriver != nullptr)
        gFileDriver->InitAppDriver(this);
    if (mAudioDriver != nullptr)
        mAudioDriver->InitAppDriver();
}

SexyAppBase::~SexyAppBase()
{
    Cleanup();

    LogManager* aLog = static_cast<LogManager*>(LogManager::getInstance());
    if (aLog != nullptr)
        aLog->close();

    for (Dialog* aDialog : mDialogList)
        delete aDialog;
    mDialogList.clear();
    mDialogMap.clear();

    for (Widget* aWidget : mSafeDeleteList)
        delete aWidget;
    mSafeDeleteList.clear();
}

// --------------------------------------------------------------------------
// The empty hooks. A derived app overrides the ones it cares about.
// --------------------------------------------------------------------------
void SexyAppBase::OnFullVersionChange() {}
void SexyAppBase::AppEnteredBackground() {}
void SexyAppBase::AppEnteredForeground() {}
void SexyAppBase::CleanupHook() {}
bool SexyAppBase::AppCanRestore() { return true; }
void SexyAppBase::ModalOpen() {}
void SexyAppBase::ModalClose() {}
void SexyAppBase::GotFocus() {}
void SexyAppBase::LostFocus() {}
void SexyAppBase::URLOpenFailed(const Sexy::String&) {}
void SexyAppBase::URLOpenSucceeded(const Sexy::String&) {}
void SexyAppBase::SEHOccured() { mSEHOccured = true; }
void SexyAppBase::ShutdownHook() {}
void SexyAppBase::PreDrawScreen() {}
void SexyAppBase::BeginPopup() {}
void SexyAppBase::EndPopup() {}
void SexyAppBase::HandleNotifyGameMessage(int, int) {}
void SexyAppBase::CloseRequestAsync() { mCloseRequested = true; }
void SexyAppBase::Done3dTesting() {}
void SexyAppBase::MakeWindow() {}
void SexyAppBase::LoadingThreadProc() {}
void SexyAppBase::LoadingThreadCompleted() {}
void SexyAppBase::PreTerminate() {}
void SexyAppBase::Terminate() { if (mAppDriver != nullptr) mAppDriver->Terminate(); }
void SexyAppBase::PreDisplayHook() {}
void SexyAppBase::PreDDInterfaceInitHook() {}
void SexyAppBase::PostDDInterfaceInitHook() {}
bool SexyAppBase::ChangeDirHook(const char*) { return false; }
void SexyAppBase::InitPropertiesHook() {}
void SexyAppBase::InitHook() {}
void SexyAppBase::InputStatusChanged(void*) {}
void SexyAppBase::PreLoadResources() {}
void SexyAppBase::PostPreLoad() {}
void SexyAppBase::StartSensor(int) {}
void SexyAppBase::StopSensor(int) {}
bool SexyAppBase::IsUIOrientationAllowed(int) { return true; }
void SexyAppBase::UIOrientationChanged(int) {}
int SexyAppBase::GetUIOrientation() { return 0; }
void SexyAppBase::LowMemoryWarning() {}
bool SexyAppBase::FrameNeedsSwapScreenImage() { return false; }
void SexyAppBase::GetMemoryInfo(int& theMemoryInfo)
{
    if (mAppDriver != nullptr)
        mAppDriver->GetMemoryInfo(&theMemoryInfo);
}
Sexy::String SexyAppBase::NotifyCrashHook() { return Sexy::String(); }
void SexyAppBase::LogScreenSaverError(const Sexy::String&) {}
void SexyAppBase::ReadDemoBuffer(Sexy::String&) {}
void SexyAppBase::WriteDemoBuffer() {}
void SexyAppBase::DemoSyncBuffer(Sexy::Buffer*) {}
void SexyAppBase::DemoSyncString(Sexy::String*) {}
void SexyAppBase::DemoSyncInt(int*) {}
void SexyAppBase::DemoSyncBool(bool*) {}
void SexyAppBase::DemoAssertStringEqual(const Sexy::String&) {}
void SexyAppBase::DemoAddMarker(const Sexy::String& theMarker) { mDemoMarkers.push_back(theMarker); }
void SexyAppBase::DemoAssertIntEqual(int) {}
void SexyAppBase::DumpProgramInfo() {}
void SexyAppBase::RestoreScreenResolution() {}
void SexyAppBase::GetSEHWebParams(void*) {}
void SexyAppBase::OpenRegisterPageWithParameters(void*) {}
void SexyAppBase::OpenRegisterPage() {}
void SexyAppBase::PrecacheAdditive(Sexy::MemoryImage*) {}
void SexyAppBase::PrecacheAlpha(Sexy::MemoryImage*) {}
void SexyAppBase::PrecacheNative(Sexy::MemoryImage*) {}
void SexyAppBase::DemoSyncRefreshRate() {}
void SexyAppBase::AuthFinished(bool) {}

// --------------------------------------------------------------------------
// Dialogs.
// --------------------------------------------------------------------------
Dialog* SexyAppBase::NewDialog(int theDialogId, bool isModal, const Sexy::String& theHeader,
                               const Sexy::String& theLines, const Sexy::String& theFooter,
                               int theButtonMode)
{
    return new Dialog(nullptr, nullptr, theDialogId, isModal, theHeader, theLines,
                      theFooter, theButtonMode);
}

Dialog* SexyAppBase::DoDialog(int theDialogId, bool isModal, const Sexy::String& theHeader,
                              const Sexy::String& theLines, const Sexy::String& theFooter,
                              int theButtonMode)
{
    KillDialog(theDialogId);
    Dialog* aDialog = NewDialog(theDialogId, isModal, theHeader, theLines, theFooter, theButtonMode);
    AddDialogWithId(theDialogId, aDialog);
    return aDialog;
}

void SexyAppBase::AddDialogWithId(int theDialogId, Dialog* theDialog)
{
    if (theDialog == nullptr)
        return;

    BeginPopup();
    mDialogMap[theDialogId] = theDialog;
    mDialogList.push_back(theDialog);

    if (mWidgetManager != nullptr)
    {
        mWidgetManager->AddWidget(theDialog);
        if (theDialog->IsModal())
        {
            mWidgetManager->AddBaseModal(theDialog);
            ModalOpen();
            mWidgetManager->SetFocus(theDialog);
        }
    }
}

void SexyAppBase::AddDialog(Dialog* theDialog)
{
    if (theDialog != nullptr)
        AddDialogWithId(theDialog->mId, theDialog);
}

Dialog* SexyAppBase::GetDialog(int theDialogId)
{
    std::map<int, Dialog*>::iterator aFound = mDialogMap.find(theDialogId);
    if (aFound == mDialogMap.end())
        return nullptr;
    return aFound->second;
}

int SexyAppBase::GetDialogCount()
{
    return (int)mDialogMap.size();
}

bool SexyAppBase::KillDialogFull(int theDialogId, bool removeWidget, bool deleteWidget)
{
    std::map<int, Dialog*>::iterator aFound = mDialogMap.find(theDialogId);
    if (aFound == mDialogMap.end())
        return false;

    Dialog* aDialog = aFound->second;
    mDialogMap.erase(aFound);
    mDialogList.remove(aDialog);

    if (removeWidget && mWidgetManager != nullptr && aDialog != nullptr)
        mWidgetManager->RemoveWidget(aDialog);
    if (aDialog != nullptr && aDialog->IsModal())
    {
        ModalClose();
        if (mWidgetManager != nullptr)
            mWidgetManager->RemoveBaseModal(aDialog);
    }
    if (deleteWidget)
        SafeDeleteWidget(aDialog);
    EndPopup();
    return true;
}

bool SexyAppBase::KillDialogById(int theDialogId)
{
    return KillDialogFull(theDialogId, true, true);
}

bool SexyAppBase::KillDialogByPointer(Dialog* theDialog)
{
    if (theDialog == nullptr)
        return false;
    return KillDialogById(theDialog->mId);
}

bool SexyAppBase::KillDialog(int theDialogId)
{
    return KillDialogFull(theDialogId, true, true);
}

bool SexyAppBase::KillDialog(Dialog* theDialog)
{
    return KillDialogByPointer(theDialog);
}

void SexyAppBase::DialogButtonPress(int, int) {}
void SexyAppBase::DialogButtonDepress(int theDialogId, int theButtonId)
{
    // @0x336A20 forwards the result to the application's button handler.
    if (theButtonId == 1000)
        ButtonDepress(theDialogId + 2000);
    else if (theButtonId == 1001)
        ButtonDepress(theDialogId + 3000);
}

// --------------------------------------------------------------------------
// Messages, popups, focus.
// --------------------------------------------------------------------------
int SexyAppBase::MsgBox(const Sexy::String& theText, const Sexy::String& theTitle, int theFlags)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->MsgBox(theText, theTitle, theFlags);
}

int SexyAppBase::MsgBoxIntString(const Sexy::String& theText, const Sexy::String& theTitle, int theFlags)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->MsgBoxIntString(theText, theTitle, theFlags);
}

void SexyAppBase::Popup(const Sexy::String& theText)
{
    if (mAppDriver != nullptr)
        mAppDriver->Popup(theText);
}

void SexyAppBase::PopupIntString(const Sexy::String& theText)
{
    if (mAppDriver != nullptr)
        mAppDriver->PopupIntString(theText);
}

bool SexyAppBase::IsAltKeyUsed(int) { return false; }

bool SexyAppBase::KeyDown(int theKeyCode)
{
    if (mAppDriver == nullptr)
        return false;
    return mAppDriver->KeyDown(theKeyCode) != 0;
}

bool SexyAppBase::DebugKeyDown(int theKeyCode)
{
    if (mAppDriver == nullptr)
        return false;
    return mAppDriver->DebugKeyDown(theKeyCode) != 0;
}

bool SexyAppBase::DebugKeyDownAsync(int theKeyCode, bool controlDown, bool altDown)
{
    if (mAppDriver == nullptr)
        return false;
    return mAppDriver->DebugKeyDownAsync(theKeyCode, controlDown, altDown) != 0;
}

void SexyAppBase::ClearKeysDown()
{
    if (mWidgetManager != nullptr)
    {
        for (int aKey = 0; aKey < 255; ++aKey)
        {
            KeyState* aNode = mWidgetManager->mKeyDown.Get(aKey);
            if (aNode != nullptr)
                aNode->mDown = false;
        }
    }
    mCtrlDown = false;
    mAltDown = false;
}

bool SexyAppBase::ProcessMessage(Sexy::Event& theEvent)
{
    switch (theEvent.modifiers)
    {
        case Event::MOUSE_DOWN:
        case Event::MOUSE_UP:
        case Event::MOUSE_MOVE:
        {
            mLastUserInputTick = mLastTimerTime;
            if (mWidgetManager == nullptr)
                return true;
            int mouseX = int(theEvent.eventCodeMask);
            int mouseY = theEvent.matchEvent;
            mWidgetManager->RemapMouse(mouseX, mouseY);
            if (theEvent.modifiers == Event::MOUSE_DOWN)
                mWidgetManager->MouseDown(mouseX, mouseY, theEvent.standard);
            else if (theEvent.modifiers == Event::MOUSE_UP)
                mWidgetManager->MouseUp(mouseX, mouseY, theEvent.standard);
            else
                mWidgetManager->MouseMove(mouseX, mouseY);
            return true;
        }
        case Event::MOUSE_WHEEL:
            mLastUserInputTick = mLastTimerTime;
            if (mWidgetManager != nullptr)
                mWidgetManager->MouseWheel(int(theEvent.eventCodeMask), theEvent.matchEvent);
            return true;
        case Event::MOUSE_EXIT:
        case Event::MOUSE_CANCEL:
            if (mWidgetManager != nullptr)
            {
                mWidgetManager->MouseExit(0, 0);
                if (theEvent.modifiers == Event::MOUSE_CANCEL)
                {
                    for (int clickCount : {1, -1, 3})
                        mWidgetManager->MouseUp(-10000, -10000, clickCount);
                }
            }
            return true;
        case Sexy::Event::CHARACTER_INPUT:
            mLastUserInputTick = mLastTimerTime;
            if (mWidgetManager != nullptr)
                mWidgetManager->KeyUnicode((int)theEvent.eventCodeMask);
            return true;
        case 1:
        {
            mLastUserInputTick = mLastTimerTime;
            if (mWidgetManager != nullptr)
                mWidgetManager->KeyDown(theEvent);
            return true;
        }
        case 2:
            mLastUserInputTick = mLastTimerTime;
            if (mWidgetManager != nullptr)
                mWidgetManager->KeyUp(theEvent);
            return true;
        default:
            return true;
    }
}

bool SexyAppBase::ProcessMessages(std::vector<Sexy::Event>& theEvents)
{
    if (theEvents.empty())
        return true;

    if (theEvents.front().modifiers == 16)
    {
        mLastUserInputTick = mLastTimerTime;
        for (Sexy::Event& anEvent : theEvents)
        {
            if (mWidgetManager != nullptr)
            {
                int aX = (int)anEvent.mScaleDivisorX;
                int aY = (int)anEvent.mScaleDivisorY;
                mWidgetManager->RemapMouse(aX, aY);
            }
        }
        if (mWidgetManager == nullptr)
            return true;

        switch ((int)theEvents.front().matchEvent)
        {
            case 1: return true;
            case 2: return true;
            case 3: return true;
            default: return true;
        }
    }

    if (theEvents.size() != 1)
        return true;
    return ProcessMessage(theEvents.front());
}

void SexyAppBase::PopulateMessages()
{
    if (mInputManager == nullptr)
        return;

    mInputManager->Update();

    Sexy::Event anEvent;
    std::memset(&anEvent, 0, sizeof(anEvent));
    anEvent.eventCode = -1;
    if (mInputManager->PopEvent(anEvent) == 0)
        return;
    mQueuedEvents.push_back(anEvent);
}

// --------------------------------------------------------------------------
// Lifetime: startup, the frame, shutdown.
// --------------------------------------------------------------------------
void SexyAppBase::Startup()
{
    if (mAppDriver != nullptr)
        mAppDriver->Startup();
}

void SexyAppBase::Start()
{
    if (mAppDriver != nullptr)
        mAppDriver->Start();
}

void SexyAppBase::Init()
{
// #ifndef NDEBUG
    InstallCrashDiagnostics();
// #endif
    if (mInputManager != nullptr)
    {
        Sexy::InputDriverFactory::GetInputDriverFactory()->Load();
        mInputManager->Init();
    }

    LogManager* aLogManager = static_cast<LogManager*>(LogManager::getInstance());
    if (aLogManager != nullptr)
    {
        Sexy::String aName = aLogManager->getName();
        if (aName.compare("default") == 0)
        {
            Sexy::String aLower;
            StringToLower(&aLower, mProdName.c_str());
            aLogManager->setName(aLower);
        }
    }

    Sexy::ServiceManager::setName(Sexy::ServiceManager::getInstance(nullptr), mProdName);

    if (mShutDown)
        return;

    if (mWidgetManager != nullptr)
        mWidgetManager->mClip = !mWidescreenAware;

    if (mAppDriver != nullptr && mAppDriver->Init() == 0)
        mShutDown = true;

    if (mMusicInterface != nullptr)
        mMusicInterface->SetVolume(mMusicVolume);
    if (mSoundManager != nullptr)
        mSoundManager->SetVolume(mSfxVolume);
    if (mPlatformDriverOrQueue != nullptr)
        mPlatformDriverOrQueue->Update();

    InitHook();
    mInitialized = true;
}

bool SexyAppBase::UpdateAppStep(bool* updated)
{
    if (mAppDriver == nullptr)
        return false;
    return mAppDriver->UpdateAppStep(updated) != 0;
}

bool SexyAppBase::UpdateApp()
{
    bool anUpdated = false;
    bool aResult = true;
    for (;;)
    {
        aResult = UpdateAppStep(&anUpdated);
        if (!aResult || anUpdated)
            break;
    }

    if (mExitToTop && !mShutDown && mUpdateDepth == 0)
    {
        mExitToTop = false;
        Shutdown();
    }
    return aResult;
}

void SexyAppBase::DrawOneFrame()
{
    const int aMaxIterations = 4;
    int aIteration = 0;
    int anUpdateCount = 0;
    do
    {
        if (mShutDown)
            return;
        anUpdateCount = mUpdateCount;
        mCloseRequested = false;
        UpdateApp();
    }
    while (mUpdateCount == anUpdateCount && ++aIteration < aMaxIterations);
}

void SexyAppBase::UpdateFrames()
{
    ++mUpdateCount;
    if (!mMinimized && mWidgetManager != nullptr && mWidgetManager->UpdateFrame() != 0)
        ++mFPSDirtyCount;

    if (mMusicInterface != nullptr)
        mMusicInterface->Update();
    if (mSoundManager != nullptr)
        mSoundManager->Update();
    if (mInputConnectManager != nullptr)
        mInputConnectManager->Update();
    if (mHttpDriver != nullptr)
        mHttpDriver->Update();

    if (mLoaded && mAuthManager != nullptr && mAuthManager->GetAuthState() == 2)
    {
        AuthFinished(mAuthManager->IsAuthenticaled() != 0);
        mAuthManager->Handled();
    }

    CleanSharedImages();
}

bool SexyAppBase::DrawDirtyStuff()
{
    if (mAppDriver == nullptr)
        return false;
    return mAppDriver->DrawDirtyStuff(false) != 0;
}

void SexyAppBase::Redraw(TRect<int>* theClipRect)
{
    if (mAppDriver != nullptr)
        mAppDriver->Redraw(theClipRect);
}

void SexyAppBase::Quit()
{
    logfi("Quiting the %s(nested level: %d)", mProdName.c_str(), mUpdateDepth);
    mCloseRequested = true;
    mExitToTop = true;
}

void SexyAppBase::Shutdown()
{
    mShutDown = true;
    if (mAppDriver != nullptr)
        mAppDriver->Shutdown();
    ShutdownHook();
}

void SexyAppBase::Cleanup()
{
    if (gSexyAppBase == nullptr)
        return;

    Shutdown();
    logfi("Cleanuping the %s", mProdName.c_str());

    if (mAppDriver != nullptr)
        mAppDriver->WaitForLoadingThread();

    if (mInputConnectManager != nullptr)
    {
        mInputConnectManager->Stop();
        mInputConnectManager->RemoveListener(static_cast<InputConnectListener*>(this));
    }
    if (mInputManager != nullptr)
        mInputManager->Cleanup();

    ProcessSafeDeleteList();

    if (mPreLoadWidget != nullptr && mWidgetManager != nullptr)
        mWidgetManager->RemoveWidget(mPreLoadWidget);
    delete mPreLoadWidget;
    mPreLoadWidget = nullptr;

    for (Dialog* aDialog : mDialogList)
    {
        if (mWidgetManager != nullptr)
            mWidgetManager->RemoveWidget(aDialog);
        delete aDialog;
    }
    mDialogList.clear();
    mDialogMap.clear();

    delete mWidgetManager;
    mWidgetManager = nullptr;

    if (mResStreamsDriver != nullptr)
        mResStreamsDriver->Destroy();
    mResStreamsDriver = nullptr;

    delete mResourceManager;
    mResourceManager = nullptr;

    delete mInputConnectManager;
    mInputConnectManager = nullptr;
    delete mInputManager;
    mInputManager = nullptr;

    for (std::map<std::pair<Sexy::String, Sexy::String>, SharedImage>::iterator
             anEntry = mSharedImages.begin();
         anEntry != mSharedImages.end(); ++anEntry)
    {
        if (anEntry->second.mRefCount == 0)
            delete anEntry->second.mImage;
    }
    mSharedImages.clear();

    for (std::map<Sexy::String, std::map<Sexy::String, Image*>>::iterator
             aGroup = mImageGroups.begin();
         aGroup != mImageGroups.end(); ++aGroup)
    {
        for (std::map<Sexy::String, Image*>::iterator anImage = aGroup->second.begin();
             anImage != aGroup->second.end(); ++anImage)
            delete anImage->second;
    }
    mImageGroups.clear();

    Delete3DImageData();

    delete mRenderTargetPool;
    mRenderTargetPool = nullptr;

    delete mHandCursor;
    mHandCursor = nullptr;
    delete mDraggingCursor;
    mDraggingCursor = nullptr;
    delete mPointerCursor;
    mPointerCursor = nullptr;

    delete mMusicInterface;
    mMusicInterface = nullptr;
    delete mSoundManager;
    mSoundManager = nullptr;

    if (mAudioDriver != nullptr)
        mAudioDriver->Destroy();
    mAudioDriver = nullptr;

    if (mHttpDriver != nullptr)
        mHttpDriver->Destroy();
    mHttpDriver = nullptr;

    if (mAppDriver != nullptr)
        mAppDriver->Destroy();
    mAppDriver = nullptr;
    mPlatformDriverOrQueue = nullptr;

    if (gFileDriver != nullptr)
        gFileDriver->Destroy();
    gFileDriver = nullptr;
    mFileDriver = nullptr;

    delete mAuthManager;
    mAuthManager = nullptr;

    CleanupHook();
    gSexyAppBase = nullptr;
    WriteDemoBuffer();

    Sexy::ServiceManager::release(Sexy::ServiceManager::getInstance(nullptr));
    logfd("The %s is ready to exit.", mProdName.c_str());
}

int SexyAppBase::DoExit(int theCode)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->DoExit(theCode);
}

void SexyAppBase::HandleGameAlreadyRunning()
{
    if (mOnlyAllowOneCopyToRun)
        DoExit(0);
}

void SexyAppBase::PauseApp()
{
    mPaused = true;
    if (mAppDriver != nullptr)
        mAppDriver->Pause();
    AppEnteredBackground();
}

void SexyAppBase::ResumeApp()
{
    Sexy::Thread aThread = CurrentThread();
    mPrimaryThread.mThread = aThread.mThread;
    mPrimaryThread.mValid = aThread.mValid;

    if (mPlatformDriverOrQueue == nullptr)
        return;

    if (mAppDriver != nullptr)
        mAppDriver->Resume();

    Sexy::Event anEvent;
    std::memset(&anEvent, 0, sizeof(anEvent));
    anEvent.modifiers = 8;
    anEvent.eventCode = -1;
    anEvent.eventCodeMask = 1;
    ProcessMessage(anEvent);
    AppEnteredForeground();
}

void SexyAppBase::StartAuth()
{
    if (mAuthManager == nullptr)
        return;
    if (mAuthManager->GetAuthMode() != 0)
    {
        mAuthManager->Start();
        return;
    }
    if (mAuthManager->DoAuthenticate() == 0)
        Shutdown();
}

// --------------------------------------------------------------------------
// The loading thread and the preload widget.
// --------------------------------------------------------------------------
int SexyAppBase::StartLoadingThread()
{
    if (mAppDriver == nullptr)
        return 0;
    // The flag records that a thread was spawned. This port's slot pulls
    // one slice per call, and TitleScreen asks again each frame, so the
    // flag must not swallow those later slices.
    mLoadingThreadStarted = true;
    return mAppDriver->StartLoadingThread();
}

int SexyAppBase::WaitForLoadingThread()
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->WaitForLoadingThread();
}

double SexyAppBase::GetLoadingThreadProgress()
{
    int aTotalTasks = mNumLoadingThreadTasks.load();
    if (aTotalTasks <= 0)
        return 0.0;
    return (double)mCompletedLoadingThreadTasks.load() / (double)aTotalTasks;
}

void SexyAppBase::InitPreLoadWidget()
{
    if (mPreLoadStarted)
        return;
    if (mThreadedPreload && mAutoStartLoadingThread && mPreLoadWidget == nullptr)
        CreatePreLoadWidget();
    mPreLoadStarted = true;
}

Widget* SexyAppBase::CreatePreLoadWidget()
{
    mPreLoadWidget = new PreLoadWidget(this);
    if (mWidgetManager != nullptr)
        mWidgetManager->AddWidget(mPreLoadWidget);
    return mPreLoadWidget;
}

void SexyAppBase::FinishPreLoadWidget()
{
    mPreLoadStarted = true;
    if (mPreLoadWidget != nullptr && mWidgetManager != nullptr)
        mWidgetManager->RemoveWidget(mPreLoadWidget);
}

// --------------------------------------------------------------------------
// Sound.
// --------------------------------------------------------------------------
void SexyAppBase::PlaySample(int theSoundId)
{
    if (mSoundManager == nullptr)
        return;
    SoundInstance* aSound = mSoundManager->GetSoundInstance((unsigned int)theSoundId);
    if (aSound != nullptr)
        aSound->Play(false, true);
}

void SexyAppBase::PlaySampleWithPan(int theSoundId, int thePan)
{
    if (mSoundManager == nullptr)
        return;
    SoundInstance* aSound = mSoundManager->GetSoundInstance((unsigned int)theSoundId);
    if (aSound != nullptr)
    {
        aSound->SetPan(thePan);
        aSound->Play(false, true);
    }
}

void SexyAppBase::PlaySampleSingle(int theSoundId)
{
    PlaySample(theSoundId);
}

void SexyAppBase::PlaySampleSingleWithPan(int theSoundId, int thePan)
{
    PlaySampleWithPan(theSoundId, thePan);
}

bool SexyAppBase::IsMuted()
{
    return mMuteCount > 0;
}

void SexyAppBase::Mute(bool autoMute)
{
    if (mMuteCount == 0)
    {
        mUnmutedMusicVolume = mMusicVolume;
        mUnmutedSfxVolume = mSfxVolume;
        SetMusicVolume(0.0);
        SetSfxVolume(0.0);
    }
    ++mMuteCount;
    if (autoMute)
        ++mAutoMuteCount;
}

void SexyAppBase::Unmute(bool autoMute)
{
    if (mMuteCount <= 0)
        return;
    --mMuteCount;
    if (autoMute && mAutoMuteCount > 0)
        --mAutoMuteCount;
    if (mMuteCount == 0)
    {
        SetMusicVolume(mUnmutedMusicVolume);
        SetSfxVolume(mUnmutedSfxVolume);
    }
}

double SexyAppBase::GetMusicVolume() { return mMusicVolume; }
double SexyAppBase::GetSfxVolume() { return mSfxVolume; }

double SexyAppBase::GetMasterVolume()
{
    return (mMusicVolume + mSfxVolume) * 0.5;
}

void SexyAppBase::SetMusicVolume(double theVolume)
{
    mMusicVolume = theVolume;
    if (mMusicInterface != nullptr)
        mMusicInterface->SetVolume(theVolume);
}

void SexyAppBase::SetSfxVolume(double theVolume)
{
    mSfxVolume = theVolume;
    if (mSoundManager != nullptr)
        mSoundManager->SetVolume(theVolume);
}

void SexyAppBase::SetMasterVolume(double theVolume)
{
    SetMusicVolume(theVolume);
    SetSfxVolume(theVolume);
}

MusicInterface* SexyAppBase::CreateMusicInterface(void*)
{
    return new MusicInterface();
}

// --------------------------------------------------------------------------
// Screen mode and the cursor.
// --------------------------------------------------------------------------
void SexyAppBase::SwitchScreenModeFull(bool windowed, bool use3D, bool force)
{
    if (mAppDriver != nullptr)
        mAppDriver->SwitchScreenModeFull(windowed, use3D, force);
    mWindowed = windowed;
}

int SexyAppBase::SwitchScreenMode(bool windowed)
{
    SwitchScreenModeFull(windowed, Is3DAccelerated() != 0, true);
    return 1;
}

void SexyAppBase::SwitchScreenMode()
{
    SwitchScreenMode(!mWindowed);
}

int SexyAppBase::Is3DAccelerated()
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->Is3DAccelerated();
}

int SexyAppBase::Is3DAccelerationSupported()
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->Is3DAccelerationSupported();
}

int SexyAppBase::Is3DAccelerationRecommended()
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->Is3DAccelerationRecommended();
}

int SexyAppBase::Set3DAcclerated(bool theIs3D, bool theDoCheck)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->Set3DAcclerated(theIs3D, theDoCheck);
}

void SexyAppBase::SetAlphaDisabled(bool disabled)
{
    mAlphaDisabled = disabled;
}

void SexyAppBase::InitCursors()
{
    if (mHandCursor == nullptr)
        mHandCursor = MakeCursorImage(this, gHandCursorPixels);
    mPointerHotX = 7;
    mPointerHotY = 4;
    if (mPointerCursor == nullptr)
        mPointerCursor = MakeCursorImage(this, gPointerCursorPixels);
    mDragHotY = 13;
    mCursorHotY = 11;
    if (mDragCursor == nullptr)
        mDragCursor = MakeCursorImage(this, gDraggingCursorPixels);
    mDragHotX = 9;
    mDragHotY = 5;
}

void SexyAppBase::EnforceCursor()
{
    if (mAppDriver != nullptr)
        mAppDriver->EnforceCursor();
}

int SexyAppBase::SetCursor(int theCursorNum)
{
    mCursorNum = theCursorNum;
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->SetCursor(theCursorNum);
}

int SexyAppBase::GetCursor()
{
    return mCursorNum;
}

int SexyAppBase::SetCursorImage(int theCursorNum, Image* theImage)
{
    if (theCursorNum >= 0 && theCursorNum < 13)
        mCursorImages[theCursorNum] = theImage;
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->SetCursorImage(theCursorNum, theImage);
}

int SexyAppBase::EnableCustomCursors(bool theEnable)
{
    mCustomCursorsEnabled = theEnable;
    mCustomCursorDirty = true;
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->EnableCustomCursors(theEnable);
}

int SexyAppBase::IsCustomCursorSupported()
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->IsCustomCursorSupported();
}

MemoryImage* SexyAppBase::CreateCursorFromAndMask(unsigned char*, unsigned char*, int, int)
{
    return nullptr;
}

// --------------------------------------------------------------------------
// Images.
// --------------------------------------------------------------------------
Image* SexyAppBase::GetTexImage(const Sexy::String& theFileName, bool commitBits)
{
    if (!NameContains(theFileName, ".tex") && !NameContains(theFileName, ".TEX"))
        return nullptr;

    SDL_IOStream* aFile = p_fopen(theFileName.c_str(), "rb");
    if (aFile == nullptr)
        return nullptr;

    char aMagic[12] = {};
    if (p_fread(aMagic, 8, 1, aFile) != 1 || std::strcmp(aMagic, "SEXYTEX") != 0)
    {
        p_fclose(aFile);
        return nullptr;
    }

    TexHeader aHeader;
    std::memset(&aHeader, 0, sizeof(aHeader));
    if (p_fread(&aHeader, 40, 1, aFile) != 1 || aHeader.mSignature != 0
        || (unsigned int)(aHeader.mFormat - 2) > 8u)
    {
        p_fclose(aFile);
        return nullptr;
    }

    unsigned int aByteCount = (unsigned int)aHeader.mWidth * (unsigned int)aHeader.mHeight
                            * (unsigned int)kBytesPerPixel[aHeader.mFormat];
    unsigned char* aBits = new unsigned char[aByteCount + 4];
    unsigned char* aChunk = nullptr;
    bool aLoaded = true;

    if ((aHeader.mFlags & 1) == 0)
    {
        if (p_fread(aBits, aByteCount, 1, aFile) != 1)
            aLoaded = false;
    }
    else
    {
        unsigned int aPackedSize = (unsigned int)aHeader.mDataSize;
        unsigned int aChunkSize = aPackedSize >= 0x200000u ? 0x200000u : aPackedSize;
        aChunk = new unsigned char[aChunkSize == 0 ? 1 : aChunkSize];

        z_stream aStream;
        std::memset(&aStream, 0, sizeof(aStream));
        aStream.next_out = aBits;
        aStream.avail_out = aByteCount;

        bool aInflating = false;
        bool aDidInflate = false;
        while (aPackedSize != 0)
        {
            unsigned int aReadSize = aPackedSize >= aChunkSize ? aChunkSize : aPackedSize;
            if (p_fread(aChunk, aReadSize, 1, aFile) != 1)
            {
                aLoaded = false;
                break;
            }
            aStream.next_in = aChunk;
            aStream.avail_in = aReadSize;
            if (!aInflating && inflateInit2_(&aStream, 47, "1.2.3", (int)sizeof(z_stream)) != 0)
            {
                aLoaded = false;
                break;
            }
            aInflating = true;
            aDidInflate = true;
            if ((unsigned int)inflate(&aStream, 0) > 1u)
            {
                aLoaded = false;
                break;
            }
            aPackedSize -= aReadSize;
        }
        if (aDidInflate && inflateEnd(&aStream) != 0)
            aLoaded = false;
    }

    delete[] aChunk;
    p_fclose(aFile);
    if (!aLoaded)
    {
        delete[] aBits;
        return nullptr;
    }

    DeviceImage* anImage = new DeviceImage();
    anImage->TakePixels(PixelFormat(aHeader.mFormat), aBits, aHeader.mWidth,
                        aHeader.mHeight, commitBits);
    anImage->mFileName = theFileName;
    anImage->mFileVariant = theFileName;
    return anImage;
}

Image* SexyAppBase::GetImage(const Sexy::String& theFileName, bool commitBits)
{
    // Atlas sheets are extension-less names such as reanim/plant-atlas1, and
    // the file on disk is that name plus .tex. A name that already has an
    // extension still tries name.tex, which is the packed form of a loose file.
    Image* anImage = nullptr;
    if (!NameContains(theFileName, ".tex") && !NameContains(theFileName, ".TEX"))
    {
        Sexy::String aTexName = theFileName;
        aTexName += ".tex";
        anImage = GetTexImage(aTexName, commitBits);
    }
    if (anImage != nullptr)
        return anImage;

    ImageLib::Image* aDecoded = ImageLib::GetImage(theFileName, true, true, false);
    if (aDecoded == nullptr)
        return nullptr;

    DeviceImage* aNewImage = new DeviceImage(this);
    aNewImage->Create(aDecoded->GetWidth(), aDecoded->GetHeight());
    // Create() allocates the pixel buffer. A palettized GIF has its colours in
    // mBits as well (the decoder expands them), so both paths copy those pixels.
    // The old palettized branch copied into a buffer Create() never allocated.
    if (aDecoded->mBits != nullptr)
    {
        aNewImage->SetBits(aDecoded->mBits, aDecoded->GetWidth(), aDecoded->GetHeight(), commitBits);
    }
    aNewImage->mFileName = theFileName;
    delete aDecoded;
    return aNewImage;
}

Image* SexyAppBase::GetImageWithAttrs(const Sexy::String& theFileName, const Sexy::String& theVariant,
                                      const Sexy::String& theAttributes, bool commitBits)
{
    (void)theVariant;
    (void)theAttributes;
    return GetImage(theFileName, commitBits);
}

bool SexyAppBase::ReloadImage(Image* theImage)
{
    if (theImage == nullptr || theImage->mFileName.empty())
        return false;
    logfi("Reloading image: %s", theImage->mFileName.c_str());
    Image* aFresh = GetImage(theImage->mFileName, true);
    if (aFresh == nullptr)
        return false;
    delete aFresh;
    return true;
}

void SexyAppBase::RemoveImageForInput(const Sexy::String& theFileName)
{
    for (std::map<Sexy::String, std::map<Sexy::String, Image*>>::iterator
             aGroup = mImageGroups.begin();
         aGroup != mImageGroups.end(); ++aGroup)
    {
        std::map<Sexy::String, Image*>::iterator anEntry = aGroup->second.find(theFileName);
        if (anEntry == aGroup->second.end())
            continue;
        delete anEntry->second;
        aGroup->second.erase(anEntry);
    }
}

Image* SexyAppBase::GetImageForInput(const Sexy::String& theFileName, int theWidth, int theHeight,
                                     const Sexy::String& theVariant)
{
    (void)theWidth;
    (void)theHeight;
    (void)theVariant;
    std::map<Sexy::String, Image*>& aGroup = mImageGroups[theFileName];
    std::map<Sexy::String, Image*>::iterator aFound = aGroup.find(theFileName);
    if (aFound != aGroup.end())
        return aFound->second;
    Image* anImage = GetImage(theFileName, true);
    aGroup[theFileName] = anImage;
    return anImage;
}

SharedImageRef SexyAppBase::GetSharedImage(const Sexy::String& theFileName, const Sexy::String& theVariant,
                                           bool* isNew, bool commitBits, bool allowTriReps)
{
    (void)allowTriReps;
    std::pair<Sexy::String, Sexy::String> aKey(UpperCopy(theFileName), UpperCopy(theVariant));
    std::map<std::pair<Sexy::String, Sexy::String>, SharedImage>::iterator aFound =
        mSharedImages.find(aKey);
    if (aFound != mSharedImages.end())
    {
        if (isNew != nullptr)
            *isNew = false;
        ++aFound->second.mRefCount;
        SharedImageRef aRef(&aFound->second);
        return aRef;
    }

    SharedImage aShared;
    aShared.mImage = GetImage(theFileName, commitBits);
    aShared.mRefCount = 1;
    mSharedImages[aKey] = aShared;
    if (isNew != nullptr)
        *isNew = true;
    SharedImageRef aRef(&mSharedImages[aKey]);
    return aRef;
}

SharedImageRef SexyAppBase::GetSharedImageWithAttrs(const Sexy::String& theFileName,
                                                    const Sexy::String& theVariant,
                                                    const Sexy::String& theAttributes,
                                                    const Sexy::String& theAlphaFile,
                                                    bool* isNew)
{
    (void)theAttributes;
    (void)theAlphaFile;
    return GetSharedImage(theFileName, theVariant, isNew, true, false);
}

void SexyAppBase::CleanSharedImages()
{
    if (!mCleanupSharedImages)
        return;
    mCleanupSharedImages = false;

    for (std::map<std::pair<Sexy::String, Sexy::String>, SharedImage>::iterator
             anEntry = mSharedImages.begin();
         anEntry != mSharedImages.end();)
    {
        if (anEntry->second.mRefCount == 0)
        {
            delete anEntry->second.mImage;
            anEntry = mSharedImages.erase(anEntry);
        }
        else
        {
            ++anEntry;
        }
    }
}

void SexyAppBase::AddMemoryImage(MemoryImage* theMemoryImage)
{
    if (theMemoryImage == nullptr)
        return;
    mMemoryImageCritSect.Lock();
    mMemoryImages.insert(theMemoryImage);
    mMemoryImageCritSect.Unlock();
}

int SexyAppBase::RemoveMemoryImage(MemoryImage* theImage)
{
    if (theImage == nullptr)
        return 0;
    mMemoryImageCritSect.Lock();
    mMemoryImages.erase(theImage);
    mMemoryImageCritSect.Unlock();
    Remove3DData(theImage);
    return 1;
}

void SexyAppBase::Remove3DData(MemoryImage* theImage)
{
    if (mAppDriver != nullptr)
        mAppDriver->Remove3DData(theImage);
}

void SexyAppBase::DeleteNativeImageData()
{
    mMemoryImageCritSect.Lock();
    for (MemoryImage* anImage : mMemoryImages)
        anImage->DeleteNativeData();
    mMemoryImageCritSect.Unlock();
}

void SexyAppBase::DeleteExtraImageData()
{
    mMemoryImageCritSect.Lock();
    for (MemoryImage* anImage : mMemoryImages)
        anImage->DeleteExtraBuffers();
    mMemoryImageCritSect.Unlock();
}

void SexyAppBase::ReInitImages()
{
    mMemoryImageCritSect.Lock();
    for (MemoryImage* anImage : mMemoryImages)
        anImage->ReInit();
    mMemoryImageCritSect.Unlock();
}

void SexyAppBase::Delete3DImageData()
{
    mMemoryImageCritSect.Lock();
    for (MemoryImage* anImage : mMemoryImages)
        Remove3DData(anImage);
    mMemoryImageCritSect.Unlock();
}

void SexyAppBase::Evict3DImageData(unsigned int theAmount)
{
    mMemoryImageCritSect.Lock();
    for (MemoryImage* anImage : mMemoryImages)
    {
        if (theAmount == 0)
            break;
        Remove3DData(anImage);
    }
    mMemoryImageCritSect.Unlock();
}

int SexyAppBase::CreateImage(int theWidth, int theHeight)
{
    MemoryImage* anImage = new MemoryImage(this);
    anImage->Create(theWidth, theHeight);
    return 0;
}

MemoryImage* SexyAppBase::CopyImage(Image* theImage, const TRect<int>& theRect)
{
    MemoryImage* aCopy = new MemoryImage(this);
    aCopy->Create(theRect.mWidth, theRect.mHeight);
    Graphics aGraphics(aCopy);
    aGraphics.DrawImage(theImage, -theRect.mX, -theRect.mY);
    aCopy->CopyAttributes(theImage);
    return aCopy;
}

MemoryImage* SexyAppBase::CopyImage(Image* theImage)
{
    if (theImage == nullptr)
        return nullptr;
    TRect<int> aRect(0, 0, theImage->GetWidth(), theImage->GetHeight());
    return CopyImage(theImage, aRect);
}

MemoryImage* SexyAppBase::CreateCrossfadeImage(Image* theFirst, const TRect<int>& theSrcRect,
                                               Image* theSecond, const TRect<int>& theDestRect,
                                               double theAlpha)
{
    (void)theSecond;
    (void)theDestRect;
    (void)theAlpha;
    return CopyImage(theFirst, theSrcRect);
}

void SexyAppBase::ColorizeImage(Image* theImage, const Color& theColor)
{
    (void)theImage;
    (void)theColor;
}

MemoryImage* SexyAppBase::CreateColorizedImage(Image* theImage, const Color& theColor)
{
    (void)theColor;
    if (theImage == nullptr)
        return nullptr;
    return CopyImage(theImage);
}

int SexyAppBase::MirrorImage(Image*) { return 0; }
int SexyAppBase::FlipImage(Image*) { return 0; }
int SexyAppBase::RotateImageHue(MemoryImage* theImage, int theHue)
{
    (void)theImage;
    (void)theHue;
    return 0;
}

unsigned int SexyAppBase::HSLToRGB(int theHue, int theSaturation, int theLightness)
{
    return HslToRgb(theHue, theSaturation, theLightness);
}

unsigned int SexyAppBase::RGBToHSL(int theRed, int theGreen, int theBlue)
{
    int aMax = theRed;
    if (theGreen > aMax) aMax = theGreen;
    if (theBlue > aMax) aMax = theBlue;
    int aMin = theRed;
    if (theGreen < aMin) aMin = theGreen;
    if (theBlue < aMin) aMin = theBlue;
    int aLight = (aMax + aMin) / 2;
    int aSat = 0;
    int aHue = 0;
    if (aMax != aMin)
    {
        int aRange = aMax - aMin;
        aSat = aLight > 127 ? (aRange * 255) / (510 - aMax - aMin) : (aRange * 255) / (aMax + aMin);
        if (aMax == theRed)
            aHue = ((theGreen - theBlue) * 43) / aRange;
        else if (aMax == theGreen)
            aHue = 85 + ((theBlue - theRed) * 43) / aRange;
        else
            aHue = 171 + ((theRed - theGreen) * 43) / aRange;
        if (aHue < 0)
            aHue += 256;
    }
    return (unsigned int)((aHue << 16) | (aSat << 8) | aLight);
}

unsigned int SexyAppBase::HSLToRGB(const unsigned int* theInput, unsigned int* theOutput, int theCount)
{
    if (theInput == nullptr || theOutput == nullptr)
        return 0;
    for (int i = 0; i < theCount; ++i)
    {
        unsigned int aColor = theInput[i];
        theOutput[i] = HslToRgb((int)((aColor >> 16) & 0xFF),
                                (int)((aColor >> 8) & 0xFF),
                                (int)(aColor & 0xFF));
    }
    return (unsigned int)theCount;
}

unsigned int SexyAppBase::RGBToHSL(const unsigned int* theInput, unsigned int* theOutput, int theCount)
{
    if (theInput == nullptr || theOutput == nullptr)
        return 0;
    for (int i = 0; i < theCount; ++i)
    {
        unsigned int aColor = theInput[i];
        theOutput[i] = RGBToHSL((int)((aColor >> 16) & 0xFF),
                                (int)((aColor >> 8) & 0xFF),
                                (int)(aColor & 0xFF));
    }
    return (unsigned int)theCount;
}

// --------------------------------------------------------------------------
// Properties.
// --------------------------------------------------------------------------
void SexyAppBase::SetBoolean(const Sexy::String& theId, bool theValue)
{
    mBoolProperties[theId] = theValue;
}

void SexyAppBase::SetInteger(const Sexy::String& theId, int theValue)
{
    mIntProperties[theId] = theValue;
}

void SexyAppBase::SetDouble(const Sexy::String& theId, double theValue)
{
    mDoubleProperties[theId] = theValue;
}

void SexyAppBase::SetString(const Sexy::String& theId, const Sexy::String& theValue)
{
    mStringProperties[theId] = theValue;
    int aId = 0;
    if (PropertyKeyIsInteger(theId, &aId) && mPopLoc.find(aId) == mPopLoc.end())
        mPopLoc[aId] = theValue;
}

int SexyAppBase::GetBoolean(const Sexy::String& theId)
{
    return GetBoolean(theId, false);
}

int SexyAppBase::GetBoolean(const Sexy::String& theId, bool theDefault)
{
    std::map<Sexy::String, bool>::iterator aFound = mBoolProperties.find(theId);
    if (aFound == mBoolProperties.end())
        return theDefault ? 1 : 0;
    return aFound->second ? 1 : 0;
}

int SexyAppBase::GetInteger(const Sexy::String& theId)
{
    return GetInteger(theId, 0);
}

int SexyAppBase::GetInteger(const Sexy::String& theId, int theDefault)
{
    std::map<Sexy::String, int>::iterator aFound = mIntProperties.find(theId);
    if (aFound == mIntProperties.end())
        return theDefault;
    return aFound->second;
}

double SexyAppBase::GetDouble(const Sexy::String& theId)
{
    return GetDouble(theId, 0.0);
}

double SexyAppBase::GetDouble(const Sexy::String& theId, double theDefault)
{
    std::map<Sexy::String, double>::iterator aFound = mDoubleProperties.find(theId);
    if (aFound == mDoubleProperties.end())
        return theDefault;
    return aFound->second;
}

Sexy::String SexyAppBase::GetString(const Sexy::String& theId)
{
    return GetString(theId, Sexy::String());
}

Sexy::String SexyAppBase::GetString(const Sexy::String& theId, const Sexy::String& theDefault)
{
    std::map<Sexy::String, Sexy::String>::iterator aFound = mStringProperties.find(theId);
    if (aFound == mStringProperties.end())
        return theDefault;
    return aFound->second;
}

std::vector<Sexy::String> SexyAppBase::GetStringVector(const Sexy::String& theId)
{
    std::map<Sexy::String, std::vector<Sexy::String>>::iterator aFound =
        mStringVectorProperties.find(theId);
    if (aFound == mStringVectorProperties.end())
        return std::vector<Sexy::String>();
    return aFound->second;
}

int SexyAppBase::LoadProperties(const Sexy::String& theFileName, bool required, bool checkSig)
{
    Buffer aBuffer;
    if (ReadBufferFromFile(theFileName, &aBuffer, false) == 0)
    {
        if (!required)
            return 1;
        Popup(Sexy::String("Unable to open properties file ") + theFileName);
        return 0;
    }
    if (checkSig && !CheckSignature(aBuffer, theFileName))
    {
        Popup(Sexy::String("Signature check failed on ") + theFileName);
        return 0;
    }
    return 1;
}

int SexyAppBase::LoadProperties()
{
    return LoadProperties(mDefaultPropertiesPath, true, false);
}

void SexyAppBase::LoadResourceManifest()
{
    if (mResourceManager != nullptr)
        mResourceManager->ParseResourcesFile(mResourceManifestPath);
}

void SexyAppBase::ShowResourceError(bool showError)
{
    if (mResourceManager == nullptr)
        return;
    Sexy::String anError = mResourceManager->GetErrorText();
    logfe("%s", anError.c_str());
    if (!showError)
        return;
    if (mUpdateDepth <= 0)
        DoExit(0);
    else
        Shutdown();
}

bool SexyAppBase::CheckSignature(const Sexy::Buffer&, const Sexy::String&)
{
    return false;
}

// --------------------------------------------------------------------------
// The command line.
// --------------------------------------------------------------------------
void SexyAppBase::HandleCmdLineParam(const Sexy::String& theName, const Sexy::String& theValue)
{
    if (theName.compare("-help") == 0)
    {
        Sexy::String aHelp = FormatHelpString();
        std::puts(aHelp.c_str());
        Shutdown();
        return;
    }

    if (mAppDriver != nullptr)
        mAppDriver->HandleCmdLineParam(theName, theValue);

    if (theName.compare("-d") == 0 || theName.compare("-userdata") == 0)
    {
        mAppDataDir = theValue;
        Sexy::SetAppDataFolder(theValue);
        return;
    }

    if (mAuthManager != nullptr)
        mAuthManager->HandleCmdLineParam(theName, theValue);
}

void SexyAppBase::ParseCmdLine(const Sexy::String& theCommandLine)
{
    if (theCommandLine.empty())
        return;

    Sexy::String aName;
    Sexy::String aValue;
    bool aNameOpen = false;
    bool aValueOpen = false;
    bool aInQuote = false;

    for (unsigned int i = 0; i < theCommandLine.size(); ++i)
    {
        char aChar = theCommandLine[i];
        bool aEmit = false;
        bool aLast = i + 1 == theCommandLine.size();

        if (aChar == '"')
        {
            aInQuote = !aInQuote;
            if (aLast || !aInQuote)
                aEmit = true;
            else
                continue;
        }
        else if (aChar == ' ')
        {
            if (aNameOpen)
            {
                aNameOpen = false;
                aValueOpen = true;
            }
            else if (aValueOpen && !aInQuote)
            {
                aEmit = true;
            }
        }
        else if (aChar == '=')
        {
            aNameOpen = false;
            aValueOpen = true;
        }
        else if (!aValueOpen)
        {
            if (aChar == '-')
            {
                aName.push_back('-');
                aNameOpen = true;
            }
            else if (aNameOpen)
            {
                aName.push_back(aChar);
            }
        }
        else if (aChar == '-' && aValue.empty())
        {
            if (!aName.empty())
                HandleCmdLineParam(aName, aValue);
            aName.clear();
            aNameOpen = true;
            aValueOpen = false;
            continue;
        }
        else
        {
            aValue.push_back(aChar);
        }

        if (!aEmit && aLast)
            aEmit = true;
        if (aEmit && !aName.empty())
        {
            HandleCmdLineParam(aName, aValue);
            aName.clear();
            aValue.clear();
            aValueOpen = false;
        }
    }
}

void SexyAppBase::ParseCmdLineVector(const std::vector<Sexy::String>& theArguments)
{
    if (theArguments.size() <= 1)
        return;

    Sexy::String aName;
    Sexy::String aValue;
    for (unsigned int i = 1; i < theArguments.size(); ++i)
    {
        const Sexy::String& anArgument = theArguments[i];
        if (anArgument.empty() || anArgument[0] != '-')
        {
            if (aName.empty())
                continue;
            HandleCmdLineParam(aName, anArgument);
            aName.clear();
            aValue.clear();
            continue;
        }

        aName.clear();
        aValue.clear();
        bool aValueOpen = false;
        aName.push_back('-');
        for (unsigned int j = 1; j < anArgument.size(); ++j)
        {
            char aChar = anArgument[j];
            if (aChar == '=')
                aValueOpen = true;
            else if (aValueOpen)
                aValue.push_back(aChar);
            else
                aName.push_back(aChar);
        }

        if (aValueOpen)
        {
            HandleCmdLineParam(aName, aValue);
            aName.clear();
            aValue.clear();
        }
        else if (i + 1 < theArguments.size())
        {
            ++i;
            HandleCmdLineParam(aName, theArguments[i]);
            aName.clear();
        }
    }
}

void SexyAppBase::ParseCmdLineString(const Sexy::String& theCommandLine)
{
    ParseCmdLine(theCommandLine);
}

void SexyAppBase::DoParseCmdLine()
{
    ParseCmdLineVector(mCommandLine);
    mCmdLineParsed = true;
}

int SexyAppBase::GetCmdLine()
{
    mCommandLineText.clear();
    for (unsigned int i = 0; i < mCommandLine.size(); ++i)
    {
        if (i != 0)
            mCommandLineText += " ";
        mCommandLineText += mCommandLine[i];
    }
    return (int)mCommandLine.size();
}

void SexyAppBase::InitCmdLine(int argc, char** argv)
{
    mCommandLine.clear();
    for (int i = 0; i < argc && argv != nullptr; ++i)
        mCommandLine.push_back(Sexy::String(argv[i] != nullptr ? argv[i] : ""));
}

int SexyAppBase::AddParameterEntry(const ParameterEntry* theEntry)
{
    if (theEntry == nullptr)
        return 0;
    mParameters.push_back(*theEntry);
    return (int)mParameters.size();
}

int SexyAppBase::AddParameterEntries(const ParameterEntry* theEntries)
{
    if (theEntries == nullptr)
        return 0;
    for (const ParameterEntry* anEntry = theEntries; anEntry->mName != nullptr; ++anEntry)
        mParameters.push_back(*anEntry);
    return (int)mParameters.size();
}

void SexyAppBase::AddParameterEntry(void* theEntry)
{
    AddParameterEntry(static_cast<const ParameterEntry*>(theEntry));
}

void SexyAppBase::AddParameterEntries(void* theEntries)
{
    AddParameterEntries(static_cast<const ParameterEntry*>(theEntries));
}

Sexy::String SexyAppBase::FormatHelpString()
{
    Sexy::String aResult = "Usage of ";
    aResult += mTitle;
    aResult += ":\n";

    for (unsigned int i = 0; i < mParameters.size(); ++i)
    {
        const char* aName = mParameters[i].mName;
        const char* aValue = mParameters[i].mValue;
        Sexy::String aLine = "  ";
        if (aName != nullptr)
            aLine += aName;
        if (aValue != nullptr)
        {
            aLine += "\t";
            aLine += aValue;
        }
        aResult += aLine;
        aResult += "\n";
    }
    return aResult;
}

// --------------------------------------------------------------------------
// Files, the registry and the clipboard. Each one forwards to the driver.
// --------------------------------------------------------------------------
IFileDriver* SexyAppBase::InitFileDriver()
{
    if (gFileDriver == nullptr)
        gFileDriver = Lawn_CreateFileDriver(nullptr);
    mFileDriver = gFileDriver;
    return gFileDriver;
}

int SexyAppBase::InitDDInterface() { return 0; }

bool SexyAppBase::FileExists(const Sexy::String& theFileName)
{
    SDL_IOStream* aFile = p_fopen(theFileName.c_str(), "rb");
    if (aFile == nullptr)
        return false;
    p_fclose(aFile);
    return true;
}

bool SexyAppBase::EraseFile(const Sexy::String& theFileName)
{
    return SDL_RemovePath(theFileName.c_str());
}

int SexyAppBase::ReadBufferFromFile(const Sexy::String& theFileName, Buffer* theBuffer, bool dontWriteToDemo)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ReadBufferFromFile(theFileName, theBuffer, dontWriteToDemo);
}

bool SexyAppBase::WriteBufferToFile(const Sexy::String& theFileName, const Buffer* theBuffer)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->WriteBufferToFile(theFileName, theBuffer);
}

bool SexyAppBase::WriteBytesToFile(const Sexy::String& theFileName, const void* theData, unsigned long theLength)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->WriteBytesToFile(theFileName, theData, theLength);
}

int SexyAppBase::RegistryWrite(const Sexy::String& theKey, unsigned long theType,
                               const unsigned char* theData, unsigned long theLength)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigWrite(theKey, theType, (unsigned char*)theData, theLength);
}

int SexyAppBase::RegistryWriteString(const Sexy::String& theKey, const Sexy::String& theValue)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigWriteString(theKey, theValue);
}

int SexyAppBase::RegistryWriteInteger(const Sexy::String& theKey, int theValue)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigWriteInteger(theKey, theValue);
}

int SexyAppBase::RegistryWriteBoolean(const Sexy::String& theKey, bool theValue)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigWriteBoolean(theKey, theValue);
}

int SexyAppBase::RegistryWriteData(const Sexy::String& theKey, const unsigned char* theData,
                                   unsigned long theLength)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigWriteData(theKey, (unsigned char*)theData, theLength);
}

int SexyAppBase::RegistryEraseKey(const Sexy::String& theKey)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigEraseKey(theKey);
}

int SexyAppBase::RegistryEraseValue(const Sexy::String& theKey)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigEraseValue(theKey);
}

int SexyAppBase::RegistryRead(const Sexy::String& theKey, unsigned long* theType,
                              unsigned char* theData, unsigned long* theLength)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigRead(theKey, theType, theData, (unsigned char*)theLength);
}

int SexyAppBase::RegistryReadString(const Sexy::String& theKey, Sexy::String* theValue)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigReadString(theKey, theValue);
}

int SexyAppBase::RegistryReadInteger(const Sexy::String& theKey, int* theValue)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigReadInteger(theKey, theValue);
}

int SexyAppBase::RegistryReadBoolean(const Sexy::String& theKey, bool* theValue)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigReadBoolean(theKey, theValue);
}

int SexyAppBase::RegistryReadData(const Sexy::String& theKey, unsigned char* theData,
                                  unsigned long* theLength)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->ConfigReadData(theKey, theData, theLength);
}

void SexyAppBase::WriteToRegistry()
{
    RegistryWriteInteger("MusicVolume", (int)(mMusicVolume * 100.0));
    RegistryWriteInteger("SfxVolume", (int)(mSfxVolume * 100.0));
    RegistryWriteInteger("Muted", (mMuteCount - mAutoMuteCount) > 0 ? 1 : 0);
    RegistryWriteInteger("ScreenMode", mWindowed ? 0 : 1);
    RegistryWriteInteger("PreferredX", mPreferredX);
    RegistryWriteInteger("PreferredY", mPreferredY);
    RegistryWriteInteger("PreferredWidth", mPreferredX2);
    RegistryWriteInteger("PreferredHeight", mPreferredY2);
    RegistryWriteInteger("CustomCursors", mCustomCursorsEnabled ? 1 : 0);
    RegistryWriteInteger("InProgress", 0);
    RegistryWriteBoolean("WaitForVSync", mWaitForVSync);
    RegistryWriteBoolean("Is3D", mIs3DAccelerated);
    if (mAppDriver != nullptr)
    {
        mAppDriver->WriteToConfig();
        mAppDriver->ConfigFlush();
    }
}

void SexyAppBase::ReadFromRegistry()
{
    mReadFromRegistry = true;
    mRegKey = GetString("RegistryKey", mRegKey);
    if (mRegKey.empty())
        return;

    int aTemp = 0;
    if (RegistryReadInteger("MusicVolume", &aTemp) != 0)
        mMusicVolume = (double)aTemp * 0.01;
    if (RegistryReadInteger("SfxVolume", &aTemp) != 0)
        mSfxVolume = (double)aTemp * 0.01;
    if (RegistryReadInteger("Muted", &aTemp) != 0)
        mMuteCount = aTemp;
    if (RegistryReadInteger("ScreenMode", &aTemp) != 0)
        mWindowed = aTemp == 0;

    RegistryReadInteger("PreferredX", &mPreferredX);
    RegistryReadInteger("PreferredY", &mPreferredY);
    RegistryReadInteger("PreferredWidth", &mPreferredX2);
    RegistryReadInteger("PreferredHeight", &mPreferredY2);

    if (RegistryReadInteger("CustomCursors", &aTemp) != 0)
        EnableCustomCursors(aTemp != 0);
    if (RegistryReadInteger("Is3D", &aTemp) != 0)
        mSavedIs3D = aTemp != 0;
    RegistryReadBoolean("WaitForVSync", &mWaitForVSync);

    if (mAppDriver != nullptr)
        mAppDriver->ReadFromConfig();
}

int SexyAppBase::CopyToClipboard(const Sexy::String& theText)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->CopyToClipboard(theText);
}

Sexy::String SexyAppBase::GetClipboard()
{
    Sexy::String aText;
    if (mAppDriver != nullptr)
        mAppDriver->GetClipboard(&aText);
    return aText;
}

// --------------------------------------------------------------------------
// The rest of the queries.
// --------------------------------------------------------------------------
bool SexyAppBase::OpenURL(const Sexy::String& theURL, bool shutdownOnOpen)
{
    logfi("url: %s", theURL.c_str());
    mIsOpeningURL = true;
    if (mAppDriver == nullptr)
        return false;
    return mAppDriver->OpenURL(theURL, shutdownOnOpen) != 0;
}

Sexy::String SexyAppBase::GetProductVersion(const Sexy::String&)
{
    return mProductVersion;
}

Sexy::String SexyAppBase::GetLocale()
{
    const char* aLocale = setLocale(nullptr, nullptr);
    if (aLocale == nullptr)
        return Sexy::String();
    return Sexy::String(aLocale);
}

Sexy::String SexyAppBase::GetLocaleEncoding()
{
    return Sexy::String();
}

void SexyAppBase::SetLocale(const Sexy::String& theLanguage, const Sexy::String& theEncoding)
{
    (void)theEncoding;
    setLocale(nullptr, theLanguage.c_str());
}

int SexyAppBase::IsMainThread()
{
    Sexy::Thread aThread = CurrentThread();
    return mPrimaryThread.mThread == aThread.mThread ? 1 : 0;
}

Sexy::String SexyAppBase::GetAppDataDir()
{
    if (!mAppDataDir.empty())
        return mAppDataDir;

    Sexy::String aFolder;
    GetAppDataFolder(&aFolder);
    if (!aFolder.empty())
        return aFolder;

    if (mCommandLineText.compare(".") == 0 || mCommandLineText.compare("./") == 0)
        return Sexy::String(".");
    if (mCommandLineText.empty())
        return mTitle;
    return mCommandLineText + "\\" + mTitle;
}

Sexy::String SexyAppBase::GetGameSEHInfo()
{
    unsigned int anElapsed = Sexy::GetTickCount() - mTimeLoaded;
    char aTime[16];
    char aThreadId[20];
    snprintf(aTime, sizeof(aTime), "%02u:%02u:%02u",
             anElapsed / 1000 / 3600, (anElapsed / 1000 / 60) % 60, (anElapsed / 1000) % 60);
    snprintf(aThreadId, sizeof(aThreadId), "%X", mPrimaryThreadId);

    Sexy::String aResult = "Product: ";
    aResult += mProdName;
    aResult += "\r\nVersion: ";
    aResult += mProductVersion;
    aResult += "\r\nTime Loaded: ";
    aResult += aTime;
    aResult += "\r\nFullscreen: ";
    aResult += mWindowed ? "No" : "Yes";
    aResult += "\r\nThread: ";
    aResult += aThreadId;
    return aResult;
}

bool SexyAppBase::IsScreenSaver() { return mIsScreenSaver; }

bool SexyAppBase::ClearUpdateBacklog(bool relaxTime)
{
    mPendingUpdatesAcc = 0;
    mUpdateFTimeAcc = 0;
    mLastTimeCheck = Sexy::GetTickCount();
    if (relaxTime)
        mRelaxUpdateBacklogCount = 1000;
    return true;
}

bool SexyAppBase::TakeScreenshot(MemoryImage& theImage)
{
    ClearUpdateBacklog(false);
    if (mAppDriver == nullptr)
        return false;
    return mAppDriver->TakeScreenshot(&theImage) != 0;
}

int SexyAppBase::TakeScreenshot()
{
    MemoryImage anImage;
    return TakeScreenshot(anImage) ? 1 : 0;
}

int SexyAppBase::RehupFocus()
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->RehupFocus();
}

int SexyAppBase::ShowMemoryUsage() { return 0; }

int SexyAppBase::ProcessDeferredMessages(bool theAll)
{
    if (mAppDriver == nullptr)
        return 0;
    return mAppDriver->Process(theAll);
}

void SexyAppBase::SafeDeleteWidget(Widget* theWidget)
{
    if (theWidget != nullptr
        && std::find(mSafeDeleteList.begin(), mSafeDeleteList.end(), theWidget) == mSafeDeleteList.end())
        mSafeDeleteList.push_back(theWidget);
}

void SexyAppBase::ProcessSafeDeleteList()
{
    // A modal wait still has its dialog and input callbacks on the stack.
    if (mUpdateAppDepth != 0 || mUpdateDepth != 0 || mIsDrawing)
        return;
    std::list<Widget*> aPendingDelete;
    aPendingDelete.swap(mSafeDeleteList);
    for (Widget* aWidget : aPendingDelete)
        delete aWidget;
}

char* SexyAppBase::GetCurrentEvent()
{
    return (char*)&mParameterStorage;
}

int SexyAppBase::GetInputManager()
{
    return mInputManager != nullptr ? 1 : 0;
}

int SexyAppBase::GetInputConnectManager()
{
    return mInputConnectManager != nullptr ? 1 : 0;
}

int SexyAppBase::CheckControllerStatus() { return 0; }
int SexyAppBase::HasTouchScreen() { return 0; }

SharedRenderTarget::Pool* SexyAppBase::GetSharedRenderTargetPool()
{
    if (mRenderTargetPool == nullptr)
        mRenderTargetPool = new SharedRenderTarget::Pool();
    return mRenderTargetPool;
}

DialogListener::~DialogListener() {}

} // namespace Sexy

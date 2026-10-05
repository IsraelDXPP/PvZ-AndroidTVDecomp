// LawnApp -- reconstructed from libGameMain.so.
//
// ---------------------------------------------------------------------------
// Inheritance
// ---------------------------------------------------------------------------
// The binary's chain is
//     class LawnApp : public Sexy::GamepadApp, public Sexy::ProfileEventListener,
//                     public Sexy::IGameCenter::Listener
// and it is what puts the first own field (mBoard) at +0x8A0:
//     SexyAppBase          0x000 .. 0x79F   (0x7A0: vptr + the three interface
//                                            vptrs at +4/+8, then its own table)
//     -> SexyCommonApp     +0x7A0 .. +0x823 (0x824; Sexy\SexyCommonApp.h ends at
//                                            mTrialCheckDelayPassed +0x821)
//     -> GamepadApp        +0x7A0 .. +0x897 (0x898; the InputDeviceListener vptr
//                                            at +0x824 -- `_ZThn2084_` -- then
//                                            0x70 bytes of own members)
//     ProfileEventListener +0x898 .. +0x89B (one vptr; `_ZThn2200_` thunks)
//     IGameCenter::Listener +0x89C .. +0x89F (one vptr; `_ZThn2204_` thunk)
//     LawnApp own members  +0x8A0 ..
// 2200 = 0x898 and 2204 = 0x89C, and the two sub-vtables the binary keeps for
// this class sit at 0x687DF0 (four slots, all `_ZThn2200_N7LawnApp...`) and
// 0x687E08 (twelve slots, with `_ZThn2204_N7LawnApp5onPayEi` at index 9) --
// see _recon/_off_tables.txt and _recon/vtables.json.
//
// Both listener bases are pure interfaces with no data member, so they are four
// bytes each and neither shifts a field.
//
// ---------------------------------------------------------------------------
// Object size
// ---------------------------------------------------------------------------
// 0xB28 = 2856 bytes, from the two allocation sites:
//     main      0x131E40   MOVW R0, #0xB28 ; heap alloc of 0xB28 at 0x131E68
//     GameInit  0x2C437C   MOVW R0, #0xB28 ; heap alloc of 0xB28 at 0x2C4380
//                          then BL LawnApp::LawnApp at 0x2C4384
// The members below run from +0x8A0 to +0xB24 and are the restore tree's 120
// fields, cross-checked against the raw offsets the transcribed bodies use
// (+2208 mBoard, +2300 mGameMode, +2412 mLevel, +2616 mBoardResult, +2764
// mSession, +2808 mHelpBarWidget, +2836/+2848 mQueryCoinState, +2852 the last
// byte). NOTE: the restore tree numbered the tail one ordinal too high
// (mQueryCoinState 709~713, mCanDoBuyMenuDialog 714); this file uses the
// measured offsets, which is what makes the block end exactly at 0xB28.
//
// TODO(recon): measured with the chain above in place, mBoard is at exactly
// +0x8A0 and every offset down to mSession (+0xACC) matches the comments below.
// From mSession on the object runs 4 bytes long, and the cause is outside this
// file: Game\LawnSession.h is 0x18 bytes where the binary's is 0x14. The
// static_assert at the bottom accepts the current value and names the fix.
//
// ---------------------------------------------------------------------------
// Vtable
// ---------------------------------------------------------------------------
// 178 slots in the binary, dumped from _ZTV7LawnApp in libGameMain115_rtti.c.
// The full list is kept at the bottom of this comment so the order below can be
// checked against it; the slots marked (own) are the ones LawnApp itself adds,
// the rest are inherited from Sexy::ButtonListener / Sexy::SexyAppBase /
// Sexy::SexyCommonApp / Sexy::GamepadApp.
//
// Slot 0/1 are the complete and deleting destructors. Itanium/ARM emits two,
// MSVC x86 emits one, so the generated absolute slot numbers are one lower than
// the numbers below. The declaration order below still follows the binary.
//
// Per the stage-1 rule every override below now carries the base chain's return
// type and is spelled `virtual`, so the declarations reproduce slots [2]..[145]
// mechanically. Three declarations in that range are deliberately NOT overrides
// because their transcribed name or parameter list matches no base slot; each
// one carries its own TODO(recon) in the class body.
//
//   [  0] 0x2c1820  LawnApp::~LawnApp              (own, complete)
//   [  1] 0x2c1e80  LawnApp::~LawnApp              (own, deleting)
//   [  2] 0x2b261c  LawnApp::ButtonPress           (overrides ButtonListener)
//   [  3] 0x13823c  Sexy::ButtonListener::ButtonPress(int,int)
//   [  4] 0x2c11e8  LawnApp::ButtonDepress         (overrides ButtonListener)
//   [  5] 0x143e3c  Sexy::ButtonListener::ButtonDownTick
//   [  6] 0x138248  Sexy::ButtonListener::ButtonMouseEnter
//   [  7] 0x13824c  Sexy::ButtonListener::ButtonMouseLeave
//   [  8] 0x138250  Sexy::ButtonListener::ButtonMouseMove
//   [  9] 0x336424  Sexy::SexyAppBase::MakeWindow
//   [ 10] 0x336e80  Sexy::SexyAppBase::InitCursors
//   [ 11] 0x2c242c  LawnApp::EnforceCursor
//   [ 12] 0x33defc  Sexy::SexyAppBase::ClearKeysDown
//   [ 13] 0x2c1ef0  LawnApp::UpdateFrames
//   [ 14] 0x33687c  Sexy::SexyAppBase::ReInitImages
//   [ 15] 0x3367f4  Sexy::SexyAppBase::DeleteNativeImageData
//   [ 16] 0x336838  Sexy::SexyAppBase::DeleteExtraImageData
//   [ 17] 0x339e30  Sexy::SexyAppBase::Delete3DImageData
//   [ 18] 0x2b2754  LawnApp::LoadingThreadCompleted
//   [ 19] 0x339e74  Sexy::SexyAppBase::Evict3DImageData
//   [ 20] 0x3366f4  Sexy::SexyAppBase::StartSensor
//   [ 21] 0x336704  Sexy::SexyAppBase::StopSensor
//   [ 22] 0x336c98  Sexy::SexyAppBase::PauseApp
//   [ 23] 0x336cec  Sexy::SexyAppBase::ResumeApp
//   [ 24] 0x3368c0  Sexy::SexyAppBase::CreateMusicInterface
//   [ 25] 0x2b262c  LawnApp::InitHook
//   [ 26] 0x33632c  Sexy::SexyAppBase::ShutdownHook
//   [ 27] 0x336460  Sexy::SexyAppBase::PreTerminate
//   [ 28] 0x2ba85c  LawnApp::LoadingThreadProc
//   [ 29] 0x2c1574  LawnApp::WriteToRegistry
//   [ 30] 0x2c163c  LawnApp::ReadFromRegistry
//   [ 31] 0x2b3ac4  LawnApp::NewDialog
//   [ 32] 0x2c23c8  LawnApp::PreDisplayHook
//   [ 33] 0x336714  Sexy::SexyAppBase::IsUIOrientationAllowed
//   [ 34] 0x336724  Sexy::SexyAppBase::UIOrientationChanged
//   [ 35] 0x336728  Sexy::SexyAppBase::GetUIOrientation
//   [ 36] 0x2af6f8  Sexy::SexyAppBase::OnFullVersionChange
//   [ 37] 0x336738  Sexy::SexyAppBase::LowMemoryWarning
//   [ 38] 0x52e7ac  Sexy::SexyCommonApp::AppEnteredBackground
//   [ 39] 0x52e7d8  Sexy::SexyCommonApp::AppEnteredForeground
//   [ 40] 0x336368  Sexy::SexyAppBase::BeginPopup
//   [ 41] 0x336378  Sexy::SexyAppBase::EndPopup
//   [ 42] 0x336388  Sexy::SexyAppBase::MsgBox
//   [ 43] 0x336398  Sexy::SexyAppBase::MsgBox
//   [ 44] 0x3363a8  Sexy::SexyAppBase::Popup
//   [ 45] 0x3363b8  Sexy::SexyAppBase::Popup
//   [ 46] 0x336364  Sexy::SexyAppBase::LogScreenSaverError
//   [ 47] 0x2c28e0  LawnApp::SafeDeleteWidget
//   [ 48] 0x2c20ec  LawnApp::URLOpenFailed
//   [ 49] 0x2c2274  LawnApp::URLOpenSucceeded
//   [ 50] 0x2c228c  LawnApp::OpenURL
//   [ 51] 0x3366a8  Sexy::SexyAppBase::OpenRegisterPage
//   [ 52] 0x33dea4  Sexy::SexyAppBase::OpenRegisterPage
//   [ 53] 0x3362e0  Sexy::SexyAppBase::GetProductVersion
//   [ 54] 0x336308  Sexy::SexyAppBase::SEHOccured
//   [ 55] 0x33a8a0  Sexy::SexyAppBase::GetGameSEHInfo
//   [ 56] 0x336328  Sexy::SexyAppBase::GetSEHWebParams
//   [ 57] 0x2c1554  LawnApp::Shutdown
//   [ 58] 0x33678c  Sexy::SexyAppBase::Quit
//   [ 59] 0x33a358  Sexy::SexyAppBase::FormatHelpString
//   [ 60] 0x33c358  Sexy::SexyAppBase::AddParameterEntry
//   [ 61] 0x33c388  Sexy::SexyAppBase::AddParameterEntries
//   [ 62] 0x338ee8  Sexy::SexyAppBase::DoParseCmdLine
//   [ 63] 0x33785c  Sexy::SexyAppBase::ParseCmdLine
//   [ 64] 0x3375a8  Sexy::SexyAppBase::ParseCmdLine
//   [ 65] 0x2c1ea8  LawnApp::HandleCmdLineParam
//   [ 66] 0x3363c8  Sexy::SexyAppBase::HandleNotifyGameMessage
//   [ 67] 0x338298  Sexy::SexyAppBase::HandleGameAlreadyRunning
//   [ 68] 0x336464  Sexy::SexyAppBase::Startup
//   [ 69] 0x2c1e9c  LawnApp::Start
//   [ 70] 0x336484  Sexy::SexyAppBase::Terminate
//   [ 71] 0x2bb260  LawnApp::Init
//   [ 72] 0x52f044  Sexy::SexyCommonApp::Cleanup
//   [ 73] 0x33649c  Sexy::SexyAppBase::PreDDInterfaceInitHook
//   [ 74] 0x3364a0  Sexy::SexyAppBase::PostDDInterfaceInitHook
//   [ 75] 0x2b2618  LawnApp::ChangeDirHook
//   [ 76] 0x3364b0  Sexy::SexyAppBase::PlaySample
//   [ 77] 0x3364cc  Sexy::SexyAppBase::PlaySample
//   [ 78] 0x3364f8  Sexy::SexyAppBase::PlaySampleSingle
//   [ 79] 0x336538  Sexy::SexyAppBase::PlaySampleSingle
//   ... 80..167 are SexyAppBase / SexyCommonApp slots, unchanged from the
//       binary's 178-slot table (0x140..0x29C), see libGameMain115_rtti.c.
//   [166] 0x2af775  Sexy::GamepadApp::DeviceAdded
//   [167] 0x2af719  Sexy::GamepadApp::DeviceRemoved
//   [168] 0x2b265c  LawnApp::GamepadConnected        (own)
//   [169] 0x2b2660  LawnApp::GamepadDisconnected     (own)
//   [170] 0x2b9e44  LawnApp::LoadingCompleted        (own)
//   [171] 0x2b2714  LawnApp::PlaySample(int,bool)    (own)
//   [172] 0x2b2658  LawnApp::DrawPost                (own)
//   [173] 0x2b2838  LawnApp::OnButtonDown            (own)
//   [174] 0x2b2710  LawnApp::ShowResourceError       (own)
//   [175] 0x2b2630  LawnApp::GetProfileVersion       (overrides ProfileEventListener[0])
//   [176] 0x2b2724  LawnApp::CreatePlayerInfo        (overrides ProfileEventListener[1])
//   [177] 0x2b2640  LawnApp::NotifyProfileChanged    (overrides ProfileEventListener[2])
//   [178] 0x2b9888  LawnApp::OnProfileSaveError      (overrides ProfileEventListener[3])
//   [179] 0x2b56b0  LawnApp::onPay                   (overrides IGameCenter::Listener[9])
// (slots 168..177 sit at 0x2A0..0x2C4 in the binary; the ordinal shift above is
// the ARM destructor's extra slot. The last five entries are not new slots of
// this class: the binary reaches them through the ProfileEventListener sub-vtable
// at 0x687DF0 and the IGameCenter::Listener one at 0x687E08, and GCC simply emits
// those groups next to the primary table in the same _ZTV7LawnApp object.)

#pragma once

#include "ReconTypes.h"
// Sexy::ProfileEventListener, this class's second base. It used to be declared
// inline below; it moved to its own header so Sexy\ProfileMgr.cpp can use it.
#include "ProfileEventListener.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "AlmanacDialog.h"
#include "AuthManager.h"
#include "AwardScreen.h"
#include "Buffer.h"
#include "BuyMoneyDialog.h"
#include "Challenge.h"
#include "ChallengeScreen.h"
#include "CheatCodeDialog.h"
#include "CheatDialog.h"
#include "ConfirmBackToMainDialog.h"
#include "ContinueDialog.h"
#include "CreditScreen.h"
#include "CutScene.h"
#include "DaveHelp.h"
#include "DrRpcEngine.h"
#include "EffectSystem.h"
#include "GameOverDialog.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "HelpOptionsDialog.h"
#include "HelpTextScreen.h"
#include "Image.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"
#include "LawnSession.h"
#include "LawnUser.h"
#include "Level.h"
#include "LevelStats.h"
// Defines Sexy::IGameCenter::Listener, this class's third base. Its twelve slots
// are exactly the ones the binary keeps at 0x687E08 for LawnApp.
#include "Listener.h"
#include "LogComposer.h"
#include "LogManager.h"
#include "Mailbox.h"
#include "MainMenu.h"
#include "MaskHelpWidget.h"
#include "Music.h"
#include "Music2.h"
#include "NewOptionsDialog.h"
#include "NewUserDialog.h"
#include "PerfTimer.h"
#include "PlayerInfo.h"
#include "PoolEffect.h"
#include "PurchaseLimitDialog.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "ReanimationWidget.h"
#include "ReanimatorCache.h"
#include "ResourceManager.h"
#include "SavingFileDialog.h"
#include "SavingProfileDialog.h"
#include "SeedBank.h"
#include "SeedChooserScreen.h"
#include "ServerConfig.h"
#include "SessionWaitingDialog.h"
#include "SettingsDialog.h"
#include "SexyAppBase.h"
#include "SexyCommonApp.h"
#include "SlideInOutWidgetAnimator.h"
#include "StoreScreen.h"
#include "TitleScreen.h"
#include "TodFoley.h"
#include "TodHesitationBracket.h"
#include "TodParticleHolder.h"
#include "TodParticleSystem.h"
#include "TypingCheck.h"
#include "UserDialog.h"
#include "VSResultsMenu.h"
#include "VSSetupMenu.h"
#include "WaitForLeaderboardsDialog.h"
#include "WaitForRemoteFileDialog.h"
#include "Widget.h"
#include "XMLParser.h"
#include "ZenGarden.h"
#include "Zombie.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// Instructions and globals this file needs but no header in the tree provides.
// They are declared rather than included so that LawnApp.h's transitive
// dependencies stay exactly what they were.
class SaveGameContext;
class UserProfile;

// Board is stored and returned only as a pointer (`Board *mBoard` at +0x8A0,
// `Board *ShowHelpTextScreen(int)`, `Board *TryHelpTextScreen(int)`), so it is
// declared rather than included. Board.h includes this header, so the pair was a
// two-header cycle -- Board.h -> LawnApp.h -> Board.h -- and a translation unit
// entering from Board.h saw an incomplete LawnApp. No member moves: a pointer is
// four bytes whatever the pointee's definition is, so sizeof(LawnApp) is
// unchanged and the layout assert at the bottom of this file still holds.
class Board;

// Every name below is used in this header only as a pointer: as a member, as a
// parameter or as a return type. None of them is a base class and none is held by
// value, so a declaration is enough for all of them and a missed include can no
// longer turn into `C2143: missing type specifier` followed by a wrong
// sizeof(LawnApp). The includes above are still what supplies the definitions;
// these lines only make the requirement explicit. This was the failure mode that
// pushed `DaveHelp *mDaveHelp` out of the object and broke
// `static_assert(sizeof(LawnApp) == 0xB28)`.
class AlmanacDialog;
class ChallengeScreen;
class CheatCodeDialog;
class ConfirmBackToMainDialog;
class CreditScreen;
class DaveHelp;
class EffectSystem;
class HelpBarWidget;
class HelpTextScreen;
class LawnPlayerInfo;
class Mailbox;
class MainMenu;
class MaskHelpWidget;
class Music2;
class PoolEffect;
class ReanimatorCache;
class Reanimation;
class SeedChooserScreen;
class SessionWaitingDialog;
class StoreScreen;
class TitleScreen;
class TodFoley;
class TodParticleSystem;
class TypingCheck;
class VSResultsMenu;
class VSSetupMenu;
class ZenGarden;

namespace Sexy {
class DefaultPlayerInfo;
class DefaultProfileMgr;

// ---------------------------------------------------------------------------
// Sexy::ProfileEventListener -- LawnApp's second base, the callback interface
// Sexy::ProfileMgr / Sexy::DefaultProfileMgr hand a profile change back through.
// The four slots are documented in Sexy\ProfileEventListener.h, which is where
// the declaration now lives (Sexy\ProfileMgr.cpp needs the complete type too).
// ---------------------------------------------------------------------------

} // namespace Sexy

class LawnSessionTask;

// CrazyDaveState -- the type of mCrazyDaveState at +0x948. Taken verbatim from
// the authoritative header PvZ\Lawn\Common\ConstEnums.h:225. No header in this
// tree defines it (Sexy\ReconTypes.h carries the other ConstEnums.h enums but not
// this one), and the member cannot stay untyped, so the enum is declared here.
// TODO(recon): move it into Sexy\ReconTypes.h next to BoardResult/FoleyType and
// drop this copy -- that is where every other ConstEnums.h enum lives.
enum CrazyDaveState {
    CRAZY_DAVE_OFF = 0,
    CRAZY_DAVE_ENTERING = 1,
    CRAZY_DAVE_LEAVING = 2,
    CRAZY_DAVE_IDLING = 3,
    CRAZY_DAVE_TALKING = 4,
    CRAZY_DAVE_HANDING_TALKING = 5,
    CRAZY_DAVE_HANDING_IDLING = 6,
};

// ListNodeBase -- the +0x0 next / +0x4 prev node the binary embeds twice, for
// mControlButtonList (ordinals 567~568 -> +2268) and mCreatedImageList
// (ordinals 569~570 -> +2276).
struct ListNodeBase
{
    ListNodeBase *next; // +0x00
    ListNodeBase *prev; // +0x04
};

// QueryCoinState -- the 16-byte block at +2836 the coin-query code keeps:
// mPending is the byte at +2836, mUserId the word at +2848 (atoi result).
struct QueryCoinState
{
    bool mPending;                 // +0x00
    unsigned int mLastRequestTime; // +0x04
    int mRequestCount;             // +0x08
    int mUserId;                   // +0x0C
};

class LawnApp : public Sexy::GamepadApp, public Sexy::ProfileEventListener, public Sexy::IGameCenter::Listener
{
    // Statics the binary keeps in this class's own storage; the
    // transcribed bodies name them through the qualified spelling.
public:
    // LawnApp::FULLSCREEN_RECT @0x715b80 -- mX/mY/mWidth/mHeight.
    // Nine files reference it, so it is public.
    static Sexy::TRect<int> FULLSCREEN_RECT;

    // LawnApp::MP_FLOWERPOT_TEST / MP_GRAVESTONE_TARGETS_TEST -- class-scope
    // debug toggles in the binary (_ZN7LawnApp17MP_FLOWERPOT_TESTE @0x715b7c and
    // _ZN7LawnApp26MP_GRAVESTONE_TARGETS_TESTE @0x715b7b). Both are BYTES: every
    // access is LDRB/STRB, and MainMenu::KeyChar toggles them with `^= 1u`.
    // Their data is all 0xFF, so they start non-zero (enabled).
    static unsigned char MP_FLOWERPOT_TEST;
    static unsigned char MP_GRAVESTONE_TARGETS_TEST;

    // -----------------------------------------------------------------------
    // Virtuals. Only the destructor and the slots the base chain does not have.
    // Ordered exactly as the binary's 178-slot table lists them.
    // -----------------------------------------------------------------------

    // Slots 0/1 in the binary; MSVC emits one. LawnApp.cpp has no out-of-line
    // destructor, so it is defaulted here to give the vtable its slot.
    virtual ~LawnApp() = default;

    // -----------------------------------------------------------------------
    // Binary slots 168..179, in order.
    //
    // 168/169 override Sexy::GamepadApp's own virtuals (GamepadApp.h:107/108).
    // 170..174 are the only genuinely new virtuals this class adds. The
    // authoritative LawnAppVTable types all four `void`; the bodies follow it, so
    // none of them returns the transcription's `int` any more.
    // 171 is `PlaySampleWithLoopFlag` in the authoritative table -- it is the
    // two-argument `PlaySample(int, bool)` here, which HIDES rather than
    // overrides `SexyAppBase::PlaySample(int)`.
    // 175..179 override the two listener bases.
    // -----------------------------------------------------------------------
    virtual void GamepadConnected();                                       // [168]
    virtual void GamepadDisconnected();                                    // [169]
    virtual void LoadingCompleted();                                       // [170]
    virtual void PlaySample(int theSoundId, bool theLoop);                 // [171]
    virtual void DrawPost();                                               // [172]
    // `void OnButtonDown(LawnApp*, int button, int playerIndex, unsigned int flags)`
    // in the authoritative table: the pointer the binary leaves in r0 is not a
    // return value, it is simply the register the widget search ended in.
    virtual void OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags); // [173]
    virtual void ShowResourceError(bool theExit);                          // [174]
    // Overrides of Sexy::ProfileEventListener, the interface declared above --
    // its slots [0]..[3] in that order.
    virtual int GetProfileVersion();                                       // [175]
    virtual LawnPlayerInfo *CreatePlayerInfo();                            // [176]
    virtual void NotifyProfileChanged(Sexy::PlayerInfo *theProfile);       // [177]
    virtual void OnProfileSaveError(Sexy::PlayerInfo *theProfile);         // [178]
    // Overrides Sexy::IGameCenter::Listener::onPay (Listener.h, slot [9]).
    virtual void onPay(int theResult);                                     // [179]

    // -----------------------------------------------------------------------
    // Overrides of the base chain, ordered as in the binary's table. Every return
    // type below is the base declaration's, so each one is a real override:
    // ButtonListener.h 42/44, SexyAppBase.h (see the per-line slot comments) and
    // SexyCommonApp.h 57/58 for HandleCmdLineParam and Init.
    //
    // TODO(recon): the three entries marked below are NOT overrides yet -- their
    // transcribed name or parameter list matches no base slot, so each declares a
    // hidden overload instead and the binary's slot keeps the base implementation.
    // They are left for phase 2 because fixing them means renaming the .cpp
    // definitions and their call sites, not just a return type.
    // -----------------------------------------------------------------------
    virtual void ButtonPress(int theId);           // [  2]
    virtual void ButtonDepress(int theId);         // [  4]  void in ButtonListener.h:44
    virtual void EnforceCursor();                  // [ 11]
    virtual void UpdateFrames();                   // [ 13]
    virtual void LoadingThreadCompleted();         // [ 18]
    virtual void InitHook();                       // [ 25]
    virtual void LoadingThreadProc();              // [ 28]
    virtual void WriteToRegistry();                // [ 29]
    virtual void ReadFromRegistry();               // [ 30]
    virtual Sexy::Dialog *NewDialog(int theDialogId, bool theModal, Sexy::String const &theHeader, Sexy::String const &theLines, Sexy::String const &theFooter, int theButtonMode); // [ 31]
    virtual void PreDisplayHook();                 // [ 32]
    virtual void SafeDeleteWidget(Sexy::Widget *theWidget); // [ 47]
    virtual void URLOpenFailed(Sexy::String const &theURL); // [ 48]
    // The base takes the URL (`void URLOpenSucceeded(const Sexy::String &)`,
    // SexyAppBase.h:326) and the binary's own symbol is
    // `_ZN7LawnApp16URLOpenSucceededERKSs`, so this is an override.
    virtual void URLOpenSucceeded(Sexy::String const &theURL); // [ 49]
    virtual bool OpenURL(Sexy::String const &theURL, bool theShutdownOnOpen); // [ 50]
    virtual void Shutdown();                       // [ 57]
    virtual void HandleCmdLineParam(Sexy::String const &theParamName, Sexy::String const &theParamValue); // [ 65]
    int GetFrameCounter() override { return mAppCounter; }
    virtual void Start();                          // [ 69]
    virtual void Init();                           // [ 71]
    virtual bool ChangeDirHook(char const *theIntendedPath); // [ 75]
    // TODO(recon): the base's three-argument slot is
    // `SwitchScreenModeFull(bool windowed, bool use3D, bool force)`
    // (SexyAppBase.h) and byte 0x190 of the binary's table belongs to LawnApp, so
    // this is that function. The name now matches the base, which makes it a real
    // override instead of a hidden `SwitchScreenMode` overload.
    virtual void SwitchScreenModeFull(bool theWindowed, bool theUse3D, bool theForce); // [101]
    virtual Sexy::Dialog *DoDialog(int theDialogId, bool theModal, Sexy::String const &theHeader, Sexy::String const &theLines, Sexy::String const &theFooter, int theButtonMode); // [103]
    // Slot 107 (byte 0x1AC) is LawnApp's `KillDialog(int)`, NOT a
    // `KillDialogById`. The vtable says so directly: `_ZTV7LawnApp` slot 107 holds
    // `_ZN7LawnApp10KillDialogEi`, and the two neighbours are
    // SexyAppBase::KillDialog(int,bool,bool) at 106 and
    // SexyAppBase::KillDialog(Dialog*) at 108. The binary exports no
    // `_ZN7LawnApp12KillDialogByIdEi` at all.
    //
    // A `virtual bool KillDialogById(int)` used to be declared here. Because it
    // has the same signature as SexyAppBase's own by-id slot it became an
    // implicit override, MSVC emitted it into the vtable, and the linker then
    // wanted a definition the binary does not have. The real override,
    // `KillDialog(int)`, is declared below with the rest of the class's methods
    // and defined in LawnApp.cpp:9455.
    // Callers in other files still spell the by-id kill `KillDialog(id)`. It is
    // not a slot of its own -- the binary's only entry is 0x1AC above -- so this
    // stays a non-virtual forwarder until those call sites are rewritten.
    // TODO(recon): rename the callers and delete this overload.
    int KillDialog(int theDialogId);
    virtual void ModalOpen();                      // [108]
    virtual void ModalClose();                     // [109]
    virtual void GotFocus();                       // [112]
    virtual void LostFocus();                      // [113]
    virtual void CloseRequestAsync();              // [120]
    virtual void PreDrawScreen();                  // [123]
    virtual bool DrawDirtyStuff();                 // [124]
    virtual bool UpdateAppStep(bool *theUpdated);  // [128]
    virtual bool UpdateApp();                      // [129]
    virtual bool ProcessMessage(Sexy::Event &theEvent); // [132]
    virtual void AuthFinished(bool theSuccess);    // [144]
    virtual void StartAuth();                      // [145]
    // GamepadConnected/GamepadDisconnected/LoadingCompleted/PlaySample(int,bool)
    // /DrawPost/OnButtonDown/ShowResourceError/GetProfileVersion/
    // CreatePlayerInfo/NotifyProfileChanged/OnProfileSaveError/onPay are declared
    // in the 168..179 block above.

    // -----------------------------------------------------------------------
    // Non-virtual methods. Unchanged from the transcription.
    // -----------------------------------------------------------------------
    int DoCheatDialog();
    int DoCheatCodeDialog();
    int GetCurrentChallengeIndex();
    const ChallengeDefinition* GetCurrentChallengeDef();
    bool IsAdventureMode();
    bool IsSurvivalMode();
    bool IsCoopMode();
    int IsTwoPlayerGame();
    bool IsTwinSunbankMode();
    bool IsPuzzleMode();
    bool IsChallengeMode();
    int IsArtChallenge();
    bool IsSquirrelLevel();
    bool IsIZombieLevel();
    bool IsShovelLevel();
    int IsWallnutBowlingLevel();
    bool IsSlotMachineLevel();
    int IsWhackAZombieLevel();
    bool IsLittleTroubleLevel();
    int IsScaryPotterLevel();
    bool IsSeedPlantingLevel();
    bool IsStormyNightLevel();
    bool IsBungeeBlitzLevel();
    bool IsMiniBossLevel();
    bool IsFinalBossLevel();
    bool IsContinuousChallenge();
    int IsChallengeWithoutSeedBank();
    int IsNight();
    int IsIceDemo();
    int IsRegistered();
    int IsFullVersion();
    int IsDRMConnected();
    int IsTrialStageLocked();
    bool IsOptionsScreenActive();
    bool IsFirstTimeAdventureMode();
    int EndLevel();
    int SetBoardResult(BoardResult theResult);
    int showRechargeBoard(Sexy::String theAmount);
    // The dialog that was showing, or null when that id had none.
    Sexy::Dialog* DoDialogDelay(int theDialogId);
    int DoUserDialog();
    int FinishUserDialog(bool theYes);
    int DoBuyMoneyDialog(const Sexy::String &theMessage);
    int FinishBuyMoneyDialog(bool theYes);
    int DoCreateUserDialog();
    void FinishCreateUserDialog(bool theYes);
    bool FinishCheatCodeDialog(bool theYes);
    void DoConfirmDeleteUserDialog(Sexy::String const &theName);
    UserDialog *FinishConfirmDeleteUserDialog(bool theYes);
    int DoRenameUserDialog(Sexy::String const &theName);
    UserDialog *FinishRenameUserDialog(bool theYes);
    int FinishNameError(int theId);
    int KillNewOptionsDialog();
    int KillAlmanacDialog();
    EffectSystem *KillAll();
    bool NeedPauseGame();
    int NeedShowQRCode();
    void StepAdvance();
    int ToggleSlowMo();
    int ToggleFastMo();
    int ToggleStep();
    void UpdatePlayTimeStats();
    void TraceLoadGroup(char const *theGroupName, int theGroupTime, int theTotalGroupWeight, int theTaskWeight);
    void LoadGroup(char const *theGroupName, int theGroupAveMsToLoad);
    // Title-screen load. Prepare reads the loader-bar group so the roll can
    // draw, then each step pulls the next slice of LoadingImages / fonts /
    // sounds and the work the bar is counting. The bar finishes only after
    // the last step sets mLoadingThreadCompleted.
    void PrepareTitleLoad();
    bool StepTitleLoad();
    // _ZN7LawnApp11ConfirmQuitEv returns what DoDialog returned, i.e. the
    // created Sexy::Dialog (a LawnDialog), not an int.
    Sexy::Dialog *ConfirmQuit();
    void ConfirmCheckForUpdates();
    void CheckForUpdates();
    // PlayFoley(FoleyType) -- mangled _ZN7LawnApp9PlayFoleyE9FoleyType, so the
    // parameter IS declared even though the 16-byte body at 0x2b507c never reads
    // r1 (it forwards *(this+2264) to TodFoley::PlayFoley). Call sites that omit
    // the argument need to pass it, not the other way round.
    FoleyTypeDataElement* PlayFoley(FoleyType theFoley);
    FoleyTypeDataElement* PlayFoleyPitch(FoleyType theFoley, float thePitch);
    // _ZN7LawnApp14GetStageStringEi @0x2b509c -- sret: ARM passes the by-value
    // String result's destination in r0, so IDA prints it as the receiver and
    // the real argument arrives in r1. The body is
    // `Sexy::StrFormat(this, "%d-%d", v6, v7 + a3); return this;`.
    Sexy::String GetStageString(int theLevel);
    int SrvLogoutFromServer();
    int SrvQueryCoin();
    void OnSessionTaskFailed(int theTask, Sexy::String theMessage);
    int OnSessionTaskSucceed();
    SessionWaitingDialog *DoSessionWaitDialog();
    bool SrvBuyTool(E_SHOP_PACKET thePacket);
    bool SrvDebugAddMoney(int theAmount);
    // @0x2B5AE4 is not an sret. r0 on entry is this and is unused; the word
    // left in r0 is the path string's destructor result, and the only caller
    // overwrites it with 1.
    void writeCoinNum(int theCoinNum);
    // `_ZN7LawnApp11GetUserNameEv`. Returns the platform identifier, or
    // "transmension" when that identifier is empty. The empty case also clears
    // mCanDoBuyMoneyDialog (+0xB24).
    Sexy::String GetUserName();
    bool judgeDoTransRecharge();
    int showPurchaseLimitDialog(int theLimit);
    int GetAwardSeedForLevel(int theLevel);
    // @0x2B60DC is a ten-byte tail call: `return ReanimationHolder::
    // AllocReanimation(...)`, which allocates a 220-byte record and hands back
    // its address. A pointer, not an index.
    Reanimation * AddReanimation(float theX, float theY, int theRenderOrder, ReanimationType theType);
    // AddTodParticle(float,float,int,ParticleEffect) -- the demangled signature
    // in the binary's vtable listing at 0x2B60E8. The transcription had lost
    // the parameter list.
    TodParticleSystem* AddTodParticle(float theX, float theY, int theLayer, ParticleEffect theEffect);
    ReanimationID ReanimationGetID(Reanimation *theReanimation);
    // _ZN7LawnApp13ReanimationGetE13ReanimationID @0x2b60fc -- the body returns
    // `*v3 + 220 * index`, the address of a 220-byte Reanimation record inside
    // the holder's array, so the result is a POINTER. The transcription typed it
    // `int`; the two callers that wrote `(Reanimation *)` around the result are
    // unchanged by the fix.
    Reanimation * ReanimationGet(ReanimationID theReanimationId);
    // _ZN7LawnApp19ReanimationTryToGetE13ReanimationID @0x2b6110. The body
    // returns `*v3 + 220 * index`, the address of a 220-byte Reanimation record
    // inside the holder's array, so the result is a POINTER.
    //
    // Typed as such now. Every existing call site writes `(Reanimation *)` or
    // `reinterpret_cast<Reanimation *>` around the result, and an identity cast
    // from `Reanimation *` to `Reanimation *` still compiles, so flipping the
    // type costs nothing at the call sites and removes the last reason to think
    // the value is an index.
    Reanimation * ReanimationTryToGet(ReanimationID theReanimationId);
    ParticleSystemID ParticleGetID(TodParticleSystem *theParticle);
    TodParticleSystem* ParticleGet(ParticleSystemID theId);
    TodParticleSystem* ParticleTryToGet(ParticleSystemID theId);
    Reanimation *RemoveReanimation(ReanimationID theReanimationId);
    TodParticleSystem *RemoveParticle(ParticleSystemID theParticleId);
    bool IsSurvivalNormal(GameMode theGameMode);
    bool IsSurvivalHard(GameMode theGameMode);
    bool IsSurvivalEndless(GameMode theGameMode);
    bool IsEndlessScaryPotter(GameMode theGameMode);
    bool IsEndlessIZombie(GameMode theGameMode);
    bool HasBeatenChallenge(GameMode theGameMode);
    PottedPlant* GetPottedPlantByIndex(int theIndex);
    bool HasFinishedAdventure();
    int CanShowAlmanac();
    bool CanShowStore();
    int GetSeedsAvailable(bool theIncludeImitater);
    bool CanSpawnYetis();
    int CrazyDaveEnter();
    void CrazyDaveDoneHanding();
    void CrazyDaveStopSound();
    Reanimation *CrazyDaveDie();
    void CrazyDaveLeave();
    void CrazyDaveTalkMessage(Sexy::String const &theMessage);
    Reanimation *CrazyDaveStopTalking();
    // _ZN7LawnApp15UpdateCrazyDaveEv @0x2b6da0 -- the binary's body ends in
    // `Reanimation::Update(v3)` and hands back the word that call leaves in r0;
    // Update is void, so this is a void function too.
    void UpdateCrazyDave();
    // _ZN7LawnApp13DrawCrazyDaveEPN4Sexy8GraphicsE -- the binary's own body ends
    // in `return Reanimation::Draw(...)`, but Reanimation::Draw is void, so the
    // float the dump shows in r0 is not a result this function produces.
    void DrawCrazyDave(Sexy::Graphics *g);
    // Pluralize(int, char const *, char const *) -- the symbol
    // `_ZN7LawnApp9PluralizeEiPKcS1_` returns its result through the ARM sret
    // pointer, i.e. a Sexy::String. The transcribed declaration spelled that
    // buffer `LawnApp *`. Game\LawnApp.cpp:4574 has to be retyped to match.
    Sexy::String Pluralize(int theCount, char const *theSingular, char const *thePlural);
    int GetNumTrophies(ChallengePage thePage);
    int TrophiesNeedForGoldSunflower();
    bool EarnedGoldTrophy();
    void BuyFullVersion();
    bool CanShowZenGarden();
    bool HasSeedType(SeedType theSeedType, bool theIncludeImitater);
    int GetNumPreloadingTasks();
    int PreloadForUser();
    // Both return the formatted line by value. GetMoneyString ignores its
    // receiver (the amount is the only input). GetCrazyDaveText reads
    // mPlayerInfo off this.
    Sexy::String GetMoneyString(int theAmount);
    Sexy::String GetCrazyDaveText(int theIndex);
    void CrazyDaveTalkIndex(int theMessageIndex);
    int AdvanceCrazyDaveText();
    int CanDoPinataMode();
    int CanDoDanceMode();
    int CanDoDaisyMode();
    int GetTitleID();
    void WriteLeaderboards();
    void WriteHouse(bool theForce);
    int ShowNetError();
    // @0x2B7DB0 stores the shared empty-string data pointer into the sret
    // buffer and ignores the id.
    Sexy::String GetLeaderboardName(unsigned int theId);
    int HasReplays(unsigned int theId);
    int NumReplays(unsigned int theId);
    // _ZN7LawnApp23HandleCorruptedGameFileEv @0x2b7dd0 and its three siblings:
    // each one's body ends in `return Board::Pause(...)`, but Board::Pause is
    // void, so none of them produces a value.
    void HandleCorruptedGameFile();
    void HandleOldGameFile();
    void HandleNoDevice();
    void HandleInvalidLevel();
    bool DoProfileLoadErrorDialog();
    int DoProfileSaveErrorDialog();
    int On2PProfileLoadError();
    int DoHelpDialog(bool theFromGameSelector);
    int DoSettingsDialog(bool theFromGameSelector);
    // The restart prompt is a plain `Sexy::Dialog` created by `DoDialog(24, ...)`
// and returned by id; ConfirmBackToMainDialog is a different dialog.
// _ZN7LawnApp22DoConfirmRestartDialogEv @0x2b86a4 returns the dialog the
// DoDialog call produced, and the body re-labels its two LawnDialog buttons.
Sexy::Dialog *DoConfirmRestartDialog();
    // Every Do*Dialog below ends in the `DoDialog(...)` call, and the binary
    // hands that call's return value straight back. The transcription typed them
    // `int` and stored `->mId` on the way out, which lost the dialog the callers
    // in StoreScreen.cpp and ZenGarden.cpp then dereference.
    Sexy::Dialog *DoNotEnoughMoneyDialog();
    Sexy::Dialog *DoUpgradeDialog();
    // _ZN7LawnApp22DoRestartConfirmDialogEv @0x2b8ba8 is the sibling of
    // DoConfirmRestartDialog above and returns its DoDialog result the same way.
    Sexy::Dialog *DoRestartConfirmDialog();
    Sexy::Dialog *DoTreeItemDialog();
    Sexy::Dialog *DoStoreItemDialog();
    Sexy::Dialog *DoZenSellDialog();
    Sexy::Dialog *DoPacketPurchaseDialog(int theCost, int theCount);
    void DoProfileChangeDialog();
    Sexy::Dialog *DoRetryLeaderboardDialog();
    Sexy::Dialog *DoRetryAchievementsDialog();
    // _ZN7LawnApp22DoContentSavingDialogEv @0x2b9580 and its loading sibling
    // build a SavingFileDialog and return it.
    SavingFileDialog *DoContentSavingDialog();
    SavingFileDialog *DoContentLoadingDialog();
    int DoRepickWarningDialog(char const *theMessage);
    int GamepadToPlayerIndex(unsigned int theGamepadIndex);
    int PlayerToGamepadIndex(int thePlayerIndex);
    int SetSecondPlayer(int theGamepadIndex);
    int CheckFullVersion(bool thePrompt);
    int WriteCurrentUserConfig();
    int WriteSecondUserConfig();
    int TryToSaveProfile();
    int UpdatePlayerProfileForFinishingLevel();
    void OnFullVersionLicenseChange();
    int ShowHelpBarWidget();
    Sexy::WidgetAnimator *HideHelpBarWidget();
    Sexy::WidgetAnimator *ForceHideHelpBarWidget();
    void CheckControllerConnections();
    int ClearSecondPlayer();
    int ShowMainMenuScreen();
    int ShowGameSelector();
    int DoBackToMain();
    int ShowTitleScreen(bool theFromGame);
    int DrawDialogFade(Sexy::Graphics *g);
    int DrawQRCode(Sexy::Graphics *g);
    void DoGameSaveErrorDialog();
    int ShowSavingDingus();
    int *HideSavingDingus();
    char **DrawSavingDingus(Sexy::Graphics *g);
    int UpdateSavingDingus();
    int SetDingusText(Sexy::String &theText);
    void DeleteSavedGame();
    int CanShopLevel();
    int ShopPadCanShow();
    void StartHelp(int theHelpIndex);
    void SdkExit();
    void setErrorCode(Sexy::String theErrorCode);
    void doTransRecharge();
    int SrvLoginToServer();
    void loadCoinNum();
    void BuyMoney(int theAmount);
    void setQueryBalanceRes(bool theSuccess, int theBalance);
    void setPayItemsRes(bool theSuccess, char const *theItemId, int theCount, char const *theOrderId);
    int setItemToCoinItems();
    int NextTestDialog();
    void ToggleTestDialog();
    void PrevTestDialog();
    int GotoBuyTool(E_SHOP_PACKET thePacket);
    int buyToolForDemo(E_SHOP_PACKET thePacket);
    int setItemToShopItems();
    int SetKeyValueData();
    int AddBuySunMoneyCount();
    int GetKeyValueData();
    int QueryKeyValues();
    void UpdateSessionState();
    LawnApp();
    void BetaAddFile(std::list<Sexy::String> &theUploadFileList, Sexy::String theFileName, Sexy::String theShortName);
    int TryLoadGame();
    void PreNewGame(GameMode theGameMode, bool theLookForSavedGame);
    void FinishCheatDialog(bool theYes);
    void FinishRestartConfirmDialog();
    void CheckForGameEnd();
    void FastLoad(GameMode theGameMode);
    int FinishLoadGame();
    // True when the PNG writer accepted the image.
    bool WriteImage(Sexy::String const &thePath, Sexy::Image *theImage);
    int HardwareInit();
    int LoadLevelConfiguration(int theLevel, int theStage);
    int SaveFileExists();
    int NeedRegister();
    void UpdateRegisterInfo();
    void BetaSubmit(bool theAskForComments);
    void BetaRecordLevelStats();
    bool CanPauseNow();
    int getLevelForPost();
    void PostEnterLevel();
    void PostLeaveLevel();
    void PostLevelSuccess();
    void PostLevelFailed();
    void TransitionToHouseScreen();
    void ShowLeaderboardScreen();
    void ShowHouseScreen();
    void KillHouseScreenFromGame();
    void ShowHouseScreenFromGame();
    void KillTitleScreen();
    int KillMainMenu();
    int KillGameSelector();
    Board *ShowHelpTextScreen(int theHelpIndex);
    char *KillHelpTextScreen();
    Board *TryHelpTextScreen(int theHelpIndex);
    void KillLeaderboardScreen();
    void KillHouseScreen();
    int ShowAwardScreen(AwardType theAwardType);
    int KillAwardScreen();
    int ShowCreditScreen(bool theFromMainMenu);
    int KillCreditScreen();
    int ShowChallengeScreen(ChallengePage thePage);
    int KillChallengeScreen();
    int KillStoreScreen();
    int ShowVSSetupScreen();
    int KillVSSetupScreen();
    int ShowVSResultsScreen();
    int KillVSResultsScreen();
    int ShowSeedChooserScreen();
    int KillSeedChooserScreen();
    int ShowZombieChooserScreen();
    int KillZombieChooserScreen();
    int StartPlaying();
    ConfirmBackToMainDialog *DoConfirmBackToMain(bool theFromGameSelector);
    int DoContinueDialog();
    int FinishTimesUpDialog();
    void FinishModelessDialogs();
    int KillBoard();
    int MakeNewBoard();
    int NewGame();
    StoreScreen *ShowStoreScreen();
    int DoNewOptions(bool theFromGameSelector, unsigned int thePlayerIndex);
    AlmanacDialog *DoAlmanacDialog(SeedType theSeedType, ZombieType theZombieType);
    void DoPauseDialog();
    int LawnMessageBox(int theDialogId, char const *theHeader, char const *theLines, char const *theButton1, char const *theButton2, int theButtonMode);

    // -----------------------------------------------------------------------
    // Members. +0x8A0 .. +0xB27, 0x288 bytes.
    //
    // Every offset below is absolute and was cross-checked against a raw-offset
    // access in Game/LawnApp.cpp; the restore tree's ordinal comments agree
    // (ordinal N -> byte 4*N) except for the last two, which it numbered one
    // too high.
    // -----------------------------------------------------------------------
    Board *mBoard;                          // +0x8A0 (552)
    TitleScreen *mTitleScreen;              // +0x8A4 (553)
    MainMenu *mGameSelector;                // +0x8A8 (554)
    // Two modal counters. Board::DrawOverlay skips its overlay while either is set.
    int mModalCount[2];                     // +0x8AC .. +0x8B3 (555~556)
    HelpTextScreen *mHelpTextScreen;        // +0x8B4 (557)
    // Cleared with the neighbouring screen pointers. Nothing reads it.
    int mLeaderboardScreen;                 // +0x8B8 (558)
    VSSetupMenu *mVSSetupMenu;              // +0x8BC (559)
    VSResultsMenu *mVSResultsMenu;          // +0x8C0 (560)
    SeedChooserScreen *mSeedChooserScreen;  // +0x8C4 (561)
    SeedChooserScreen *mZombieChooserScreen; // +0x8C8 (562)
    AwardScreen *mAwardScreen;              // +0x8CC (563)
    CreditScreen *mCreditScreen;            // +0x8D0 (564)
    ChallengeScreen *mChallengeScreen;      // +0x8D4 (565)
    TodFoley *mSoundSystem;                 // +0x8D8 (566)
    ListNodeBase mControlButtonList;        // +0x8DC .. +0x8E3 (567~568)
    ListNodeBase mCreatedImageList;         // +0x8E4 .. +0x8EB (569~570)
    Sexy::String mReferId;                  // +0x8EC (571)
    Sexy::String mRegisterLink;             // +0x8F0 (572)
    Sexy::String mMod;                      // +0x8F4 (573)
    bool mRegisterResourcesLoaded;          // +0x8F8 (byte 2296)
    bool mTodCheatKeys;                     // +0x8F9 (byte 2297)
    GameMode mGameMode;                     // +0x8FC (575)
    GameScenes mGameScene;                  // +0x900 (576)
    bool mLoadingZombiesThreadCompleted;    // +0x904 (byte 2308)
    bool mFirstTimeGameSelector;            // +0x905 (byte 2309)
    int mGamesPlayed;                       // +0x908 (578)
    int mMaxExecutions;                     // +0x90C (579)
    int mMaxPlays;                          // +0x910 (580)
    int mMaxTime;                           // +0x914 (581)
    bool mEasyPlantingCheat;                // +0x918 (byte 2328)
    PoolEffect *mPoolEffect;                // +0x91C (583)
    ZenGarden *mZenGarden;                  // +0x920 (584)
    EffectSystem *mEffectSystem;            // +0x924 (585)
    ReanimatorCache *mReanimatorCache;      // +0x928 (586)
    LawnPlayerInfo *mPlayerInfo;            // +0x92C (587)
    Sexy::DefaultPlayerInfo *mPlayer2Info;  // +0x930 (588), never initialised but tested
    // LevelStats* at +0x934. StartLevel resets it; the old int* was the dump's
    // untyped word.
    LevelStats *mLastLevelStats;            // +0x934 (589)
    bool mCloseRequest;                     // +0x938 (byte 2360)
    int mAppCounter;                        // +0x93C (591)
    Music *mMusic;                         // +0x940 (592)
    ReanimationID mCrazyDaveReanimID;       // +0x944 (593)
    CrazyDaveState mCrazyDaveState;         // +0x948 (594)
    int mCrazyDaveBlinkCounter;             // +0x94C (595)
    ReanimationID mCrazyDaveBlinkReanimID;  // +0x950 (596)
    int mCrazyDaveMessageIndex;             // +0x954 (597)
    // +0x958 (598). The bodies copy this string into a scratch Sexy::String,
    // hand it to the {TAG} replacers and clear it when a line-up ends, so it is
    // a real String and not the word the transcription typed it as.
    Sexy::String *mCrazyDaveMessageText;
    int mAppRandSeed;                       // +0x95C (599)
    // DrawDialogFade latches this and then draws the shade once.
    bool mDialogFadeDrawn;                  // +0x960 (600)
    Sexy::DefaultProfileMgr *mProfileMgr;   // +0x964 (601)
    bool mSkipProfileSaving;                // +0x968 (602 * 4)
    Sexy::Level mLevel;                     // +0x96C .. +0x9F3 (603~636, 34 words)
    Sexy::Image *mQRCodeImage;              // +0x9F4 (637)
    Sexy::Image *mQRCodeImageBackground;    // +0x9F8 (638)
    // Key/value pairs the session stores, such as "iBuySunMoneyCount". The
    // binary keeps a 24-byte libstdc++ map here.
    std::map<Sexy::String, int> mKeyValueData; // +0x9FC .. +0xA13 (639~644)
    int mInitialSunMoney;                   // +0xA14 (645), gives the player starting sun
    bool mIsFullVersion;                    // +0xA18 (byte 2584)
    int mPendingBuyToolId;                  // +0xA1C (647)
    int mPendingRechargeAmount;             // +0xA20 (648)
    // TODO(recon): TrialType does not exist in this tree yet; the restore tree
    // has it as an enum at ordinal 649.
    int mTrialType;                         // +0xA24 (649)
    int mVsInitialPlantMode;                // +0xA28 (650)
    // Launch time in seconds, then two counters Init leaves at 0.
    int mSessionStartSeconds;               // +0xA2C (651)
    int mSessionCounter[2];                 // +0xA30 .. +0xA37 (652~653)
    BoardResult mBoardResult;               // +0xA38 (654)
    bool mKilledYetiAndRestarted;           // +0xA3C (byte 2620)
    TypingCheck *mKonamiCheck;              // +0xA40 (656)
    TypingCheck *mMustacheCheck;            // +0xA44 (657)
    TypingCheck *mMoustacheCheck;           // +0xA48 (658)
    TypingCheck *mSuperMowerCheck;          // +0xA4C (659)
    TypingCheck *mSuperMowerCheck2;         // +0xA50 (660)
    TypingCheck *mFutureCheck;              // +0xA54 (661)
    TypingCheck *mPinataCheck;              // +0xA58 (662)
    TypingCheck *mDanceCheck;               // +0xA5C (663)
    TypingCheck *mDaisyCheck;               // +0xA60 (664)
    TypingCheck *mSukhbirCheck;             // +0xA64 (665)
    bool mMustacheMode;                     // +0xA68 (byte 2664)
    bool mSuperMowerMode;                   // +0xA69 (byte 2665)
    bool mFutureMode;                       // +0xA6A (byte 2666)
    bool mPinataMode;                       // +0xA6B (byte 2667)
    bool mDanceMode;                        // +0xA6C (byte 2668)
    bool mDaisyMode;                        // +0xA6D (byte 2669)
    bool mSukhbirMode;                      // +0xA6E (byte 2670)
    // Constructor stores 0. Nothing reads it.
    int mPlayTimeActiveSession;             // +0xA70 (668)
    bool mMuteSoundsForCutscene;            // +0xA74 (byte 2676)
    bool mNeedLoadGame;                     // +0xA75 (byte 2677)
    bool mNeedGoBackToMain;                 // +0xA76 (byte 2678)
    // TODO(recon): SaveGameOperation is an enum in the authoritative tree; this
    // tree has it only in PvZ\Lawn\Common\ConstEnums.h, which LawnApp.h must not
    // start including, so the field keeps the int spelling.
    int mSaveGameOperation;                 // +0xA78 (670)
    int mSecondPlayerGamepadIndex;          // +0xA7C (671)
    // +0xA80 (672) is never read or written. Left out.
    int mCurrentTestDialogId;               // +0xA84 (673)
    int mCurrentTestDaveMessage;            // +0xA88 (674)
    Mailbox *mMailBox;                      // +0xA8C (675)
    // +0xA90 (676) is never read or written. Left out.
    bool mMailboxRefreshed;                 // +0xA94 (byte 2708)
    ReanimationID mSavingDingusReanimationID; // +0xA98 (678)
    float mSavingDingusCurrentY;            // +0xA9C (679)
    float mSavingDingusAnimationStartY;     // +0xAA0 (680)
    float mSavingDingusAnimationEndY;       // +0xAA4 (681)
    float mSavingDingusAnimationTime;       // +0xAA8 (682)
    Sexy::String mDingusText;               // +0xAAC (683)
    int mSavingDingusShowCount;             // +0xAB0 (684)
    bool mProfileSaveInProgress;            // +0xAB4 (685[0])
    bool mProfileOperationPending1;         // +0xAB5 (685[1])
    bool mProfileOperationPending2;         // +0xAB6 (685[2])
    int mP2JoinPromptTimer;                 // +0xAB8 (686)
    // Constructor stores false. Nothing reads it.
    bool mProfileSaveFailed;                // +0xABC (byte 2748)
    SaveGameContext *mSaveGame;             // +0xAC0 (688)
    // TODO(recon): SessionTaskType is an enum in the authoritative tree, same
    // situation as mSaveGameOperation above.
    int mSessionTaskType;                   // +0xAC4 (689)
    bool mLoginToServer;                    // +0xAC8 (byte 2760)
    LawnSession mSession;                   // +0xACC .. +0xADF (691~695)
    LawnSessionTask *mLawnSessionTask;      // +0xAE0 (696)
    Sexy::String mGameInfoStrings[5];       // +0xAE4 .. +0xAF7 (697~701)
    HelpBarWidget *mHelpBarWidget;          // +0xAF8 (702)
    // StartPlaying clears this after the help bar is rebuilt. Nothing reads it.
    int mHelpIndex;                         // +0xAFC (703)
    LogComposer *mLogComposer;              // +0xB00 (704)
    // The log manager this slot holds is a TM::LogManager, NOT a
    // Sexy::LogManager. Two independent proofs from the binary:
    //   * LawnApp::HardwareInit @0x2C26C8 calls
    //     `_ZN2TM10LogManagerC1ERKSsSs` and stores the result at +0xB04.
    //   * Every reader -- PostEnterLevel @0x2B3186, PostLeaveLevel,
    //     PostLevelSuccess, PostLevelFailed, StartAuth, AuthFinished,
    //     UpdateSessionState -- loads `[this+0xB04]` and calls
    //     `_ZN2TM10LogManager7sendLogERKSs` on it.
    // The tree typed it Sexy::LogManager, which is a different class with its
    // own sendLog, so the call mangled to `?sendLog@LogManager@Sexy@@...` while
    // the linker wanted the TM one.
    TM::LogManager *MLogManager;            // +0xB04 (705)
    int mLaunchTime;                        // +0xB08 (706)
    DaveHelp *mDaveHelp;                    // +0xB0C (707)
    MaskHelpWidget *mMaskHelpWidget;        // +0xB10 (708)
    QueryCoinState mQueryCoinState;         // +0xB14 .. +0xB23 (709~712)
    bool mCanDoBuyMoneyDialog;              // +0xB24 (byte 2852)
    // size 0xB28
};

// Layout check. With the real inheritance chain the object is
//     Sexy::GamepadApp 0x898 + ProfileEventListener 4 + IGameCenter::Listener 4
//     + the 0x288 bytes of own members = 0xB28,
// which is exactly what main (0x131E40) and GameInit (0x2C437C) allocate, and it
// puts mBoard at +0x8A0 -- the offset every `this + 2208` access in the
// transcribed bodies uses. Both halves were measured: sizeof(Sexy::GamepadApp)
// is 0x898, each listener base is 4, and mBoard/.../mSession land on their
// commented offsets.
//
// That fix has landed: Game\LawnSession.h is 0x14 now, because Sexy\Thread.h
// declares the binary's `Sexy::Thread` (a pthread_t plus a validity flag, 8 bytes)
// next to TM::Thread (12 bytes) and LawnSession uses the former. So this is a hard
// check with no second term.


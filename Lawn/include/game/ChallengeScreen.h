// ChallengeScreen -- reconstructed from libGameMain.so.
//
// Polymorphic class with two polymorphic bases:
//   class ChallengeScreen : public Sexy::Widget, public Sexy::ButtonListener
// The binary's RTTI base list for _ZTV15ChallengeScreen is exactly
// N4Sexy6WidgetE plus N4Sexy14ButtonListenerE, and the second base's vptr is the
// word at +0x100, which is where the ButtonListener subobject starts.
//
// Object size 0x5F8. The member table below runs to +0x5F4 and the reference tree
// calls the base 380 ints (0x5F0) plus the two porting-only pointers, which agrees.
// The transcribed object was 0x600, four bytes longer than the real one.
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x18a0c4  ChallengeScreen::~ChallengeScreen
//   [  1] 0x18a1d8  ChallengeScreen::~ChallengeScreen
//   [ 29] 0x18c684  ChallengeScreen::AddedToManager
//   [ 30] 0x18c6b0  ChallengeScreen::RemovedFromManager
//   [ 31] 0x18c540  ChallengeScreen::Update
//   [ 36] 0x18aee4  ChallengeScreen::Draw
//   [ 61] 0x18c6dc  ChallengeScreen::GotFocus
//   [ 67] 0x18bc2c  ChallengeScreen::KeyChar
//   [ 71] 0x18bd0c  ChallengeScreen::KeyDown
// plus, in the ButtonListener subobject:
//   [122] 0x18a098  ChallengeScreen::ButtonPress
//   [123] 0x18a330  ChallengeScreen::ButtonDepress
//
// MouseDown and MouseUp are deliberately NOT virtual here. The community
// restoration lists them as overrides, but the binary's slots [76]/[77] and
// [79] still hold Sexy::Widget::MouseDown / Sexy::Widget::MouseUp, and no
// _ZN15ChallengeScreen8MouseDownEiii symbol exists.
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "ButtonListener.h"
#include "Curve1DUtil.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "ToolTipWidget.h"
#include "UnlockDialog.h"
#include "Widget.h"
#include "WidgetContainer.h"

// LawnPlayerInfo is not named in this file, so the include was pure cycle: it was
// the last edge of LawnPlayerInfo.h -> GameStats.h -> SeedBank.h -> CutScene.h ->
// ChallengeScreen.h -> LawnPlayerInfo.h. LawnApp is reached only through
// `LawnApp *mApp` and the constructor's pointer parameter.
// Sexy\ReconTypes.h already carries the LawnApp declaration.
class LawnPlayerInfo;
class LawnApp;

class NewLawnButton;
class WaitForSecondPlayerDialog;
namespace Sexy { class ButtonWidget; }

// NUM_CHALLENGE_MODES = GameMode::NUM_GAME_MODES - 1. The authoritative header
// derives it from the GameMode enum (95 modes, so 94 challenge modes); this tree
// spells GameMode as `typedef int`, so the value is written out. It sizes the
// three button/page arrays below and matches the reference tree's
// "65 ~ 158", "188 ~ 281" and "282 ~ 375" ordinal spans, which are 94 entries each.
inline constexpr int NUM_CHALLENGE_MODES = 94;

class ChallengeScreen : public Sexy::Widget, public Sexy::ButtonListener
{
public:
    enum {
        ChallengeScreen_Back = 100,
        ChallengeScreen_Mode = 200,
        ChallengeScreen_Page = 300,
    };

    virtual ~ChallengeScreen();
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);  // slot 29
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // slot 30
    virtual void Update();                                  // slot 31
    virtual void Draw(Sexy::Graphics *theGraphics);         // slot 36
    virtual void GotFocus();                                // slot 61
    virtual int KeyChar(char theChar);                      // slot 67
    virtual int KeyDown(Sexy::KeyCode theKey);              // slot 71
    virtual void ButtonPress(int theId);                    // ButtonListener slot 2
    virtual void ButtonDepress(int theId);                  // ButtonListener slot 4

    // +0x104 .. +0x5F7 (the first 0x100 bytes are Sexy::Widget, and the
    // ButtonListener subobject is the word at +0x100; this class's own members
    // therefore begin at +0x104).
    Sexy::ButtonWidget *mChallengeButtons[NUM_CHALLENGE_MODES]; // +0x104 .. +0x27B
    LawnApp *mApp;                       // +0x27C
    ToolTipWidget *mToolTip;             // +0x280
    ChallengePage mPage;                 // +0x284
    bool mCheatEnableChallenges;         // +0x288
    UnlockingState mUnlockState;         // +0x28C
    int mUnlockStateCounter;             // +0x290
    int mUnlockChallengeIndex;           // +0x294
    float mLockShakeX;                   // +0x298
    float mLockShakeY;                   // +0x29C
    // ChallengeScreen.cpp:1405 constructs a Sexy::Curve1DUtil here and the
    // reference tree calls the span ordinals 168..183, i.e. 16 words.
    // TODO(recon): Curve1DUtil.h still declares 0x50 bytes; the 0x40 this span
    // needs (and the 0x20 StoreScreen uses at +0x384) have to be reconciled
    // when that header is reconstructed.
    Sexy::Curve1DUtil mUtil;             // +0x2A0 .. +0x2DF
    HelpBarWidget *mHelpBarWidget;       // +0x2E0
    int mScrollPosition;                 // +0x2E4
    int mScrollTargetPosition;           // +0x2E8
    float mScrollAnimationTime;          // +0x2EC
    int mPageChallengeIndex[NUM_CHALLENGE_MODES];  // +0x2F0 .. +0x467
    float mPageChallengeAnimTime[NUM_CHALLENGE_MODES]; // +0x468 .. +0x5DF
    int mPageChallengeCount;             // +0x5E0
    GameMode mSelectedGameMode;          // +0x5E4, always mSelectedMode - 2
    int mSelectedChallengeIndex;         // +0x5E8
    int mSurvivalCount;                  // +0x5EC
    // The two porting-only pointers the reference tree adds after the 380-int
    // base layout. Nothing in ChallengeScreen.cpp reaches +0x5F0 or +0x5F4 yet.
    NewLawnButton *mBackButton;          // +0x5F0
    WaitForSecondPlayerDialog *mConnectDialog; // +0x5F4
    // size 0x5F8

    bool ShowPageButtons();
    int UpdateButtons();
    bool IsScaryPotterLevel(GameMode theGameMode);
    bool IsIZombieLevel(GameMode theGameMode);
    int MoreTrophiesNeeded(int theChallengeIndex);
    int AccomplishmentsNeeded(int theChallengeIndex);
    void DrawButton(Sexy::Graphics *theGraphics, int theIndex, int thePageIndex);
    void UpdateToolTip();
    void GameAxisMove();
    void GameButtonUp();
    ChallengeScreen* SetScrollTarget(int theTarget);
    int * SetUnlockChallengeIndex(ChallengePage thePage, bool theAnimate);
    ChallengeScreen(LawnApp *theApp, ChallengePage thePage, unsigned int theFlags);
    // _ZN15ChallengeScreen14GameButtonDownEN4Sexy13GamepadButtonEij @0x18BF98:
    // three arguments, and the body reads only the button. Non-virtual, so the
    // signature is layout-neutral.
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    int DoUnlockSurvivalDialog(int theChallengeIndex);
    int SaveSurvivalCount();
    int LoadSurvivalCount();
    void MouseDown(int x, int y, int theClickCount) override;
    void MouseDrag(int x, int y) override;
    void MouseUp(int x, int y, int theClickCount) override;
    void MouseWheel(int theDelta) override;
    int mMouseDownY = 0;
    int mMouseScrollStart = 0;
    bool mMousePressed = false;
    bool mMouseScrolled = false;
};

// TODO(recon): the binary also defines ChallengeDefinition (six members:
// mChallengeMode, mChallengeIconIndex, mPage, mRow, mCol, mChallengeName), the
// global gChallengeDefs[200], GetChallengeDefinition(int) and the two globals
// gChallengeScreenRequestState / gChallengeScreenOpenReplayManage. None of them
// has a declaration anywhere in this tree yet; they belong to a later batch
// because ChallengeScreen.cpp only calls GetChallengeDefinition().

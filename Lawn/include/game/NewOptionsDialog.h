// NewOptionsDialog -- reconstructed from libGameMain.so.
//
// `class NewOptionsDialog : public LawnDialog, public Sexy::SliderListener,
//                            public Sexy::CheckboxListener`.
//   The authoritative PvZ\Lawn\Widget\NewOptionsDialog.h says so explicitly, and
//   the vtable proves it: slots 31/36/50/71 hold `LawnDialog::Update` (0x141188),
//   `LawnDialog::Draw` (0x13ef00), `LawnDialog::Resize` (0x13f3b8) and
//   `LawnDialog::KeyDown` (0x13eb3c), and slot 45 holds `Sexy::Dialog::SetColor`
//   (0x301f84) -- the dialog chain is LawnDialog -> Dialog -> WidgetContainer.
//   The .cpp confirms the base offset: it reads +0x2E0 as a `LawnApp *`, which is
//   exactly `LawnDialog::mApp`, and LawnDialog is 0x2FC bytes (LawnApp.cpp:1260
//   allocates 0x2FC bytes for it).
//
// The two listener bases are what the constructor's second and third vptr stores
// prove: it writes 0x67F0E8 into +0x2FC and 0x67F0F4 into +0x300, and those two
// tables sit at the end of _ZTV16NewOptionsDialog (0x67EE90):
//   0x67EE90  [0] 0            [1] _ZTI16NewOptionsDialog   <- the main table
//   0x67F0B4  [0] -256         [1] _ZTI16NewOptionsDialog   <- the ButtonListener at +0x100
//   0x67F0E0  [0] -764         [1] _ZTI16NewOptionsDialog   <- the SliderListener at +0x2FC
//   0x67F0F0  [0] -768         [1] _ZTI16NewOptionsDialog   <- the CheckboxListener at +0x300
// and their single slots are `_ZThn764_...SliderValEid` (0x67F0E8) and
// `_ZThn768_...CheckboxCheckedEib` (0x67F0F4). SettingsDialog has the same two
// bases at the same two offsets, so the layout is corroborated.
//
// SIZE 0x344, NOT 0x8A8.
//   LawnApp.cpp:1415 allocates this dialog as 0x344 bytes, which is LawnDialog's
//   0x2FC plus 18 words, and the .cpp's largest real member access is +0x340. The
//   0x8A8 the transcriber recorded has no member behind it -- the only larger offset
//   in the file, +2064 at NewOptionsDialog.cpp:982, is a vtable-slot read
//   transcribed as a member access (map-lawn.md section "Widget\NewOptionsDialog.h").
//   The 18 own words are named below from the MakeButton call sites, which pin the
//   button ids, and from the constructor's parameter stores.
//
// VTABLE -- 135 slots; the ten own slots are:
//   [  0] 0x1413f0  ~NewOptionsDialog (deleting)
//   [ 29] 0x141e98  AddedToManager        [ 50] 0x141f98  Resize
//   [ 30] 0x141f20  RemovedFromManager    [ 61] 0x14260c  GotFocus
//   [ 31] 0x142248  Update                [ 71] 0x14256c  KeyDown
//   [ 36] 0x14224c  Draw                  [ 41] 0x142610  OrderInManagerChanged
// `ButtonDepress` is a `Sexy::ButtonListener` slot that arrives through Dialog's
// second base, not a new slot of this class.
// The authoritative header only lists three hook targets (AddedToManager,
// RemovedFromManager, ButtonDepress); the other seven come from this file's vtable
// comment block and have implementations in the .cpp.
// The Itanium table has a D0/D1 destructor pair; MSVC x86 keeps one slot.
//
// NOTE: the transcribed .cpp spells several of these with `int`/`_DWORD *`/`LawnApp *`
// returns; the base classes declare `void`, which is what an override needs, so the
// declarations below use void. Phase 2 aligns the bodies.
//
// BUTTON IDS. The authoritative header's protected enum
// (House=0, Useless=1, HelpDialog=2, Almanac=3, MainMenu=4, Restart=5, Update=6,
// ClearSecondPlayer=7) does NOT match this build: ButtonDepress switches on
// 0/2/3/4/5/6/7 (no case 1) while MakeButton is called with 0/1/2/3/4/5/7/8/1000.
// The enum below keeps the authoritative names and adds the two extra ids this
// build uses.
// Id 8 is the full-version prompt; id 1000 resumes the game.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "CheckboxListener.h"
#include "Dialog.h"
#include "GamepadApp.h"
#include "LawnDialog.h"
#include "SexyAppBase.h"
#include "SliderListener.h"
#include "TodFoley.h"
#include "VSResultsMenu.h"
#include "Widget.h"

// Board, CutScene and ZenGarden are not named anywhere in this file, so all three
// includes were pure cycle -- Board.h -> LawnApp.h -> NewOptionsDialog.h ->
// Board.h among them. LawnApp is reached only through `LawnApp *mApp` and the
// constructor's pointer parameter, so it is declared too.
// Sexy\ReconTypes.h already carries the LawnApp declaration.
class Board;
class CutScene;
class ZenGarden;
class LawnApp;

class GameButton;

class NewOptionsDialog : public LawnDialog, public Sexy::SliderListener, public Sexy::CheckboxListener
{
public:
    enum
    {
        NewOptionsDialog_House = 0,
        NewOptionsDialog_Useless = 1,
        NewOptionsDialog_HelpDialog = 2,
        NewOptionsDialog_Almanac = 3,
        NewOptionsDialog_MainMenu = 4,
        NewOptionsDialog_Restart = 5,
        NewOptionsDialog_Update = 6,
        NewOptionsDialog_ClearSecondPlayer = 7,
        NewOptionsDialog_UnlockFullVersion = 8,
        NewOptionsDialog_Resume = 1000,
    };

    virtual ~NewOptionsDialog();

    // slot 29
    virtual void AddedToManager(Sexy::WidgetManager * theWidgetManager);
    // slot 30
    virtual void RemovedFromManager(Sexy::WidgetManager * theWidgetManager);
    // slot 31
    virtual void Update();
    // slot 36
    virtual void Draw(Sexy::Graphics * theGraphics);
    // slot 41
    virtual void OrderInManagerChanged();
    // slot 50
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    // slot 61
    virtual void GotFocus();
    // slot 71
    virtual int KeyDown(Sexy::KeyCode theKey);
    // `Sexy::ButtonListener` slot, reached through Dialog's second base.
    virtual void ButtonDepress(int theId);
    // Also a `Sexy::ButtonListener` slot, through the same second base.
    virtual void ButtonPress(int theId);

    // ---- members, absolute offsets ----
    // LawnDialog occupies +0x00..+0x2FB, the SliderListener base's vptr is
    // +0x2FC and the CheckboxListener base's vptr is +0x300, so this class's own
    // members start at +0x304, where the opening gamepad index is stored.
    // The button ids and labels follow the constructor at 0x14140C.
    int mGamepadIndex;              // +0x304  the constructor stores the gamepad index here
    GameButton * mHelpDialogButton; // +0x308  MakeButton(2) "[HELP AND OPTIONS]"
    GameButton * mLeaderboardsButton; // +0x30C  MakeButton(0) "[LEADERBOARDS]"
    GameButton * mAchievementsButton; // +0x310  MakeButton(1) "[ACHIEVEMENTS]"
    GameButton * mAlmanacButton;    // +0x314  MakeButton(3), either label
    GameButton * mUnlockButton;     // +0x318  MakeButton(8)
    GameButton * mRestartButton;    // +0x31C  MakeButton(5), restart or concede
    GameButton * mDropP2Button;      // +0x320  MakeButton(7) "[DROP_P2]"
    GameButton * mMainMenuButton;   // +0x324  MakeButton(4)
    GameButton * mBackToGameButton;
    // True when the pause dialog was opened from a menu rather than a level.
    // Hides restart, relabels resume as OK, and changes the almanac rules.
    bool mFromMenu;
    Sexy::Font *mFontId;

    // ---- methods that are not vtable slots (kept from the transcription) ----
    virtual int GetPreferredHeight(int theWidth);   // overrides Dialog's [125]
    // Overrides of the two listener bases. Both bodies are empty in the binary.
    virtual void SliderVal(int theId, double theValue);
    virtual void CheckboxChecked(int theId, bool checked);
    NewOptionsDialog(LawnApp *theApp, bool theFromMenu, unsigned int theGamepadIndex);
    virtual void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
};

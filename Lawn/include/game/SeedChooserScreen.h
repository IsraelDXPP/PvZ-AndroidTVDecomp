// SeedChooserScreen -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class SeedChooserScreen : public Sexy::Widget, public Sexy::ButtonListener`
// (RTTI "17SeedChooserScreen"; primary base "N4Sexy6WidgetE" at offset 0,
// secondary base "N4Sexy14ButtonListenerE" at offset 0x100).
//
// 0x100 is confirmed twice: the class's vtable is followed immediately by a
// secondary vtable whose offset-to-top is -256 (-0x100) and whose RTTI pointer is
// this class's, and the destructor's non-virtual thunk is `_ZThn256_...`. This
// class's own members therefore start at +0x104, exactly where the authoritative
// header's mShowHelpText ("// 65") sits.
//
// Object size 0xF18 = 3864: LawnApp::ShowSeedChooserScreen @0x2B38F8 and
// ShowZombieChooserScreen @0x2B3990 both allocate 0xF18 bytes, and the
// authoritative header's own numbering ends there (mButtonSlotState is ordinal
// 965, 965 * 4 + 4 = 3864). The transcribed header's claim that 0xF18 was a
// placeholder and 0xF28 the real size had the causality backwards.
//
// vtable order -- this class's own slots (129 in all; the rest come from Widget):
//   [  0] 0x14bcb0  ~SeedChooserScreen    (D1)
//   [  1] 0x14bf20  ~SeedChooserScreen    (D0)
//   [ 29] 0x151670  AddedToManager        (override)
//   [ 30] 0x14b9dc  RemovedFromManager    (override)
//   [ 31] 0x151460  Update                (override)
//   [ 36] 0x14fb4c  Draw                  (override)
//   [ 54] 0x14bb8c  DrawOverlay           (override)
//   [ 61] 0x151658  GotFocus              (override)
//   [ 67] 0x14e738  KeyChar               (override)
//   [ 71] 0x14ba6c  KeyDown(Sexy::KeyCode) (override)
//   [ 72] 0x14b9ec  KeyUp(Sexy::KeyCode)  (override)
//   [ 76] 0x14b9e0  MouseDown             (override)
//   [ 79] 0x14b99c  MouseUp               (override)
//   [122] 0x14b990  ButtonPress           (overrides ButtonListener)
//   [123] 0x14e780  ButtonDepress         (overrides ButtonListener)
//   [124] 0x14ee3c  OnKeyDown             (new)
//   [125] 0x14b9a0  OnKeyUp               (new)
//   [126] 0x14bb38  GameAxisMove          (new)
//   [127] 0x14f618  GameButtonDown        (new)
//   [128] 0x14bb64  GameButtonUp          (new)
// Widget has 122 slots (0..121), so 122..128 are this class's own; MSVC x86
// gives a virtual destructor ONE slot where Itanium gives two (D1+D0).
//
// MouseMove (slot 75) and MouseDrag (slot 81) still hold Sexy::Widget's own
// addresses -- `_ZN17SeedChooserScreen9MouseMoveEii` does not exist -- so they are
// Mouse support now overrides both inherited methods.
//
// TODO(recon): in the ARM binary ButtonPress and ButtonDepress also occupy slots
// 122/123 of the PRIMARY vtable, because GCC appends overrides of secondary-base
// virtuals there; MSVC puts them in the secondary vtable at +0x100 instead, which
// shifts OnKeyDown..GameButtonUp down by two. The declaration order below keeps
// the binary's sequence so the intent stays readable.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_SEEDCHOOSERSCREEN_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_SEEDCHOOSERSCREEN_H

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "ButtonListener.h"
#include "Challenge.h"
#include "GameButton.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "Level.h"
#include "MTRand.h"
#include "SeedPacket.h"
#include "SexyAppBase.h"
#include "ToolTipWidget.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include <vector>

// Board is stored only as `Board *mBoard` and CutScene is not named in this file
// at all, so both includes were pure cycle. The Board.h edge is the important
// one: Board.h includes LawnApp.h and StoreScreen.h, which is how
// SeedChooserScreen.h ended up on chains such as
// StoreScreen.h -> SeedChooserScreen.h -> Board.h -> ... -> StoreScreen.h.
//
// ImitaterDialog is likewise only a pointer (`ImitaterDialog *mImitaterDialog` at
// line 244) and its header includes this one back, so that pair is cut here too:
// ImitaterDialog.h no longer includes this header, and this header declares the
// class instead of including it.
class Board;
class CutScene;
class ImitaterDialog;

// LawnApp is reached only through `LawnApp *mApp` at +0xE8C, so it is declared
// rather than included: the include was half of the LawnApp.h <->
// SeedChooserScreen.h cycle, which fed several longer chains through Board.h and
// StoreScreen.h. VSSetupMenu.h and ImitaterDialog.h both include this header back
// and are cut on their own side. Sexy\ReconTypes.h already declares LawnApp.
class LawnApp;
class VSSetupMenu;
// +0xEF4 is only ever a pointer, and the authoritative header
// (PvZ\Lawn\Widget\SeedChooserScreen.h:38) forward-declares it the same way.
// Without this declaration the member below failed to parse (C2143/C4430/C2238),
// which took down every translation unit that reaches this header.
class ImitaterDialog;

// Touch bookkeeping for the chooser. Declared in the authoritative header as a
// free enum next to the class.
enum SeedChooserTouchState
{
    VIEW_LAWN_BUTTON,
    SEEDCHOOSER_TOUCHSTATE_SEED_CHOOSER,
    SEEDCHOOSER_TOUCHSTATE_STORE_BUTTON,
    SEEDCHOOSER_TOUCHSTATE_START_BUTTON,
    SEEDCHOOSER_TOUCHSTATE_ALMANAC_BUTTON,
    SEEDCHOOSER_TOUCHSTATE_NONE,
};

inline SeedChooserTouchState gSeedChooserTouchState = SEEDCHOOSER_TOUCHSTATE_NONE;
inline bool daveNoPickSeeds;

// Native seed-selection state, plus transient layout animation data.
class ChosenSeed
{
public:
    int mX; // +0x00
    int mY; // +0x04
    int mTimeStartMotion; // +0x08
    int mTimeEndMotion; // +0x0C
    int mStartX; // +0x10
    int mStartY; // +0x14
    int mEndX; // +0x18
    int mEndY; // +0x1C
    int mChosenPlayerIndex; // +0x20
    SeedType mSeedType; // +0x24
    ChosenSeedState mSeedState; // +0x28
    int mSeedIndexInBank; // +0x2C
    bool mRefreshing; // +0x30
    // +0x31 ~ +0x33 padding
    int mRefreshCounter; // +0x34
    SeedType mImitaterType; // +0x38
    bool mCrazyDavePicked; // +0x3C
    // +0x3D ~ +0x3F padding
    float mStartScale;
};
using ChosenSeedData = ChosenSeed;

// A packet the extended chooser has banned. Sixteen bytes, from the
// authoritative header; this tree had no such type.
class BannedSeed
{
public:
    int mX; // +0x00
    int mY; // +0x04
    SeedType mSeedType; // +0x08
    BannedSeedState mSeedState; // +0x0C
    // Size 0x10.
};

class SeedChooserScreen : public Sexy::Widget, public Sexy::ButtonListener
{
public:
    // From the authoritative header; used as the button ids.
    enum
    {
        SeedChooserScreen_Start = 100,
        SeedChooserScreen_Random = 101,
        SeedChooserScreen_ViewLawn = 102,
        SeedChooserScreen_Almanac = 103,
        SeedChooserScreen_Menu = 104,
        SeedChooserScreen_Store = 105,
        SeedChooserScreen_Imitater = 106,
        SeedChooserScreen_Page,
    };

    // Direction of GetNextSeedInDir. The authoritative header nests this enum in
    // the class; this tree had a `SeedChooserScreen_SeedDir` shim instead.
    enum SeedDir
    {
        SEED_DIR_UP,
        SEED_DIR_DOWN,
        SEED_DIR_LEFT,
        SEED_DIR_RIGHT,
    };

    virtual ~SeedChooserScreen();

    // Slots 29..79: overrides of Widget/WidgetContainer.
    //
    // All of them return void in the base, so the `int`/`char *` spellings the
    // transcriber copied out of the decompiler were C2555, not overloads: an
    // override with a different return type is rejected and becomes a brand new
    // virtual instead, which shifts every following slot. Sources for `void`:
    //   * PvZ\SexyAppFramework\Widget\WidgetContainer.h, WidgetContainerVTable:
    //     AddedToManager / RemovedFromManager / Update / Draw / DrawAll.
    //   * PvZ\SexyAppFramework\Widget\Widget.h, WidgetVTable: DrawOverlay,
    //     GotFocus, KeyChar, KeyDown, KeyUp, MouseDown -- all `void`.
    //   * PvZ\Lawn\Widget\SeedChooserScreen.h declares the same six methods
    //     `void` (lines 214, 215, 230, 255, 267, 268) and its own
    //     `old_SeedChooserScreen_MouseDown/MouseUp` hooks are `void` too.
    // KeyDown/KeyUp keep `void` even though the hook header spells them `bool`:
    // the base `Sexy::Widget::KeyDown(KeyCode)` is void in the authoritative
    // vtable, so a bool override would not compile.
    virtual void AddedToManager(Sexy::WidgetManager *theManager) override;
    virtual void RemovedFromManager(Sexy::WidgetManager *theManager) override;
    virtual void Update() override;
    virtual void Draw(Sexy::Graphics *g) override;
    virtual void DrawOverlay(Sexy::Graphics *g) override;
    virtual void GotFocus() override;
    virtual int KeyChar(char theChar) override;
    virtual int KeyDown(Sexy::KeyCode theKey) override;
    virtual int KeyUp(Sexy::KeyCode theKey) override;
    virtual void MouseDown(int x, int y, int theClickCount) override;
    virtual void MouseUp(int x, int y, int theClickCount) override;
    void MouseMove(int x, int y) override;
    void MouseDrag(int x, int y) override;
    Sexy::Rect GetInsetRect() override;
    // ButtonListener overrides, then this class's own new virtuals.
    virtual void ButtonPress(int theId) override;
    virtual void ButtonDepress(int theId) override;
    virtual int OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags);
    virtual int OnKeyUp(Sexy::KeyCode theKey, unsigned int theFlags);
    virtual int GameAxisMove(int theAxis, int theButton, int thePlayerIndex);
    // TV passes the button first, then the originating gamepad index and flags.
    virtual int GameButtonDown(Sexy::GamepadButton theButton, int theGamepadIndex, unsigned int theFlags);
    virtual int GameButtonUp(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);

    // Widget occupies +0x00..+0xFF; ButtonListener's vptr +0x100..+0x103.
    bool mShowHelpText; // +0x104
    // +0x105 ~ +0x107 padding
    GameButton *mImitaterButton; // +0x108
    ChosenSeedData mChosenSeeds[54]; // Native seed slots and extension slots.
    LawnApp *mApp; // +0xE8C
    Board *mBoard; // +0xE90
    int mSeedChooserAge; // +0xE94
    int mSeedsInFlight; // +0xE98
    int mSeedsInBank; // +0xE9C
    int mSeedsIn1PBank; // +0xEA0
    int mSeedsIn2PBank; // +0xEA4
    ToolTipWidget *mToolTip1; // +0xEA8
    ToolTipWidget *mToolTip2; // +0xEAC
    int mToolTipSeed1; // +0xEB0
    int mToolTipSeed2; // +0xEB4
    int mCursorPositionX1; // +0xEB8
    int mCursorPositionX2; // +0xEBC
    int mCursorPositionY1; // +0xEC0
    int mCursorPositionY2; // +0xEC4
    SeedChooserState mChooseState; // +0xEC8
    int mViewLawnTime; // +0xECC
    bool mOpeningDialog; // +0xED0
    // +0xED1 ~ +0xED3 padding
    int mPlayerIndex; // +0xED4
    int mSeedIndex1; // +0xED8
    int mSeedIndex2; // +0xEDC
    float mCursorBobPhase; // +0xEE0
    bool mIsZombieChooser; // +0xEE4
    // +0xEE5 ~ +0xEE7 padding
    SeedBank *mSeedBank1; // +0xEE8
    SeedBank *mSeedBank2; // +0xEEC
    int mDimCounter; // +0xEF0
    ImitaterDialog *mImitaterDialog; // +0xEF4
    GameButton *mViewLawnButton; // +0xEF8
    GameButton *mStoreButton; // +0xEFC
    GameButton *mStartButton; // +0xF00
    GameButton *mAlmanacButton; // +0xF04
    // The bottom-row buttons: view lawn, store, start, almanac.
    std::vector<GameButton *> mButtons;
    GameButton *mSelectedButton;
    SeedType mMousePressedSeed = SEED_NONE;
    int mMousePressedButton = -1;
    int mMouseChoosingPlayer = 0;
    // Size 0xF18 (3864).

    SeedChooserScreen(bool theIsZombieChooser);

    int PickFromWeightedArrayUsingSpecialRandSeed(TodWeightedArray const *theArray, int theCount, MTRand &theRand);
    int PickSeedsFromLevelConfig();
    int EnableStartButton(bool theEnabled);
    int NumColumns();
    int GetSeedPositionInBank(int theIndex, int &theX, int &theY, int thePlayerIndex);
    int SeedNotAllowedToPick(SeedType theSeedType);
    int SeedNotAllowedDuringTrial(SeedType theSeedType);
    int GetZombieType(int theIndex);
    int GetZombieSeedType(int theIndex);
    int GetSeedPacketIndex(SeedType theSeedType);
    int VSAutoPickResourceGen();
    void DrawPacket(Sexy::Graphics *g, int theX, int theY, int theSeedType, int theImitaterType, float thePercentDark,
        int theGrayness, Color &theColor, bool theDrawCost, bool theUseCurrentCost, float theScale = 1.0f);
    float GetChosenSeedScale(const ChosenSeedData &theChosenSeed) const;
    void UpdateViewLawn();
    bool DisplayRepickWarningDialog(char const *theMessage);
    int FlyersAreComing();
    int FlyProtectionCurrentlyPlanted();
    int RemoveToolTip(int thePlayerIndex);
    int CancelLawnView();
    int RebuildHelpbar();
    int PickedPlantType(SeedType theSeedType);
    unsigned int SeedNotRecommendedToPick(SeedType theSeedType);
    int CheckSeedUpgrade(int theUpgradeTo, int theUpgradeFrom);
    int HasPacket(int theSeedType, bool theIsZombie);
    int CrazyDavePickSeeds();
    int Has7Rows();
    int GetSeedPositionInChooser(int theIndex, int &theX, int &theY);
    int GetNextSeedInDir(int theIndex, SeedDir theDirection);
    int SeedHitTest(int theX, int theY);
    SeedType MouseSeedHitTest(int theX, int theY);
    void UpdateMouseSeedCursor(SeedType theSeed);
    Sexy::Rect GetMouseButtonRect(const GameButton &theButton) const;
    GameButton *MouseButtonHitTest(int theX, int theY) const;
    int UpdateCursor();
    int ShowToolTip(int thePlayerIndex);
    int FindSeedInBank(int theIndexInBank, int thePlayerIndex);
    int CloseSeedChooser();
    int OnStartButton();
    int ClickedSeedInBank(ChosenSeedData &theChosenSeed, int theGamepadIndex);
    void UpdateImitaterButton();
    void LandFlyingSeed(ChosenSeedData &theChosenSeed);
    void UpdateAfterPurchase();
    int PickRandomSeeds();
    bool CanPickNow();
    int ClickedSeedInChooser(ChosenSeedData &theChosenSeed, int thePlayerIndex);
    int ShouldDisplayCursor(int thePlayerIndex);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_SEEDCHOOSERSCREEN_H

// LawnDialog -- reconstructed from libGameMain.so.
//
// Inheritance: `class LawnDialog : public Sexy::Dialog`. The authoritative header
// PvZ\Lawn\Widget\LawnDialog.h says so, and the binary agrees: _ZTV10LawnDialog
// @0x67D088 repeats Dialog's 131 primary slots verbatim up to index 129 and then
// appends LawnDialog's own, and the ctor at LawnDialog.cpp:548 does
// `new ((Sexy::Dialog *)(uintptr_t)this) Sexy::Dialog(...)` in place.
//
// SIZE 0x2FC (the v115 layout), NOT the transcriber's 0x710.
//   * LawnApp.cpp:1260 allocates `operator new(0x2FCu)`.
//   * The authoritative header ends at "115: 191" = 191 ints = 0x2FC.
//   * The derived classes' own first members land exactly there:
//     HelpOptionsDialog::mApp, HouseChooserDialog::mSelectedHouseType,
//     NewUserDialog's EditListener sub-object at +0x2FC, MailScreen::mInited at
//     +0x2FC, AlmanacDialog's ScrollListener sub-object at +0x2FC.
//   The 0x710 came from three out-of-range offsets the bodies reach (+0x700,
//   +0x480, +0x390) that belong to other `this` types; see build/map-lawn.md.
//
// ---------------------------------------------------------------------------
// VTABLE @0x67D088, 133 primary slots. LawnDialog's own entries:
//   [  0] 0x13e3f4  ~LawnDialog             override
//   [  1]           (ARM D0; MSVC x86 keeps ONE dtor slot)
//   [ 29] 0x1410fc  AddedToManager          override
//   [ 30] 0x141254  RemovedFromManager      override
//   [ 31] 0x141188  Update                  override
//   [ 36] 0x13ef00  Draw                    override
//   [ 50] 0x13f3b8  Resize                  override
//   [ 61] 0x13e9a8  GotFocus                override
//   [ 71] 0x13eb3c  KeyDown(KeyCode)        override
//   [128] 0x13e36c  ButtonPress             override
//   [129] 0x13e980  ButtonDepress           override
//   --- slots 130 and below are new in LawnDialog ---
//   [131] 0x13e340  SetButtonDelay
//   [132] 0x13e398  CheckboxChecked
// Slots 122..127 and 130 (SetButtonFont/SetHeaderFont/SetLinesFont/
// GetPreferredHeight/IsModal/WaitForResult/ButtonDownTick) still hold Dialog's
// implementations, so they are NOT redeclared.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "Font.h"
#include "Graphics.h"
#include "ReanimationWidget.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// LawnApp is reached only through pointers here (`LawnApp *mApp` and the
// constructor's first parameter), so it is declared rather than included.
//
// This edge was the most damaging one in the tree. LawnDialog.h was included at
// the top of LawnApp.h's list, so the pair formed a cycle: entering from this
// side ran `#include "LawnApp.h"`, and LawnApp.h's own include of AlmanacDialog.h
// came back round to LawnDialog.h while this file was still mid-parse, so
// `#pragma once` turned it into a no-op and LawnDialog was never defined. Every
// class deriving from LawnDialog in that translation unit then reported
// C2504 "base class undefined" -- MailScreen, HouseChooserDialog,
// BuySunMoneyDialog, RevivalGameDialog, ImitaterDialog, WaitForSecondPlayerDialog,
// UnlockDialog, GameOverDialog, AlmanacDialog, BuyMoneyDialog and the rest.
// Sexy\ReconTypes.h already carries this declaration for the whole tree.
class LawnApp;

class GameButton;

class LawnDialog : public Sexy::Dialog
{
public:
    // ---- members, absolute offsets (authoritative ordinals / byte comments) ----
    LawnApp * mApp;                    // +0x2E0 (184)
    int mButtonDelay;                  // +0x2E4 (185)
    // The constructor does `new ReanimationWidget()`, RemovedFromManager calls
    // `ReanimationWidget::Dispose` on it and Resize reads its mReanimation and
    // mPosY, so the member is the derived type, not Sexy::Widget. Same 4 bytes;
    // no offset moves and the 0x2FC assert still holds.
    ReanimationWidget * mReanimationWidget; // +0x2E8 (186)
    bool mDrawStandardBack;            // +0x2EC (748)
    GameButton * mLawnYesButton;       // +0x2F0 (188)
    GameButton * mLawnNoButton;        // +0x2F4 (189)
    bool mTallBottom;                  // +0x2F8 (760)
    bool mVerticalCenterText;          // +0x2F9 (761)
    bool mDrawFade;                    // +0x2FA, DrawDialogFade runs when this is set
    bool mDrawQRCode;                  // +0x2FB, DrawQRCode runs after the fade
    // size 0x2FC

    // ---- the Dialog slots this class overrides, in binary order ----
    virtual ~LawnDialog();                                                     // [  0]
    virtual void AddedToManager(Sexy::WidgetManager * theWidgetManager);       // [ 29]
    virtual void RemovedFromManager(Sexy::WidgetManager * theWidgetManager);   // [ 30]
    virtual void Update();                                                    // [ 31]
    virtual void Draw(Sexy::Graphics * g);                                    // [ 36]
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);     // [ 50]
    virtual void GotFocus();                                                  // [ 61]
    virtual int KeyDown(Sexy::KeyCode theKey);                               // [ 71]
    virtual void ButtonPress(int theId);                                      // [128]
    virtual void ButtonDepress(int theId);                                    // [129]

    // ---- slots 131..132: new in LawnDialog ----
    virtual void SetButtonDelay(int theDelay);                                // [131]
    virtual void CheckboxChecked(int theId, bool checked);                    // [132]

    // ---- methods the binary has but the authoritative header does not list ----
    int GetLeft();
    int GetWidth();
    int GetTop();
    int CalcSize(int theWidth, int theHeight, bool theSetPos);
    LawnDialog(LawnApp * theApp, Sexy::Image * theImage, int theId, bool isModal,
               const Sexy::String & theDialogHeader, const Sexy::String & theDialogLines,
               const Sexy::String & theDialogFooter, int theButtonMode);
};


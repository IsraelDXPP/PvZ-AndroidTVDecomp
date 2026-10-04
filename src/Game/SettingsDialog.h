// SettingsDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class:
//   `class SettingsDialog : public LawnDialog, public Sexy::SliderListener, public Sexy::CheckboxListener`
//
// RTTI "14SettingsDialog" lists three bases: "10LawnDialog" at offset 0,
// "N4Sexy14SliderListenerE" at 0x2FC and "N4Sexy16CheckboxListenerE" at 0x300.
// That is what dates the layout, and it corrects the authoritative header, which
// models the two listeners as DATA (`Sexy::SliderListener *mSliderListener;` and
// a by-value `Sexy::CheckboxListener mCheckboxListener;`). They are not members:
// each is a polymorphic base whose vptr sits at +0x2FC and +0x300, and this
// class's own members start at +0x304 -- exactly where the authoritative table's
// mApp ("// 193") is. MSVC x86 lays a non-virtual multiple inheritance out the
// same way.
//
// Object size 0x32C: LawnApp.cpp:5783 allocates 0x32C bytes, which is LawnDialog
// (0x2FC) + two 4-byte listener vptrs + seven pointers + five ints. The
// transcribed class body was an opaque 0x3A0-byte block, inflated by a
// mis-attributed `this + 912` access.
//
// vtable order -- this class's own slots (135 in all); everything else comes from
// LawnDialog/Dialog/Widget:
//   [  0] 0x1681b4  ~SettingsDialog      (D1)
//   [  1] 0x168234  ~SettingsDialog      (D0)
//   [ 29] 0x168bec  AddedToManager       (override)
//   [ 30] 0x168c78  RemovedFromManager   (override)
//   [ 31] 0x168e94  Update               (override)
//   [ 36] 0x168f8c  Draw                 (override)
//   [ 50] 0x168e1c  Resize               (override)
//   [ 61] 0x1690f8  GotFocus             (override)
//   [ 71] 0x16911c  KeyDown              (override)
//   [125] ..        GetPreferredHeight   (override)
//   [128] ..        ButtonPress          (override)
//   [129] ..        ButtonDepress        (override)
//   [132] ..        CheckboxChecked      (overrides CheckboxListener)
//   [133] ..        SliderVal            (overrides SliderListener)
//   [134] ..        GameButtonDown       (new)
// MSVC x86 gives a virtual destructor ONE slot where Itanium gives two (D1+D0).
//
// TODO(recon): as with the other multiple-inheritance widgets, MSVC places the
// SliderListener and CheckboxListener overrides in the secondary vtables at
// +0x2FC/+0x300 rather than in slots 132/133 of the primary table, so
// GameButtonDown lands at 132 instead of 134. Declared in the binary's order
// regardless so the intent stays readable.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "CheckboxListener.h"
#include "Dialog.h"
#include "GameButton.h"
#include "GameSlider.h"
#include "LawnDialog.h"
#include "SexyAppBase.h"
#include "SliderListener.h"
#include "TodFoley.h"

// LawnApp is reached only through `LawnApp *mApp` at +0x304 and the constructor's
// pointer parameter, so it is declared rather than included: the include was half
// of the LawnApp.h <-> SettingsDialog.h cycle. Sexy\ReconTypes.h already has it.
class LawnApp;

class SettingsDialog : public LawnDialog, public Sexy::SliderListener, public Sexy::CheckboxListener
{
public:
    // Button ids, from the authoritative header.
    enum {
        SettingsDialog_HardwareAcceleration = 1024,
        SettingsDialog_HapticFeedback,
    };

    virtual ~SettingsDialog();

    // Slots 29..129: overrides of LawnDialog/Dialog/Widget. Every one of them is
    // `void` in LawnDialog (LawnDialog.h 79..87, which is the authoritative vtable
    // order), so the `int` the transcription used is gone. GetPreferredHeight is
    // the one override here that really returns int (Dialog.h 134, slot 125).
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    virtual void GotFocus();
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual int GetPreferredHeight(int theWidth);
    virtual void ButtonPress(int theId);
    virtual void ButtonDepress(int theId);
    // Overrides of the two listener bases, then this class's own new virtual.
    virtual void CheckboxChecked(int theId, bool theChecked);
    virtual void SliderVal(int theId, double theValue);
    // The vtable signature is GameButtonDown(GamepadButton, player, flags).
    virtual void GameButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags);

    // LawnDialog occupies +0x00..+0x2FB, SliderListener's vptr +0x2FC..+0x2FF,
    // CheckboxListener's vptr +0x300..+0x303.
    LawnApp *mApp;                  // +0x304
    Sexy::Widget *mMusicSlider;     // +0x308
    Sexy::Widget *mSoundSlider;     // +0x30C
    GameButton *mBackButton;        // +0x310
    GameButton *mSelectDeviceButton; // +0x314
    // SaveDetails returning false blocks the gamepad back button. The byte
    // beside it was a deferred-close flag that nothing in this binary sets.
    bool mSaveFailed;
    // Size 0x32C.

    SettingsDialog(LawnApp *theApp);
};

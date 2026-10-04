// TitleScreen -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class TitleScreen : public Sexy::Widget, public Sexy::ButtonListener`
// (RTTI "11TitleScreen"; primary base "N4Sexy6WidgetE" at offset 0, secondary
// base "N4Sexy14ButtonListenerE" at offset 0x100).
//
// That 0x100 is confirmed by the class's own constructor: TitleScreen.cpp:223
// builds the start button with a `Sexy::Widget *` that is the object itself and a
// `Sexy::ButtonListener *` that is the object plus 256 bytes, which pins both
// sizeof(Sexy::Widget) == 0x100 and the ButtonListener sub-object's offset. This
// class's own members therefore start at +0x104, exactly where the authoritative
// header's mPopcapLogo ("// 65") sits.
//
// Object size 0x154 = 85 ints: the authoritative header ends with that comment,
// and LawnApp.cpp:7216 / :8446 both allocate 0x154 bytes. The transcribed class
// body was an opaque 0x3A0-byte block, inflated by a mis-attributed `this + 912`
// access (0x390) that belongs to a base class.
//
// vtable order -- this class's own slots (124 in all; the rest come from Widget):
//   [  0] 0x1a66e4  ~TitleScreen          (D1)
//   [  1] 0x1a69ac  ~TitleScreen          (D0)
//   [ 29] 0x1a6480  AddedToManager        (override)
//   [ 30] 0x1a64a8  RemovedFromManager    (override)
//   [ 31] 0x1a7518  Update                (override)
//   [ 36] 0x1a6cf8  Draw                  (override)
//   [ 54] 0x1a6410  DrawOverlay           (override)
//   [ 61] 0x1a64c0  GotFocus              (override)
//   [ 71] 0x1a6284  KeyDown               (override)
//   [122] 0x1a..    ButtonPress           (overrides ButtonListener)
//   [123] 0x1a..    GameButtonDown        (new)
// Widget has 122 slots (0..121). GameButtonDown is not a Widget and not a
// ButtonListener virtual, so it is genuinely new; ButtonPress overrides the
// ButtonListener sub-object. MSVC x86 gives a virtual destructor ONE slot where
// Itanium gives two (D1+D0).
//
// TODO(recon): in the ARM binary ButtonPress also occupies a slot of the PRIMARY
// vtable (122) with GameButtonDown right after it (123) -- GCC appends overrides
// of secondary-base virtuals there. MSVC does not: ButtonPress goes to the
// secondary vtable at +0x100 and GameButtonDown becomes slot 122. The
// declaration order below keeps ButtonPress first so the intended sequence is
// still readable.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "ButtonListener.h"
#include "EffectSystem.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "Reanimation.h"
#include "ResourceManager.h"
#include "SexyAppBase.h"
#include "SexyTransform2D.h"
#include "StartButton.h"
#include "Widget.h"
#include "WidgetContainer.h"

// LawnApp is reached only through `LawnApp *mApp` at +0x150 and the constructor's
// pointer parameter, so it is declared rather than included: the include was half
// of the LawnApp.h <-> TitleScreen.h cycle. Sexy\ReconTypes.h already has it.
class LawnApp;

class TitleScreen : public Sexy::Widget, public Sexy::ButtonListener
{
public:
    // The authoritative header declares this enum inside the class.
    enum TitleState {
        WaitingForFirstDraw = 0,
        PopcapLogo_OR_IntroVideo = 1,
        ESRBLogo = 2,   // unused in the shipped game
        GuideLogo = 3,
        Loading = 4,
    };

    virtual ~TitleScreen();

    // Slots 29..71: overrides of Widget/WidgetContainer. Every one of them is
    // `void` in the base chain -- WidgetContainer.h 149/150 (AddedToManager,
    // RemovedFromManager), Widget.h 66/80/95/105 (Update, DrawOverlay, GotFocus,
    // KeyDown) and ButtonListener.h 42 (ButtonPress). The `int` returns were the
    // transcriber's fallback and made each of these a C2555 instead of an
    // override.
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void DrawOverlay(Sexy::Graphics *theGraphics);
    virtual void GotFocus();
    virtual void KeyDown(const Sexy::Event &theEvent);
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual void ButtonPress(int theId);
    virtual void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theModifierFlag);

    // Widget occupies +0x00..+0xFF; ButtonListener's vptr +0x100..+0x103.
    Sexy::Image *mPopcapLogo;      // +0x104
    Sexy::Image *mGuide;           // +0x108
    StartButton *mStartButton;     // +0x10C
    float mCurBarWidth;            // +0x110
    float mTotalBarWidth;          // +0x114
    float mBarVel;                 // +0x118
    float mBarStartProgress;       // +0x11C
    bool mRegisterClicked;         // +0x120  TODO(recon): not read or written by this .so
    bool mLoadingThreadComplete;   // +0x121
    // +0x122 ~ +0x123 padding
    int mTitleAge;                 // +0x124
    bool mNeedRegister;            // +0x128
    bool mNeedToInit;              // +0x129
    bool mNeedShowRegisterBox;     // +0x12A  TODO(recon): not read or written by this .so
    // +0x12B padding
    float mPrevLoadingPercent;     // +0x12C
    TitleState mTitleState;        // +0x130
    int mTitleStateCounter;        // +0x134
    int mTitleStateDuration;       // +0x138
    int mIntroVideoAge;            // +0x13C
    bool mVideoCompleted;          // +0x140
    bool mLoaderScreenIsLoaded;    // +0x141
    bool mMusicInitFinished;       // +0x142
    bool mIsPlayingIntroVideo;     // +0x143
    unsigned int mOverlayAlpha;    // +0x144
    bool mNeedPlayLogo;            // +0x14C
    // +0x14D ~ +0x14F padding
    LawnApp *mApp;                 // +0x150
    // Size 0x154 (85 ints).

    TitleScreen(LawnApp *theApp);
    void SetRegistered();
    void DrawToPreload();
    void VideoCompleted();
    void OnLoading();
    void SwitchState(int theState, int theDuration);
    void BeginProfileLoad();
};

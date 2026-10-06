// CreditScreen -- reconstructed from libGameMain.so.
//
// Real inheritance: `class CreditScreen : public Sexy::Widget, public Sexy::ButtonListener`
// (authoritative header PvZ\Lawn\Widget\CreditScreen.h, and the vtable agrees --
// every slot up to 121 is Widget's own addresses). Widget occupies +0x00..+0xFF,
// the secondary ButtonListener base contributes its vptr at +0x100, and
// CreditScreen's own members start at +0x104. The authoritative header's comments
// are word indices from 65 on, i.e. byte 0x104 for the first member.
//
// Size 0x17C, from `operator new of 0x180 bytes` at LawnApp.cpp:927 (0x17C rounded to the
// allocation granularity). The old placeholder said 0x180 already.
//
// vtable order -- slots 0..121 are Widget's and are NOT redeclared here. This class
// contributes three new primary-vtable entries, declared below in this order:
//   [122] 0x1b9074  CreditScreen::ButtonPress
//   [123] 0x1b9080  CreditScreen::ButtonDepress
//   [124] 0x1b9178  CreditScreen::GameButtonDown
// It also overrides these Widget slots: AddedToManager [29], RemovedFromManager [30],
// Update [31], Draw [36], DrawOverlay [54], GotFocus [61], KeyChar [67], KeyDown [71],
// MouseUp [79].
// Under MSVC a method that is also a ButtonListener virtual lands in the secondary
// vtable instead of adding a primary slot, so the absolute indices differ there;
// the declaration order is what has to match.

#pragma once

#include "ReconTypes.h"
// The class has two bases, so both need their complete type.
#include "ButtonListener.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "EffectSystem.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "PerfTimer.h"
#include "PoolEffect.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "ReanimatorTransform.h"
#include "SexyAppBase.h"
#include "TodFoley.h"
#include "TodParticleSystem.h"
#include "Widget.h"
#include "WidgetContainer.h"

// LawnApp is reached only through `LawnApp *mApp` and the constructor's pointer
// parameter, so it is declared rather than included: the include was half of the
// LawnApp.h <-> CreditScreen.h cycle. Sexy\ReconTypes.h already carries it.
class LawnApp;

// One entry of the credits scroll table. `gCreditsTiming` (@0x697A2C) is a flat
// table of these and GetTiming walks it with a stride of four floats -- the
// element type is only visible through `v12 += 4` on a `float *` and through the
// `[4 * index]` indexing at the two ends of the walk. The first word is the frame
// time the entry begins at; the other three are the interpolation UpdateMovie
// uses while the entry is the current one, which is why its two switches read
// words [1] and [3] and its curve endpoints read word [2] as an int.
struct CreditsTiming
{
    float mStartTime; // +0x00
    int   mCurveA;    // +0x04
    int   mEndY;      // +0x08
    int   mCurveB;    // +0x0C
};

class CreditScreen : public Sexy::Widget, public Sexy::ButtonListener
{
public:
    // The credits run in phases. The authoritative header declares this as a
    // nested `enum CreditsPhase`; this tree already models it at global scope in
    // Sexy\ReconTypes.h, and the transcribed bodies spell it that way, so the
    // global type is used throughout. Its members are CREDITS_MAIN1 = 0,
    // CREDITS_MAIN2 = 1, CREDITS_MAIN3 = 2, CREDITS_END = 3.

    virtual ~CreditScreen();

    // The three entries this class appends to the primary vtable.
    virtual void ButtonPress(int theId);    // [122] 0x1b9074
    virtual void ButtonDepress(int theId);  // [123] 0x1b9080
    virtual void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex,
                                unsigned int theModifierFlag); // [124] 0x1b9178
    // Overrides of Widget slots.
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);   // [ 29] 0x1b90d4
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // [ 30] 0x1b90ec
    virtual void Update();                                               // [ 31] 0x1bc0e4
    virtual void Draw(Sexy::Graphics *g);                                // [ 36] 0x1bc390
    virtual void DrawOverlay(Sexy::Graphics *g);                         // [ 54] 0x1b9118
    virtual void GotFocus();                                             // [ 61] 0x1b9114
    virtual int KeyChar(char theChar);                                  // [ 67] 0x1bbf84
    virtual int KeyDown(Sexy::KeyCode theKey);                          // [ 71] 0x1bc2e0
    virtual void MouseUp(int x, int y, int theClickCount);               // [ 79] 0x1b9070

    // Not vtable slots; the signatures are the ones the transcribed bodies define.
    CreditScreen(LawnApp *theApp, bool theFromMainMenu);
    int PreLoadCredits();
    int GetTiming(CreditsTiming **theStart, CreditsTiming **theEnd, float *theFraction);
    Reanimation *PlayReanim(int thePhase);
    int DrawFogEffect(Sexy::Graphics *theGraphics, float theOffset);
    void DrawFinalCredits(Sexy::Graphics *theGraphics);
    int UpdateBlink();
    void ExportMovieFrame(int theFrame);
    void TurnOffTongues(Reanimation *theReanim, int theTrack);
    int UpdateMovie();
    int JumpToFrame(CreditsPhase thePhase, float theTime);
    void PauseCredits();

    // -----------------------------------------------------------------------
    // Members +0x104..+0x17B. Names and order from the authoritative header
    // PvZ\Lawn\Widget\CreditScreen.h; offsets converted from its word indices and
    // cross-checked against CreditScreen.cpp's raw accesses (+260/+288/+336/+337/
    // +352/+376 are all reached there).
    // -----------------------------------------------------------------------
    LawnApp *mApp;                    // +0x104
    CreditsPhase mCreditsPhase;       // +0x108
    int mCreditsPhaseCounter;         // +0x10C
    ReanimationID mCreditsReanimID;   // +0x110
    ParticleSystemID mFogParticleID;  // +0x114
    int mBlinkCountdown;              // +0x118
    Sexy::Widget *mOverlayWidget;     // +0x11C
    bool mDrawBrain;                  // +0x120
    float mBrainPosX;                 // +0x124
    float mBrainPosY;                 // +0x128
    int mUpdateCount;                 // +0x12C
    int mDrawCount;                   // +0x130
    Sexy::PerfTimer mTimerSinceStart; // +0x138
    bool mDontSync;                   // +0x150
    bool mCreditsPaused;              // +0x151
    double mSavedMusicVolume;          // +0x158
    bool mPreloaded;                  // +0x160
    int mLastDrawCount;               // +0x164
    double mScrollPositionY1;         // +0x168
    double mScrollPositionY2;         // +0x170
    bool mIsFromMainMenu;             // +0x178
    // size 0x17C, allocation 0x180
};

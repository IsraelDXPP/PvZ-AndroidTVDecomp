// DaveHelp -- reconstructed from libGameMain.so.
//
// Inheritance chain: `class DaveHelp : public Sexy::Widget` (phase-1 group table;
// confirmed by the binary's ZTV8DaveHelp @0x6864f0, whose destructor chains
// straight into `Sexy::Widget::~Widget()` and whose slot 30/31/36 hold DaveHelp
// implementations).
//
// vtable -- this class overrides exactly five slots:
//   [  0] 0x1ec21c  DaveHelp::~DaveHelp
//   [  1] 0x1ec23c  DaveHelp::~DaveHelp
//   [ 30] 0x1eca08  DaveHelp::RemovedFromManager
//   [ 31] 0x1ec370  DaveHelp::Update
//   [ 36] 0x1eca1c  DaveHelp::Draw
//   [  2.. 29] and [ 32..121] still point at Sexy::Widget / WidgetContainer.
//
// MSVC x86 has ONE virtual-destructor slot where Itanium/ARM has D1+D0; only the
// declaration order matters.
//
// Size 0x11C, from the constructor call site (`an allocation of 0x11cu` followed by
// DaveHelp::DaveHelp(LawnApp*) @0x1ec488). Sexy::Widget is 0x100 bytes, so the
// class's own members are the seven words at +0x100..+0x11B.
//
// The seven words are named from the bodies:
//   +0x100  LawnApp *mApp        -- `*((_DWORD *)this + 64) = a2` in the ctor and
//                                   every LawnApp call in Update/DealClick/DrawDave
//   +0x104  int      mHelpType   -- DaveEnter stores its E_DAVEHELP parameter here
//                                   and StartHelp reads it back in DealClick
//   +0x108  int      mSingleType -- DaveSingleEnter stores its E_DAVESINGLEHELP here
//                                   and Update switches on it
//   +0x10C  bool     mIsSingle   -- DaveEnter clears it, DaveSingleEnter sets it;
//                                   Update takes the mSingleType branch when set
//   +0x110  WidgetContainer *mContainer -- DaveEnter/DaveSingleEnter store their
//                                   container here, then call its slot 6 AddWidget
//                                   and clear the word once the widget is gone
//   +0x114  int      mCounter    -- the countdown Update decrements, 0 when done
//   +0x118  bool     mDrawActive -- DaveEnter/DaveSingleEnter store their bool
//                                   parameter here; Draw tests it before the overlay
// The three bools are byte stores in the ARM (`STRB.W R0,[R4,#0x10C]` at
// 0x1ec554), and the 0x11C allocation closes right after the last word.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Graphics.h"
#include "Reanimation.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"

// DaveHelp stores only `LawnApp *mApp` and takes one as its constructor
// parameter, so the include was half of the LawnApp.h <-> DaveHelp.h cycle and
// nothing here needs the complete type. Sexy\ReconTypes.h already declares it.
class LawnApp;

class DaveHelp : public Sexy::Widget
{
public:
    virtual ~DaveHelp();                                  // [ 0],[1] 0x000/0x004
    virtual void RemovedFromManager(Sexy::WidgetManager *theManager); // [30] 0x078
    virtual void Update();                                // [31] 0x07C
    virtual void Draw(Sexy::Graphics *theGraphics);               // [36] 0x090
    // Desktop input advances the same dialogue as the TV confirm button.
    void MouseDown(int theX, int theY, int theClickCount) override;

    LawnApp         *mApp;        // +0x100
    int              mHelpType;   // +0x104  an E_DAVEHELP
    int              mSingleType; // +0x108  an E_DAVESINGLEHELP
    bool             mIsSingle;   // +0x10C
    Sexy::WidgetContainer *mContainer; // +0x110
    int              mCounter;    // +0x114
    bool             mDrawActive; // +0x118
    // 0x11C

    // The remaining methods. None of these owns a vtable slot.
    explicit DaveHelp(LawnApp *theApp);
    int DaveEnter(int theHelpType, Sexy::WidgetContainer *theContainer, bool theDrawActive);
    int DaveSingleEnter(E_DAVESINGLEHELP theSingleType, Sexy::WidgetContainer *theContainer, bool theDrawActive);
    void DaveLeave();
    int DealClick(Sexy::KeyCode theKey);
    void DrawDave(Sexy::Graphics *theGraphics);
};

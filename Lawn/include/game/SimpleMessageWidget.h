// SimpleMessageWidget -- reconstructed from libGameMain.so.
//
// NOT polymorphic: there is no RTTI record and no vtable export for
// SimpleMessageWidget anywhere in the binary (libGameMain115_rtti.c has none),
// and every call site is a direct call (`SimpleMessageWidget::Update(v9)`,
// `SimpleMessageWidget::Draw(v9, g)`), never a virtual dispatch. So the object
// has no vptr and the class has no base class.
//
// Size 0x31C, from the only allocation site:
//     SessionWaitingDialog.cpp:134   SimpleMessageWidget allocation, 0x31C bytes
//
// Member list -- OBSERVED, not inferred. Recovered from the four bodies that
// exist, each of which reaches a different field:
//   ClearReanim @0x1EAE14  walks `this + 144` to `this + 656` in steps of 4 and
//                          passes each word to LawnApp::ReanimationTryToGet, so
//                          +0x90 is a 128-entry ReanimationID array and
//                          `*(_DWORD *)this` (word 0) is the LawnApp*.
//   Update      @0x1EAF24  does `strlen((const char *)this + 4)`, so the label
//                          is at +0x04, not +0x00.
//   ClearLabel  @0x1EAF04  reads words 34, 164 and 165.
//   ~SimpleMessageWidget @0x1EAE3C is one ClearReanim call.
// Size 0x31C, from the allocation site (SessionWaitingDialog.cpp:71,
// `new SimpleMessageWidget(a2)` over a 0x31C-byte block).
//
// The class HAS the `LawnApp *mApp` field the earlier note assumed it lacked --
// word 0 is read as the app by both ClearReanim and Update. What it lacks is
// MessageWidget's SyncObject base, and the three unkWords below are what the
// earlier guess had mis-named as mDuration / mMessageStyle / mReanimType.
//
// `SimpleMessageStyle` is MessageStyle (ReconTypes.h); the one call site passes
// the literal 7 (SessionWaitingDialog.cpp:72).

#pragma once

#include "ReconTypes.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"

class LawnApp;
class Reanimation;

// The message style the SetLabel call passes; MessageStyle in the binary.
typedef MessageStyle SimpleMessageStyle;

class SimpleMessageWidget
{
public:
    explicit SimpleMessageWidget(LawnApp *theApp);
    ~SimpleMessageWidget();

    void SetLabel(const Sexy::String& theLabel, SimpleMessageStyle theStyle);
    void ClearLabel();
    void ClearReanim();

    void Update();
    void Draw(Sexy::Graphics *theGraphics);
    void DrawReanimatedText(Sexy::Graphics *theGraphics, Sexy::Font *theFont, const Sexy::Color& theColor, float thePosX);
    Sexy::Font *GetFont();
    void LayoutReanimText();
    int IsBeingDisplayed();

    LawnApp *mApp;
    char mLabel[128];
    int mDisplayTime;
    ReanimationID mTextReanimID[128];
    int mSlideOffTime;
    char mLabelNext[128];
    MessageStyle mMessageStyleNext;
};


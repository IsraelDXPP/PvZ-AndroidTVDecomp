// StretchableGameButton -- reconstructed from libGameMain.so.
//
// Inheritance chain: `class StretchableGameButton : public GameButton`
// (phase-1 group table; confirmed by the binary's ZTV21StretchableGameButton
// @0x683e30, whose slots 2..29 are the Sexy::WidgetContainer / Widget entries and
// whose destructor chains into GameButton's).
//
// vtable -- this class adds NO virtual of its own; it overrides four of
// GameButton's:
//   [  0] 0x1a21e8  StretchableGameButton::~StretchableGameButton
//   [  1] 0x1a2208  StretchableGameButton::~StretchableGameButton
//   [ 50] 0x1a2074  StretchableGameButton::Resize          (overrides Sexy::Widget::Resize)
//   [125] 0x1a3acc  StretchableGameButton::DrawButtonImage (overrides GameButton's)
//   [126] 0x1a2014  StretchableGameButton::SetFont         (overrides GameButton's)
//   [127] 0x1a2878  StretchableGameButton::SetLabel        (overrides GameButton's)
//   [134] 0x1a2108  StretchableGameButton::SetPadding      (overrides GameButton's)
// The header total is therefore 135 slots (0..134), GameButton's own count.
// The return types are GameButton's own -- a same-named, same-parameter derived
// declaration is an implicit override, so a differing return type would be C2555.
// The transcribed bodies returned `int`/`Font *`/`StretchableGameButton *` for
// these; they have been retyped to `void` to match, which is what the binary's
// own callers ignore anyway (each of those values is only the leftover of r0).
//
// MSVC x86 has ONE virtual-destructor slot where Itanium/ARM has D1+D0; only the
// declaration order matters.
//
// Size 0x3A8, from the constructor call site (`an allocation of 0x3a8u` followed by
// StretchableGameButton::StretchableGameButton(...) @0x1a39a0).
// TODO(recon): GameButton / ButtonWidget are still opaque here, so the derived
// member area cannot be placed exactly; the bodies reach 0xAB/0xAC, 0xE6..0xE9
// and 0x120.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Font.h"
#include "GameButton.h"
#include "Graphics.h"
#include <string>
#include <vector>
#include <map>
#include <set>

class StretchableGameButton : public GameButton
{
public:
    virtual ~StretchableGameButton();                                  // [  0],[1] 0x000/0x004
    virtual void Resize(int theX, int theY, int theWidth, int theHeight); // [ 50] 0x0C8
    virtual void DrawButtonImage(Sexy::Graphics *theGraphics, Sexy::Image *theImage, TRect<int> const& theRect, int theX, int theY); // [125] 0x1F4
    virtual void SetFont(Sexy::Font *theFont);                         // [126] 0x1F8
    virtual void SetLabel(Sexy::String const& theLabel);               // [127] 0x1FC
    virtual void SetPadding(int thePadX, int thePadY);                 // [134] 0x218

    // Derived member area. Sexy::ButtonWidget is 0x330 and GameButton is 0x398,
    // and both now carry their real members, so this class' own area is exactly
    // the sixteen bytes +0x398..+0x3A7.
    // The pair this replaced (`mUnk120` + `mUnk124[161]`) came from reading the
    // dump's WORD indices 72 and 230..233 as byte offsets: word 72 is
    // ButtonWidget::mLabel (+0x120) and words 171/172 are mLabelJustify/mFont,
    // so it mis-named three base members and overlapped all of ButtonWidget and
    // GameButton. The four real members below occupy the same sixteen bytes, so
    // sizeof is unchanged at 0x3A8.
    int mTextWidth;                // +0x398, the label's measured text width
    int mPadX;                     // +0x39C, SetPadding's first argument
    int mPadY;                     // +0x3A0, SetPadding's second argument
    BUTTON_ALIGNMENT_X mAlignment; // +0x3A4, 0 = centred, -1 = right, 1 = left
    // 0x3A8

    // The remaining methods. None of these owns a vtable slot.
    // The demangled constructor takes ELEVEN parameters:
    //   _ZN22StretchableGameButtonC2EiPN4Sexy14ButtonListenerERKSsiiii19BUTTON_ALIGNMENT_XiiPNS0_4FontE
    // i.e. (int, ButtonListener*, String const&, int x5, BUTTON_ALIGNMENT_X, int x2, Font*).
    // The leading `GameButton *a1` the transcription emitted was the implicit
    // `this`, not a parameter.
    StretchableGameButton(int theId, Sexy::ButtonListener *theListener, Sexy::String const& theLabel, int theX, int theY, int theWidth, int theHeight, BUTTON_ALIGNMENT_X theAlignment, int thePadX, int thePadY, Font *theFont);
    int DrawStretchableGameButton(Sexy::Graphics *theGraphics, Sexy::Image *theImage, TRect<int> const& theRect);
    int DrawStretchableGameButton(Sexy::Graphics *theGraphics, Sexy::Image *theImage, int theX, int theY, int theWidth, int theHeight);
};


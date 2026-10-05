// StartButton -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class StartButton : public Sexy::ButtonWidget`, exactly as
// the authoritative header PvZ\SexyAppFramework\Widget\StartButton.h:28 declares
// it. Note the namespace: this class is GLOBAL, not in Sexy.
//
// vtable order -- from the RTTI export _ZTV11StartButtonE (134 function slots).
// All but five slots hold WidgetContainer/Widget/ButtonWidget code; the class's
// own entries, in declaration order (MSVC x86 collapses the Itanium D2/D0 pair
// into one destructor slot):
//   [  0] 0x1a6554  StartButton::~StartButton
//   [  1] 0x1a65b4  StartButton::~StartButton
//   [ 36] 0x1a62f0  StartButton::Draw
//   [ 71] 0x1a6cbc  StartButton::KeyDown
//   [126] 0x1a627d  StartButton::SetFont
// Slot [126] is the slot StartButton itself appends: ButtonWidget has 126 slots
// (0..125) and SetFont is the 127th.
//
// Members, names and offsets are from the authoritative header; the binary's
// constructor writes the same fields (mFont = 0 at +0x354, mLabel = the shared
// empty-string rep at +0x358, mGamepadIndex = -1 at +0x35C), and Draw reads
// mColor at +0x330, mOverColor at +0x340 and mTextAlpha as a float at +0x364.
// ButtonWidget is 0x330 bytes -- the identically laid-out HyperlinkWidget has
// its mColor at the same offset.
//   The authoritative header notes "218 ints"; mUnderlineOffset is the last
//   member and lands at +0x368, so the object is 0x36C.

#pragma once

#include "ReconTypes.h"
// The transcribed bodies call methods on these, so the includes stay.
#include "ButtonWidget.h"
#include "Font.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "Widget.h"

class TitleScreen;

class StartButton : public Sexy::ButtonWidget
{
public:
    virtual ~StartButton();

    // --- this class's own vtable entries, in table order -------------------
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual void SetFont(Sexy::Font *theFont);

    // --- members -----------------------------------------------------------
    Sexy::Color  mColor;           // +0x330 (16 bytes)
    Sexy::Color  mOverColor;       // +0x340 (16 bytes)
    TitleScreen *mParent;          // +0x350
    Sexy::Font  *mFont;            // +0x354
    // A second, start-button-local label: the constructor seeds it with the
    // shared empty rep (0x1A6A10 stores Sexy::gEmptyStringData, the same constant
    // ButtonWidget uses for its own mLabel) and Draw passes it to
    // Graphics::DrawString. ButtonWidget's mLabel is at +0x120, well below
    // StartButton's first own member, so the two are genuinely distinct fields
    // and this declaration hides the base name inside StartButton's scope.
    Sexy::String mLabel;           // +0x358 (4 bytes, a one-pointer COW string)
    int          mGamepadIndex;    // +0x35C
    // 2 is the pulsing mode Draw animates with mTextAlpha.
    int          mTextDrawMode;    // +0x360
    float        mTextAlpha;       // +0x364
    int          mUnderlineOffset; // +0x368
    // size 0x36C

    StartButton(int theId, Sexy::Widget *theParent, Sexy::ButtonListener *theListener);
    int GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theGamepadIndex);
    void GameButtonUp();
};

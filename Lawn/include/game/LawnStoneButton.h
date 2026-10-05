// LawnStoneButton -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class LawnStoneButton : public GameButton`.
// The binary's RTTI base for _ZTV15LawnStoneButton is `10GameButton`, and the
// table inherits GameButton::Update [31], SetDisabled [48], Resize [50],
// FocusDirectionHint [60] and KeyDown [71] unchanged.
//
// No data members of its own. The only this-relative access in
// LawnStoneButton.cpp is `(this + 288)` at :32, which is GameButton's label slot
// at +0x120 (see Runtime\SexyString.h, which records the same offset), so the
// object is exactly as large as GameButton. The transcribed object was 0x128,
// which is smaller than GameButton's own 0x398 and therefore cannot be the size.
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x1a221c  LawnStoneButton::~LawnStoneButton
//   [  1] 0x1a223c  LawnStoneButton::~LawnStoneButton
//   [ 36] 0x1a37b8  LawnStoneButton::Draw
//   [127] 0x1a2810  LawnStoneButton::SetLabel
// Draw's body is a single tail call to GameButton::Draw, and SetLabel is the only
// real override: it translates the label and stores it into the inherited slot.
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "GameButton.h"
#include <string>
#include <vector>
#include <map>
#include <set>

class LawnStoneButton : public GameButton
{
public:
    virtual ~LawnStoneButton();
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void SetLabel(const Sexy::String& theLabel);

    LawnStoneButton(int theId, Sexy::ButtonListener* theListener);

    // No members of its own; every field it touches belongs to GameButton.
    // size == sizeof(GameButton), see GameButton.h
};

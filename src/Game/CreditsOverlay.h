// CreditsOverlay -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class CreditsOverlay : public Sexy::Widget`.
// The binary's RTTI base for _ZTV14CreditsOverlay is N4Sexy6WidgetE, and every
// WidgetContainer slot in the table is unchanged.
//
// Object size 0x104. Sexy::Widget is 0x100 and Draw reads a pointer from +0x100
// (CreditsOverlay.cpp:15-17 calls a virtual on it, so it is the owning screen).
// The transcribed object was 0x108.
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x1b908c  CreditsOverlay::~CreditsOverlay
//   [  1] 0x1b90ac  CreditsOverlay::~CreditsOverlay
//   [ 36] 0x1b9060  CreditsOverlay::Draw
// The body is a single tail call to the parent screen's own draw entry, which is
// why the recovered signature has no parameter: the parameter was dropped when the
// body became one indirect call. The framework's Draw takes a Sexy::Graphics *.
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Widget.h"

class CreditScreen;

class CreditsOverlay : public Sexy::Widget
{
public:
    virtual ~CreditsOverlay();
    virtual void Draw(Sexy::Graphics *theGraphics);

    // +0x100 .. +0x103 (the first 0x100 bytes are Sexy::Widget).
    // The PC reference source declares the same member as `CreditScreen* mParent`
    // (LawnProject\src\Lawn\CreditScreen.h); the body only ever calls a virtual on
    // it, so the type is consistent but not independently confirmed here.
    CreditScreen *mParent;   // +0x100
    // size 0x104
};

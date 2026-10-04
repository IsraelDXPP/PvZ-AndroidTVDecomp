// GameOverDialog -- reconstructed from libGameMain.so.
//
// `class GameOverDialog : public LawnDialog`.
//   RTTI: `_ZTI14GameOverDialog` @0x67CE08 records `_ZTI10LawnDialog`
//   @0x67CDFC as its base. The vtable agrees: slots 31/36/50/71 hold
//   `LawnDialog::Update` (0x141188), `LawnDialog::Draw` (0x13ef00),
//   `LawnDialog::Resize` (0x13f3b8) and `LawnDialog::KeyDown` (0x13eb3c), slot 45
//   is `Sexy::Dialog::SetColor` (0x301f84). The `non-virtual thunk to
//   GameOverDialog::~GameOverDialog` and `... ButtonDepress` symbols are the
//   `Sexy::ButtonListener` sub-object adjustment Dialog introduces.
//
// SIZE = sizeof(LawnDialog) = 0x2FC; this class adds NO data members.
//   GameOverDialog.cpp's member accesses are +0x3C/+0x40 (WidgetContainer's
//   mWidth/mHeight), +0x45 (Widget::mClip), +0xDC, +0x2B0, +0x2E0 (LawnDialog::mApp),
//   +0x2F0, +0x2F4 and +0x2FB -- all inside LawnDialog. The transcriber's 0x3A0 came
//   from a vtable-slot read transcribed as a member access: the word at +0x390 is
//   loaded and then called as a function pointer at GameOverDialog.cpp:38, so +0x390
//   is a slot offset, not a member.
//   TODO(recon): GameOverDialog.cpp:24 and :171 store a vtable pointer at byte
//   +0x100, which is inside the Widget base -- that is either an inlined
//   sub-object construction or a transcription artifact. It does not look like a
//   member of this class, so no field is declared for it.
//
// VTABLE -- 133 slots; the four own slots are:
//   [  0] 0x13e6c0  ~GameOverDialog (deleting)
//   [ 29] 0x141138  AddedToManager
//   [ 30] 0x141290  RemovedFromManager
//   [ 61] 0x13e9d8  GotFocus
//   [ 81] 0x13eab0  MouseDrag   (recovered from the symbol inventory; it is past
//                               the 80 slots the transcriber printed)
// `ButtonDepress` is a `Sexy::ButtonListener` slot inherited through Dialog.
// The Itanium table has a D0/D1 destructor pair; MSVC x86 keeps one slot.
//
// NOTE: the transcribed .cpp spells these with `int` returns; the base classes
// declare `void`, which is what an override needs.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "LawnDialog.h"

// This file never names LawnApp -- there is no member, parameter or return value
// of that type -- so the include was pure cycle (LawnApp.h <-> GameOverDialog.h)
// and dragged all 75 of LawnApp.h's includes in for nothing.
class LawnApp;

class GameOverDialog : public LawnDialog
{
public:
    virtual ~GameOverDialog();

    // slot 29
    virtual void AddedToManager(Sexy::WidgetManager * theWidgetManager);
    // slot 30
    virtual void RemovedFromManager(Sexy::WidgetManager * theWidgetManager);
    // slot 61
    virtual void GotFocus();
    // slot 81
    virtual void MouseDrag(int theX, int theY);
    // `Sexy::ButtonListener` slot, reached through Dialog's second base.
    virtual void ButtonDepress(int theId);

    // ---- no own data members; base LawnDialog = 0x2FC ----

    GameOverDialog(const Sexy::String& theMessage, bool theShowTrophy);
};

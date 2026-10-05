// ConfirmBackToMainDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class ConfirmBackToMainDialog : public LawnDialog`
// (RTTI "23ConfirmBackToMainDialog", primary base "10LawnDialog" at offset 0,
// vtable 0x67d518). Single inheritance, so the base's 0x2FC bytes are followed
// directly by this class's only member.
//
// Object size 0x300: LawnDialog is 764 = 0x2FC and the class adds one pointer.
// The authoritative header's `// 115: 192, 111: 194` records that the two game
// versions place mRestartButton at slightly different ordinals; this .so writes
// it at +764 = 0x2FC (ConfirmBackToMainDialog.cpp:205-207 stores the button made
// by `MakeButton(1, ...)` there), and both AddedToManager (:266) and
// RemovedFromManager (:276) pass that same word to WidgetContainer::AddWidget /
// RemoveWidget, so 0x300 is the size for this version. The transcribed class body
// was an opaque 0x3A0-byte block, inflated by two mis-attributed `this` accesses
// (+800 and +912) that are actually calls through a function pointer.
//
// vtable order -- this class's own slots (133 in all); the rest are LawnDialog's,
// Dialog's and Widget's:
//   [  0] 0x13e474  ~ConfirmBackToMainDialog   (D1)
//   [  1] 0x13e4c0  ~ConfirmBackToMainDialog   (D0)
//   [ 29] 0x14113c  AddedToManager             (override)
//   [ 30] 0x141294  RemovedFromManager         (override)
//   [ 61] 0x13e9fc  GotFocus                   (override)
//   [ 81] 0x13..    MouseDrag                  (override)
//   [129] 0x13..    ButtonDepress              (override)
// Slots 31 (LawnDialog::Update), 36 (LawnDialog::Draw), 71 (LawnDialog::KeyDown)
// and 76..80 (Sexy::Dialog::MouseDown/MouseUp) still hold the base
// implementations, which is why the authoritative header does not declare them.
// MSVC x86 gives a virtual destructor ONE slot where Itanium gives two (D1+D0).

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "Dialog.h"
#include "GameButton.h"
#include "LawnDialog.h"

// This file never names LawnApp -- neither a member nor a parameter -- so the
// include was pure cycle: it closed LawnApp.h <-> ConfirmBackToMainDialog.h and
// dragged the whole of LawnApp.h in for nothing. The declaration is kept because
// Sexy\ReconTypes.h, which every header here includes, already provides it.
class LawnApp;

class ConfirmBackToMainDialog : public LawnDialog
{
public:
    virtual ~ConfirmBackToMainDialog();

    // The five base slots this class overrides. Every one of them returns void in
    // the base, so the decompiler's `int`/`_DWORD *` spellings were C2555 rather
    // than overloads -- a rejected override becomes a NEW virtual and shifts the
    // slots after it. Sources for `void`:
    //   * PvZ\Lawn\Widget\LawnDialog.h: AddedToManager / RemovedFromManager /
    //     GotFocus / ButtonDepress, and Game\LawnDialog.h declares them `void`.
    //   * PvZ\Lawn\Widget\ConfirmBackToMainDialog.h: all four of its
    //     `old_ConfirmBackToMainDialog_MouseDrag / _AddedToManager /
    //     _RemovedFromManager / _ButtonDepress` hook typedefs return void.
    //   * Sexy\Dialog.h slot [81] is `virtual void MouseDrag(int x, int y)`.
    virtual void AddedToManager(Sexy::WidgetManager *theManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theManager);
    virtual void GotFocus();
    virtual void MouseDrag(int theX, int theY);
    virtual void ButtonDepress(int theId);

    // LawnDialog occupies +0x00..+0x2FB.
    GameButton *mRestartButton;   // +0x2FC
    // Size 0x300.

    explicit ConfirmBackToMainDialog(bool theLeaveGame);
};

// WaitForSecondPlayerDialog -- reconstructed from libGameMain.so.
//
// Inheritance chain: `class WaitForSecondPlayerDialog : public LawnDialog`
// (phase-1 group table; confirmed by the binary's ZTV25WaitForSecondPlayerDialog
// @0x67de38: 134 primary slots plus a secondary vtable at +0x100 with
// offset-to-top -0x100).
//
// vtable -- this class overrides exactly four slots:
//   [  0] 0x13e4dc  WaitForSecondPlayerDialog::~WaitForSecondPlayerDialog
//   [  1] 0x13e510  WaitForSecondPlayerDialog::~WaitForSecondPlayerDialog
//   [ 71] 0x13edf4  WaitForSecondPlayerDialog::KeyDown
//   [133] 0x13f2a8  WaitForSecondPlayerDialog::GameButtonDown (a NEW virtual this class appends)
// Every other slot still points at LawnDialog / Sexy::Dialog / Sexy::Widget.
// TODO(recon): the transcribed body returns `int` from KeyDown, so
// Game\WaitForSecondPlayerDialog.cpp must be retyped in the same pass.
//
// MSVC x86 has ONE virtual-destructor slot where Itanium/ARM has D1+D0; only the
// declaration order matters.
//
// Size 0x300, from the constructor call site (`an allocation of 0x300u` followed by
// WaitForSecondPlayerDialog::WaitForSecondPlayerDialog(LawnApp*) @0x1400f4).
// The authoritative header PvZ\Lawn\Widget\WaitForSecondPlayerDialog.h:71 names
// only `bool m2PJoined` and says the 1.15 build's object is 192 bytes; the 0x300
// allocation and the offsets the bodies reach (+0x2E0 five times, +0x2FC four
// times) belong to this build's larger class.
// TODO(recon): LawnDialog is still opaque in this tree, so the derived member
// area cannot be placed exactly.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "GamepadApp.h"
#include "LawnDialog.h"

// LawnApp is only the constructor's pointer parameter
// (`WaitForSecondPlayerDialog(LawnApp *)`), so it is declared rather than
// included: the include was half of the LawnApp.h <-> WaitForSecondPlayerDialog.h
// cycle. Sexy\ReconTypes.h already carries it.
class LawnApp;

class WaitForSecondPlayerDialog : public LawnDialog
{
public:
    virtual ~WaitForSecondPlayerDialog();                          // [  0],[1] 0x000/0x004
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theGamepadIndex);

    // LawnDialog ends at 0x2FC. Set once the second gamepad joins.
    bool m2PJoined;

    // The remaining methods. None of these owns a vtable slot.
    explicit WaitForSecondPlayerDialog(LawnApp *theApp);
};

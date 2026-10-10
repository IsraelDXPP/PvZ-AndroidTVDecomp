// BuyMoneyDialog -- reconstructed from libGameMain.so.
//
// Inheritance chain: `class BuyMoneyDialog : public LawnDialog`
// (phase-1 group table; confirmed by the binary's ZTV14BuyMoneyDialog @0x688038:
// 134 primary slots plus a secondary vtable at +0x100 with offset-to-top -0x100).
//
// vtable -- this class overrides exactly twelve slots:
//   [  0] 0x2c3a40  BuyMoneyDialog::~BuyMoneyDialog
//   [  1] 0x2c3abc  BuyMoneyDialog::~BuyMoneyDialog
//   [ 29] 0x2c424c  BuyMoneyDialog::AddedToManager
//   [ 30] 0x2c4288  BuyMoneyDialog::RemovedFromManager
//   [ 36] 0x2c42c4  BuyMoneyDialog::Draw
//   [ 50] 0x2c3ba8  BuyMoneyDialog::Resize
//   [ 61] 0x2c4330  BuyMoneyDialog::GotFocus
//   [ 71] 0x2c430c  BuyMoneyDialog::KeyDown
//   [ 72] 0x2c4310  BuyMoneyDialog::KeyUp
//   [125] 0x2c3ba4  BuyMoneyDialog::GetPreferredHeight   (overrides Sexy::Dialog's)
//   [129] 0x2c42c8  BuyMoneyDialog::ButtonDepress        (overrides Sexy::Dialog's)
//   [133] 0x2c4330  BuyMoneyDialog::GotGameFocus          (a NEW virtual this class appends)
// The return types are the bases' own -- a same-named, same-parameter derived
// declaration is an implicit override, so a differing return type would be C2555.
// TODO(recon): the transcribed bodies use `int` returns for several of these, so
// Game\BuyMoneyDialog.cpp must be retyped in the same pass.
//
// MSVC x86 has ONE virtual-destructor slot where Itanium/ARM has D1+D0; only the
// declaration order matters.
//
// Size 0x318, from the constructor call site (`an allocation of 0x318u` followed by
// BuyMoneyDialog::BuyMoneyDialog(LawnApp*, std::string const&) @0x2c3bac).
// The derived area starts at 0x2FC, because LawnDialog really is 0x2FC bytes
// (Sexy::Dialog 0x2E0 plus its seven own words), and the constructor's own
// stores pin the seven words below. The dialog has no header in the
// authoritative tree, so nothing between +0x100 and +0x2FC can be named.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_BUYMONEYDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_BUYMONEYDIALOG_H

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "LawnDialog.h"
#include "LawnStoneButton.h"
#include "LawnUser.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// LawnApp appears only as the constructor's pointer parameter, so it is declared
// rather than included: the include was half of the LawnApp.h <-> BuyMoneyDialog.h
// cycle, and a cycle makes the two entry orders reach different states.
class LawnApp;

class BuyMoneyDialog : public LawnDialog
{
public:
    virtual ~BuyMoneyDialog(); // [  0],[1] 0x000/0x004
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager); // [ 29] 0x074
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // [30] 0x078
    virtual void Draw(Sexy::Graphics *theGraphics); // [ 36] 0x090
    virtual void Resize(int theX, int theY, int theWidth, int theHeight); // [ 50] 0x0C8
    virtual void GotFocus(); // [ 61] 0x0F4
    virtual int KeyDown(Sexy::KeyCode theKey); // [ 71] 0x11C
    virtual int KeyUp(Sexy::KeyCode theKey); // [ 72] 0x120
    virtual int GetPreferredHeight(int theWidth); // [125] 0x1F4
    virtual void ButtonDepress(int theId); // [129] 0x204
    virtual void GotGameFocus(); // [133] 0x214 (new virtual)

    // Derived member area, from the constructor's own stores (@0x2c3c62 and
    // @0x2c3c86..0x2c3cb8). The class has no header in the authoritative tree,
    // so these seven words are the only members that can be placed: everything
    // between Sexy::Dialog's ButtonListener vptr at +0x100 and +0x2FC belongs
    // to Dialog and LawnDialog, which really are 0x2E0 and 0x2FC bytes.
    //
    std::vector<LawnStoneButton *> mTierButtons; // +0x2FC
    int mBoughtTier; // +0x308  the tier the player pressed
    int mDialogWidth; // +0x30C  506: the width the dialog resizes itself to
    int mRowPitch; // +0x310  35: what one tier row contributes to the height
    int mFirstRowY; // +0x314  120: the first tier row's Y
    // 0x318

    // The remaining methods. None of these owns a vtable slot.
    BuyMoneyDialog(LawnApp *theApp, Sexy::String const &theTierName);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_BUYMONEYDIALOG_H

// BuySunMoneyDialog -- reconstructed from libGameMain.so.
//
// Real inheritance: `class BuySunMoneyDialog : public LawnDialog`
// (LawnDialog : Sexy::Dialog : (Sexy::Widget, Sexy::ButtonListener)). The vtable
// confirms it: slots 0..132 are exactly LawnDialog's 133 slots, so this class adds
// no vtable slot of its own -- it only overrides.
//
// Size 0x300, from `(BuySunMoneyDialog *)operator new of 0x300 bytes` at Zombie.cpp:738.
// LawnDialog ends at 0x2FC, so one field remains; Zombie.cpp's own bodies and the
// dialog's FrameCounter use it. The old placeholder said 0x3A0.
//
// vtable order -- slots 0..132 come from the bases and are NOT redeclared here.
// This class only overrides base slots:
//   [ 29] 0x14115c  BuySunMoneyDialog::AddedToManager
//   [ 30] 0x1412b4  BuySunMoneyDialog::RemovedFromManager
//   [ 61] 0x13ea68  BuySunMoneyDialog::GotFocus
//   [ 81] 0x13eac0  BuySunMoneyDialog::MouseDrag
//   [129] 0x13ee58  BuySunMoneyDialog::ButtonDepress

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_BUYSUNMONEYDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_BUYSUNMONEYDIALOG_H

#include "ReconTypes.h"
// The base class, plus the classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "Zombie.h"

class BuySunMoneyDialog : public LawnDialog
{
public:
    virtual ~BuySunMoneyDialog();

    // Overrides of base slots.
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager); // [ 29] 0x14115c
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // [ 30] 0x1412b4
    virtual void GotFocus(); // [ 61] 0x13ea68
    virtual void MouseDrag(int x, int y); // [ 81] 0x13eac0
    virtual void ButtonDepress(int theId); // [129] 0x13ee58

    // Not a vtable slot; the signature is the one the transcribed body defines.
    BuySunMoneyDialog(Zombie *theZombie, int theSunAmount);

    // +0x2FC is one word past LawnDialog's 0x2FC bytes, so this is the class's
    // first own member. The constructor's first argument is stored here
    // (`*((_DWORD *)this + 191) = a2`, BuySunMoneyDialog.cpp:281) and
    // ButtonDepress calls BuyRevivalItem / BuySunMoneyItem through it.
    Zombie *mZombie; // +0x2FC
    // size 0x300, allocation 0x300
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_BUYSUNMONEYDIALOG_H

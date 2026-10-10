// PurchaseLimitDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class:
//     class PurchaseLimitDialog : public LawnDialog, public Sexy::EditListener
//
// LawnDialog is 0x2FC bytes (Game\LawnDialog.h), and the constructor stores a
// vptr at exactly +0x2FC, one word past its end -- the same shape NewUserDialog
// has (Game\NewUserDialog.h), and the same secondary table:
//     CreateEditWidget(0, (Sexy::EditListener *)((char *)this + 0x2FC), this)
// The non-virtual thunk `thunk to'AllowChar subtracts 0x2FC, which is what puts
// AllowChar in that table rather than in the primary one.
//
// The constructor's other vptr store, at +0x100, is Sexy::Dialog's
// Sexy::ButtonListener sub-object (the destructor's thunk adjusts by -256);
// both bases are real now, so the compiler emits both stores.
//
// vtable order -- the class's own entries, in slot order (MSVC x86 collapses the
// Itanium D2/D0 pair into one destructor slot):
//   [  0] 0x1eca8c  PurchaseLimitDialog::~PurchaseLimitDialog
//   [  1] 0x1ecb58  PurchaseLimitDialog::~PurchaseLimitDialog
//   [ 29] 0x1ed00c  PurchaseLimitDialog::AddedToManager
//   [ 30] 0x1ed054  PurchaseLimitDialog::RemovedFromManager
//   [ 36] 0x1ed06c  PurchaseLimitDialog::Draw
//   [ 71] 0x1ed0f4  PurchaseLimitDialog::KeyDown
//   [ 72] 0x1ed100  PurchaseLimitDialog::KeyUp
//
// Members. +0x2F0 and +0x2F4 are NOT this class's -- they are LawnDialog's
// mLawnYesButton / mLawnNoButton, which the constructor hands to the three
// SetFocusLinks calls. Everything below is this class's own, and the +0x2F0 /
// +0x2F4 offsets are absolute in the binary:
//     +0x2FC       the Sexy::EditListener sub-object
//     +0x300       the constructor's second argument (the id BuyMoney takes)
//     +0x304       the constructor's first argument (the LawnApp)
//     +0x308       mEditWidget
//     +0x30C       bool: set when the typed key did not match, cleared by
//                  AllowChar and by the constructor
//     +0x310/+0x314  the two Sexy::String members the constructor fills with
//                  TodStringTranslate + SexyUtf8ToWString; +0x314 is the one
//                  Draw prints in red
//     size 0x318

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_PURCHASELIMITDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_PURCHASELIMITDIALOG_H

#include "ReconTypes.h"
// The transcribed bodies call methods on these, so the includes stay.
#include "Dialog.h"
#include "EditListener.h"
#include "EditWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "LawnDialog.h"
#include "SexyCommonApp.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// LawnApp is only ever a pointer here, so it is declared rather than included:
// the include was half of the LawnApp.h <-> PurchaseLimitDialog.h cycle.
// Sexy\ReconTypes.h already carries it.
class LawnApp;

class PurchaseLimitDialog : public LawnDialog, public Sexy::EditListener
{
public:
    virtual ~PurchaseLimitDialog();

    // --- this class's own vtable entries, in table order -------------------
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager); // [29]
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // [30]
    virtual void Draw(Sexy::Graphics *theGraphics); // [36]
    virtual int KeyDown(Sexy::KeyCode theKey); // [71]
    virtual int KeyUp(Sexy::KeyCode theKey); // [72]

    // --- the EditListener slot this class overrides (secondary table) -------
    // Sexy\EditListener.h declares the return type as int; the binary's body
    // returns the same 0/1 the other two values are, so the base's int is kept.
    virtual int AllowChar(int theId, char theChar); // [3]

    // --- members, +0x300.. (the EditListener base owns +0x2FC) -------------
    int mPurchaseId; // +0x300, the id BuyMoney takes
    LawnApp *mApp; // +0x304
    Sexy::EditWidget *mEditWidget; // +0x308
    bool mKeyFailed; // +0x30C, the typed key did not match
    Sexy::String mPrompt; // +0x310, translated prompt, unused by Draw
    Sexy::String mFailedText; // +0x314, drawn in red after a bad key
    // size 0x318

    PurchaseLimitDialog(LawnApp *theApp, int theLimit);
    bool isalnum(char theChar);
    // Both of these return their String through r0 in the binary (ARM sret), so
    // the dump's receiver is the result buffer and its argument is the real
    // `this`. The real signature returns by value, which is what is declared.
    Sexy::String GetName();
    int SetKey(Sexy::String const &theKey);
    void ButtonDepress(int theId);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_PURCHASELIMITDIALOG_H

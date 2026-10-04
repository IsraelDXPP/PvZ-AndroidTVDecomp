// NewUserDialog -- reconstructed from libGameMain.so.
//
// Inheritance:
//     class NewUserDialog : public LawnDialog, public Sexy::EditListener
// Both the ctor (NewUserDialog.cpp:148) and the dtor (:58) write a second vptr at
// +764 = 0x2FC, and `_ZTV13NewUserDialog` @0x67F8D0 carries a third vtable group
// whose header word is 0xFFFFFD04 = -764 = -0x2FC, holding eight EditListener
// entries of which two are NewUserDialog thunks:
//     index 148  _ZThn764_NewUserDialog::EditWidgetText(int, const string &)
//     index 151  _ZThn764_NewUserDialog::AllowChar(int, char)
// 0x2FC is exactly where LawnDialog ends, so EditListener is the second base.
//
// SIZE 0x308. The largest own offset any body reaches is +772 (0x304) plus one
// word, and the base ends at 0x2FC. The transcriber's 0x3A0 is one of the
// "looks-shared-across-unrelated-classes" values build/map-lawn.md warns about;
// the header's own note already says the real end is 0x308.
//
// ---------------------------------------------------------------------------
// VTABLE @0x67F8D0, 135 primary slots. Own entries:
//   [  0] 0x14b4e8  ~NewUserDialog           override
//   [  1]           (ARM D0; MSVC x86 keeps ONE dtor slot)
//   [ 29] 0x14b8c4  AddedToManager           override
//   [ 30] 0x14b8ec  RemovedFromManager       override
//   [ 36] 0x14b944  Draw                     override
//   [ 50] 0x14b904  Resize                   override
//   [ 61] 0x14b95c  GotFocus                 override
//   --- slot 125 is new in NewUserDialog ---
//   [125] 0x14b6cc  GetPreferredHeight
// (The authoritative header also lists EditWidgetText/AllowChar; those live in the
// EditListener vtable at +0x2FC, not in the primary one.)

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "EditListener.h"
#include "EditWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "LawnDialog.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// LawnApp is reached only through `LawnApp *mApp` and the constructor's pointer
// parameter, so it is declared rather than included: the include was half of the
// LawnApp.h <-> NewUserDialog.h cycle. Sexy\ReconTypes.h already carries it.
class LawnApp;

class NewUserDialog : public LawnDialog, public Sexy::EditListener
{
public:
    // ---- own members, +0x300.. (the EditListener sub-object owns +0x2FC) ----
    LawnApp * mApp;                    // +0x300
    Sexy::EditWidget * mEditWidget;    // +0x304
    // size 0x308
    // TODO(recon): no body in this tree reads +0x308 or beyond; the old
    // `mObject[0x3A0]` had no evidence behind it.

    // ---- the LawnDialog/Dialog slots this class overrides, in binary order ----
    virtual ~NewUserDialog();                                                  // [  0]
    virtual void AddedToManager(Sexy::WidgetManager * theWidgetManager);       // [ 29]
    virtual void RemovedFromManager(Sexy::WidgetManager * theWidgetManager);   // [ 30]
    virtual void Draw(Sexy::Graphics * g);                                    // [ 36]
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);     // [ 50]
    virtual void GotFocus();                                                  // [ 61]

    // ---- slot 125: new in NewUserDialog ----
    virtual int GetPreferredHeight(int theWidth);                             // [125]

    // ---- the EditListener slots this class overrides (secondary vtable) ----
    // Sexy\EditListener.h numbers the narrow EditWidgetText at slot 0 and
    // AllowChar(int, char) at slot 3, which is exactly where the binary has
    // them, so both are real overrides and their return types are the base's.
    virtual void EditWidgetText(int theId, const Sexy::String & theText);
    virtual int AllowChar(int theId, char theChar);

    NewUserDialog(LawnApp * theApp, bool isRename);

    // GetName returns the trimmed edit text BY VALUE. The ARM at 0x14B774 takes
    // its receiver in r1 and writes the result through r0, i.e. it is an sret
    // function, so the old `NewUserDialog *` spelling was the hidden result
    // buffer mistaken for the return type. Same shape as
    // PurchaseLimitDialog::GetName (Game\PurchaseLimitDialog.cpp:132).
    Sexy::String GetName();
    int SetName(const Sexy::String & theName);
};


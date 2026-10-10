// CheatCodeDialog -- reconstructed from libGameMain.so.
//
// Real inheritance: `class CheatCodeDialog : public LawnDialog, public Sexy::EditListener`
// (LawnDialog : Sexy::Dialog : (Sexy::Widget, Sexy::ButtonListener)). The vtable
// confirms it: slots 122..127 are Dialog's own, slots 128..132 are LawnDialog's,
// and slots 133..134 are this class's.
//
// The second base is what the RTTI record says and what the layout requires:
// _recon/vtables.json's `15CheatCodeDialog` entry lists
// `[["10LawnDialog", 2], ["N4Sexy12EditListenerE", 195586]]`, and the
// constructor/destructor write a second vptr at +0x2FC -- one word past
// LawnDialog's 0x2FC bytes -- which is the EditListener sub-object. That is also
// the pointer the constructor hands to CreateEditWidget:
//     CreateEditWidget(0, (Sexy::EditListener *)((char *)this + 0x2FC), this)
// (CheatCodeDialog.cpp:215 in the dump). The two non-virtual thunks at
// 0x19216C and 0x192194 subtract exactly 0x2FC, which is the same fact.
//
// Size 0x308, from `(CheatCodeDialog *)operator new of 0x308 bytes` at
// LawnApp.cpp:1894: LawnDialog 0x2FC + the EditListener vptr 4 + mApp 4 +
// mEditWidget 4. The old placeholder's 0x3A0 was an over-allocation.
//
// vtable order -- slots 0..132 come from the bases and are NOT redeclared here.
// This class overrides these base slots:
//   [ 29] 0x1923d0  CheatCodeDialog::AddedToManager
//   [ 30] 0x1923f8  CheatCodeDialog::RemovedFromManager
//   [ 36] 0x192450  CheatCodeDialog::Draw
//   [ 50] 0x192410  CheatCodeDialog::Resize
//   [ 61] 0x192468  CheatCodeDialog::GotFocus
//   [125] 0x1923c4  CheatCodeDialog::GetPreferredHeight
// and appends two new ones, declared below in this order:
//   [133] 0x192154  CheatCodeDialog::EditWidgetText
//   [134] 0x192174  CheatCodeDialog::AllowChar

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_CHEATCODEDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_CHEATCODEDIALOG_H

#include "ReconTypes.h"
// The base class. Board was included here as well but is not named anywhere in
// this file, so that include was pure cycle (Board.h -> LawnApp.h ->
// CheatCodeDialog.h -> Board.h) and is replaced by the declaration below.
#include "Dialog.h"
#include "EditListener.h"
#include "EditWidget.h"
#include "Font.h"
#include "LawnDialog.h"

class Board;
// LawnApp is only ever a pointer here (`LawnApp *mApp`), so it stays a
// declaration: LawnApp.h includes this header, and including it back would
// re-close the cycle the note above is about. Sexy\ReconTypes.h already carries
// the declaration for the whole tree.

class CheatCodeDialog : public LawnDialog, public Sexy::EditListener
{
public:
    virtual ~CheatCodeDialog();

    // Overrides of base slots.
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager); // [ 29] 0x1923d0
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // [ 30] 0x1923f8
    virtual void Draw(Sexy::Graphics *g); // [ 36] 0x192450
    virtual void Resize(int theX, int theY, int theWidth, int theHeight); // [ 50] 0x192410
    virtual void GotFocus(); // [ 61] 0x192468
    virtual int GetPreferredHeight(int theWidth); // [125] 0x1923c4

    // The two entries this class appends to the PRIMARY table. EditWidgetText
    // fills EditListener's slot 0 as well, and AllowChar matches its slot 3, so
    // both are overrides of the listener base.
    virtual void EditWidgetText(int theId, const Sexy::String &theText);
    virtual int AllowChar(int theId, char theChar);

    explicit CheatCodeDialog(LawnApp *theApp);
    int ApplyCheat();

    // +0x2FC is the Sexy::EditListener base's vptr, so it is not a member.
    // +0x300 .. +0x307 (the first 0x2FC bytes are LawnDialog and the next 4 the
    // EditListener sub-object).
    LawnApp *mApp; // +0x300, the constructor's argument and the
    // object ApplyCheat reads mBoard from
    Sexy::EditWidget *mEditWidget; // +0x304, the CreateEditWidget result
    // size 0x308, allocation 0x308
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_CHEATCODEDIALOG_H

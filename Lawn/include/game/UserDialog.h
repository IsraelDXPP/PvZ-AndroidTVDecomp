// UserDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class with three bases:
//   class UserDialog : public LawnDialog, public Sexy::ListListener,
//                      public Sexy::EditListener
// The binary's RTTI base list for _ZTV10UserDialog is exactly
// 10LawnDialog + N4Sexy12ListListenerE + N4Sexy12EditListenerE.
//
// Layout, verified against UserDialog.cpp: the constructor writes a vptr into
// +0x2FC (the ListListener subobject) and another into +0x300 (the EditListener
// subobject); :210/:219/:235/:250 store mUserList, mRenameButton, mDeleteButton
// and mNumUsers at +0x304/+0x308/+0x30C/+0x310; :208 passes (this + 764) to the
// ListWidget constructor as its Sexy::ListListener *.
//
// That means LawnDialog is 0x2FC bytes and both listener bases are one vptr each,
// so the object is 0x314. Two dependencies are still open:
//   * LawnDialog.h currently holds the 0x710 placeholder, so the absolute offsets
//     above only become real once that header is reconstructed.
//   * ListListener.h currently holds an opaque eight-byte placeholder block. The
//     binary puts the EditListener subobject at +0x300, one word after the
//     ListListener subobject at +0x2FC, so Sexy::ListListener must be exactly
//     4 bytes (a pure interface with a vptr and no data).
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x17639c  UserDialog::~UserDialog
//   [  1] 0x176410  UserDialog::~UserDialog
//   [ 29] 0x176b20  UserDialog::AddedToManager
//   [ 30] 0x176b50  UserDialog::RemovedFromManager
//   [ 36] 0x176b80  UserDialog::Draw
//   [ 50] 0x176a94  UserDialog::Resize
//   [125] 0x176b14  UserDialog::GetPreferredHeight
//   [129] 0x176b84  UserDialog::ButtonDepress
// plus, in the ListListener and EditListener subobjects:
//   [133] 0x17642c  UserDialog::ListClicked
//   [134] 0x176354  UserDialog::EditWidgetText
//   [135] 0x176374  UserDialog::AllowChar
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_USERDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_USERDIALOG_H

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "EditListener.h"
#include "LawnDialog.h"
#include "ListListener.h"
#include "ListWidget.h"
#include "Widget.h"

// LawnApp is only the constructor's pointer parameter (`UserDialog(LawnApp *)`),
// so it is declared rather than included: the include was half of the
// LawnApp.h <-> UserDialog.h cycle. Sexy\ReconTypes.h already carries it.
class LawnApp;

class UserDialog : public LawnDialog, public Sexy::ListListener, public Sexy::EditListener
{
public:
    virtual ~UserDialog();
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager); // slot 29
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // slot 30
    virtual void Draw(Sexy::Graphics *theGraphics); // slot 36
    virtual void Resize(int theX, int theY, int theWidth, int theHeight); // slot 50
    virtual int GetPreferredHeight(int theWidth); // slot 125
    virtual void ButtonDepress(int theId); // slot 129
    virtual void ListClicked(int theId, int theIndex, int theClickCount); // ListListener subobject
    virtual void EditWidgetText(int theId, const Sexy::String &theText); // EditListener subobject
    virtual int AllowChar(int theId, char theChar); // EditListener subobject

    // +0x304 .. +0x313. The ListListener base owns +0x2FC and the EditListener
    // base owns +0x300, so this class's own members start at +0x304.
    // TODO(recon): the reference tree names the buttons `Sexy::DialogButton *`;
    // UserDialog.cpp:219/:235 store whatever the button factory returns, so the
    // exact type is unconfirmed.
    Sexy::ListWidget *mUserList; // +0x304
    Sexy::DialogButton *mRenameButton; // +0x308
    Sexy::DialogButton *mDeleteButton; // +0x30C
    int mNumUsers; // +0x310, :402 and :437 bounds-check it
    // size 0x314

    // Phase 2 recovered both signatures from the binary's own disassembly:
    //   GetSelName @0x176978 is an sret function -- r0 is the String it fills
    //     and r1 is the receiver -- so it returns a Sexy::String.
    //   FinishRenameUser @0x176A74 passes its incoming second register on to
    //     ListWidget::SetLine as the new label, so its parameter is the name.
    Sexy::String GetSelName();
    void FinishRenameUser(const Sexy::String &theNewName);
    void FinishDeleteUser();
    UserDialog(LawnApp *theApp);
};

// TODO(recon): the two button ids the framework passes to ButtonDepress --
// UserDialog_RenameUser and UserDialog_DeleteUser -- are still spelled as bare
// 0/1 in the body; the binary gives them no symbol either, so they stay
// literals until the framework's id table is reconstructed.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_USERDIALOG_H

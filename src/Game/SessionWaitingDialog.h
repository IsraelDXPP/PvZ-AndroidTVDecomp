// SessionWaitingDialog -- reconstructed from libGameMain.so.
//
// Real polymorphic class: `class SessionWaitingDialog : public Sexy::Dialog`.
// Evidence: the destructor at _recon\ida\SessionWaitingDialog.cpp:51 calls
// `Sexy::Dialog::~Dialog(this)`, and the RTTI export _ZTV20SessionWaitingDialog
// is Dialog's table with only two entries replaced:
//   [  0] 0x1eae48  SessionWaitingDialog::~SessionWaitingDialog        (D2)
//   [  1] 0x1eaee0  SessionWaitingDialog::~SessionWaitingDialog        (D0)
//   [ 31] 0x1eb018  SessionWaitingDialog::Update                       (override)
//   [ 36] 0x1eba88  SessionWaitingDialog::Draw                         (override)
// Everything else -- including slots 29/30 (AddedToManager/RemovedFromManager)
// and 41..80 -- still points at Dialog's or Widget's code, so those are not
// redeclared here. MSVC x86 gives the virtual destructor ONE slot where Itanium
// gives two, so absolute indices shift by one; only the order matters.
//
// The secondary vptr at +0x100 is Dialog's ButtonListener sub-object, not a
// member of this class: the destructor writes `&off_6862AC` there and the
// -0x100 non-virtual thunk at 0x1EAED8 belongs to it.
//
// Size 0x2EC, from the allocation site
//     LawnApp.cpp:3185   SessionWaitingDialog allocation, 0x2EC bytes
// which is Dialog (0x2E0, see the static_assert in Sexy\Dialog.h) plus three
// words. The old opaque blob said 0x330.
//
// Members, from _recon\ida\SessionWaitingDialog.cpp:
//   ctor:  `*((_DWORD *)this + 184) = v9` where v9 = new SimpleMessageWidget (0x2E0)
//          `*((_DWORD *)this + 185) = Sexy::gEmptyStringData` -- the shared empty string rep (0x2E4)
//          `*((_DWORD *)this + 186) = 0`                                 (0x2E8)
//   dtor:  deletes the widget at 0x2E0, then releases the string rep at 0x2E4
//   Update: calls SimpleMessageWidget::Update on 0x2E0 and decrements 0x2E8
//   Draw:  calls SimpleMessageWidget::Draw on the widget at 0x2E0

#pragma once

#include "ReconTypes.h"
// The waiting dialogs construct a SimpleMessageWidget; include the real class
// so the member pointer resolves.
#include "SimpleMessageWidget.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept so other files' transitive dependencies do not change.
#include "Dialog.h"

class SessionWaitingDialog : public Sexy::Dialog
{
public:
    virtual ~SessionWaitingDialog();

    // Overrides of Dialog/Widget slots.
    virtual void Update();                     // [31] 0x1eb018
    virtual void Draw(Sexy::Graphics *theGraphics);

    SessionWaitingDialog(LawnApp *theApp, int theDialogId);

    void MouseDrag(int theX, int theY);

    SimpleMessageWidget *mMessageWidget; // +0x2E0, owned and deleted by the dtor
    Sexy::String mString;                // +0x2E4, "[CONNECTION_SERVER]" style status text
    int mDelay;                          // +0x2E8, counted down by Update
    // Written when a session task finishes while this wait is up. 0 means the wait ended.
    int mResult;
    // size 0x2EC
};


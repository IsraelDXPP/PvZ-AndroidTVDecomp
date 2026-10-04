// CheatDialog -- reconstructed from libGameMain.so.
//
// Polymorphic class derived from LawnDialog (Game\LawnDialog.h) plus a second
// polymorphic base at +0x2FC. The constructor calls LawnDialog::LawnDialog, then
// stores three vptrs -- this + 0, this + 256 and this + 764 -- and the destructor
// does the same before calling LawnDialog::~LawnDialog. The non-virtual thunk of
// the destructor subtracts 256, confirming the +0x100 subobject, which is
// Sexy::Dialog's Sexy::ButtonListener base. The third one at +0x2FC is the
// ButtonListener that CreateEditWidget is handed:
//     CreateEditWidget(0, pointer at this + 764, this)
// so the class derives from Sexy::ButtonListener as well.
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x1919e0  CheatDialog::~CheatDialog
//   [  1] 0x191a34  CheatDialog::~CheatDialog
//   [ 29] 0x19208c  CheatDialog::AddedToManager
//   [ 30] 0x1920b4  CheatDialog::RemovedFromManager
//   [ 36] 0x19210c  CheatDialog::Draw
//   [ 50] 0x1920cc  CheatDialog::Resize
//   [ 61] 0x192124  CheatDialog::GotFocus
// Slots 2..28, 31..35 and the rest of the Widget / Dialog slots are inherited and
// must NOT be re-declared here.
// Itanium/ARM gives the destructor two slots; MSVC x86 gives it one, so the
// generated absolute slot numbers are one lower than the comments above.
//
// Layout after LawnDialog (0x2FC in the binary) and the ButtonListener subobject:
//     +0x300  LawnApp *mApp          -- `*((_DWORD *)this + 192) = a2`
//     +0x304  Sexy::EditWidget *mEditWidget -- the CreateEditWidget result,
//             stored at ordinal 193
//     +0x308 .. +0x39F  unknown; the object is 0x3A0 in this transcription.
// TODO(recon): none of the words after +0x304 has been named yet. LawnDialog is
// still declared as 0x710 in this tree (another group owns it), so the derived
// fields only land on the binary offsets once that header is rebuilt.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "ButtonListener.h"
#include "Dialog.h"
#include "EditWidget.h"
#include "Font.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"

// LawnApp is used only as `LawnApp *mApp` and as the constructor's pointer
// parameter, so it is declared rather than included. The include closed the
// LawnApp.h <-> CheatDialog.h cycle, which left LawnApp undefined whenever this
// header was entered first.
class LawnApp;

class CheatDialog : public LawnDialog, public Sexy::ButtonListener
{
public:
    virtual ~CheatDialog();

    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    virtual void GotFocus();

    int EditWidgetText(int theId);
    bool AllowChar(int theId, char theChar);
    CheatDialog(LawnApp *theApp);
    int ApplyCheat();
    int GetPreferredHeight(int theWidth);

    LawnApp *mApp;
    Sexy::EditWidget *mEditWidget;
};

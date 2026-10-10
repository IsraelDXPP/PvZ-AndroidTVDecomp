// CheatCodeDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from CheatCodeDialog.h and
// its bases.

#include "CheatCodeDialog.h"

#include "game/Board.h"
#include "Dialog.h"
#include "EditWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "ReanimationWidget.h"
#include "WidgetManager.h"

// The libc ctype table the binary reaches as `ctype_`: one byte per entry,
// indexed by the character value plus one (the body spells the +1 itself).
// ReconData.h declares it and ReconData.cpp defines it, so it must not be
// redeclared here: a file-local `ctype_` is a different type and is C2373.
// TODO(recon): no platform table is wired up, so every class bit reads as 0.

// ==========================================================================
// CheatCodeDialog::EditWidgetText(int,Sexy::String const&)
// @00192154  24 bytes
// ==========================================================================
// The body is nine instructions and uses none of the three arguments: the two
// listener parameters are overwritten and the word it leaves in r0 is the
// receiver, so it returns nothing.
void CheatCodeDialog::EditWidgetText(int theId, const Sexy::String &theText)
{
    (void)theId;
    (void)theText;
    mApp->ButtonDepress(mId + 2000);
}

// ==========================================================================
// CheatCodeDialog::AllowChar(int,char)
// @00192174  24 bytes
// ==========================================================================
// `ctype_[c + 1] & 3` -- libc's table, indexed by (char + 1), with the low two
// bits being the alphanumeric classes. Returns 1 when either is set and 0
// otherwise, which is what the dump's `if (masked != 0) return 1; return masked;`
// amounts to.
int CheatCodeDialog::AllowChar(int theId, char theChar)
{
    (void)theId;
    unsigned char aClass = ctype_[theChar + 1];
    return (aClass & 3) != 0;
}

// ==========================================================================
// CheatCodeDialog::~CheatCodeDialog()
// @0019219C  56 bytes
// ==========================================================================
// Two vptr stores and the explicit `~LawnDialog()` call are the compiler's; what
// is left is the owned EditWidget, released through its deleting destructor
// (vptr byte 4) behind a null test, i.e. `delete`.
CheatCodeDialog::~CheatCodeDialog()
{
    if (mEditWidget != nullptr)
        delete mEditWidget;
}

// ==========================================================================
// CheatCodeDialog::CheatCodeDialog(LawnApp *)
// @0019220C  326 bytes
// ==========================================================================
// The three Sexy::String temporaries the dump builds and then releases by hand
// are the base constructor's header/lines/footer arguments; the two vptr stores
// after them are the compiler's.
//
// `CreateEditWidget(0, (Sexy::EditListener *)((char *)this + 0x2FC), this)`
// hands the framework this object's EditListener sub-object -- which is what the
// second base is for -- and the result is kept in mEditWidget.
CheatCodeDialog::CheatCodeDialog(LawnApp *theApp) :
    LawnDialog(theApp, nullptr, 37, true, "[CHEAT]", "[ENTER_CHEAT_CODE]", "", 2)
{
    mVerticalCenterText = false;
    mApp = theApp;
    mEditWidget = (Sexy::EditWidget *)CreateEditWidget(0, (Sexy::EditListener *)this, this);
    // The binary's `STR.W R6, [R0,#0x158]` with R6 = 12: the field at +0x158 is
    // EditWidget::mMaxChars, the character budget the edit field enforces.
    mEditWidget->mMaxChars = 12;
    mEditWidget->AddWidthCheckFont(FONT_BRIANNETOD12, 220);
    CalcSize(110, 40, true);
}

// ==========================================================================
// CheatCodeDialog::ApplyCheat()
// @001923A4  30 bytes
// ==========================================================================
int CheatCodeDialog::ApplyCheat()
{
    if (mApp->mBoard != nullptr)
        mApp->mBoard->DoTypingCheck(mEditWidget->mString.data());
    return 1;
}

// ==========================================================================
// CheatCodeDialog::GetPreferredHeight(int)
// @001923C4  10 bytes
// ==========================================================================
// A direct call to Dialog's own implementation (0x302ba8), not a virtual one.
int CheatCodeDialog::GetPreferredHeight(int theWidth)
{
    return Sexy::Dialog::GetPreferredHeight(theWidth) + 40;
}

// ==========================================================================
// CheatCodeDialog::AddedToManager(Sexy::WidgetManager *)
// @001923D0  38 bytes
// ==========================================================================
// The two indirect calls are the receiver's own vptr at byte 0x18 (slot 6,
// WidgetContainer::AddWidget) and at byte 0xE4 (slot 57, Widget::SetFocus), each
// with the EditWidget as the argument -- i.e. plain unqualified virtual calls.
void CheatCodeDialog::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::AddedToManager(theWidgetManager);
    AddWidget(mEditWidget);
    SetFocus(mEditWidget, true);
}

// ==========================================================================
// CheatCodeDialog::RemovedFromManager(Sexy::WidgetManager *)
// @001923F8  22 bytes
// ==========================================================================
void CheatCodeDialog::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
    RemoveWidget(mEditWidget);
}

// ==========================================================================
// CheatCodeDialog::Resize(int,int,int,int)
// @00192410  64 bytes
// ==========================================================================
// The EditWidget follows the dialog's content insets: 12 pixels in from the
// left, 155 short of the full width, and the height less both horizontal insets
// and 24 pixels of padding.
void CheatCodeDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    LawnDialog::Resize(theX, theY, theWidth, theHeight);
    mEditWidget->Resize(
        mContentInsets.mLeft + 12, mWidth - 155, mHeight - mContentInsets.mLeft - mContentInsets.mRight - 24, 28);
}

// ==========================================================================
// CheatCodeDialog::Draw(Sexy::Graphics *)
// @00192450  24 bytes
// ==========================================================================
void CheatCodeDialog::Draw(Sexy::Graphics *g)
{
    LawnDialog::Draw(g);
    DrawEditBox(g, mEditWidget);
}

// ==========================================================================
// CheatCodeDialog::GotFocus()
// @00192468  48 bytes
// ==========================================================================
// Focus the edit box unless something else already holds the focus. The dump
// reads mFocusedChildWidget twice -- once before the base call and once after --
// because LawnDialog::GotFocus can move it, so both reads are kept.
void CheatCodeDialog::GotFocus()
{
    Widget *aFocusedBefore = mFocusedChildWidget;
    LawnDialog::GotFocus();

    if (aFocusedBefore == nullptr || mFocusedChildWidget == nullptr || mFocusedChildWidget == mEditWidget)
        SetFocus(mEditWidget, true);
}

// NewUserDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from NewUserDialog.h and its
// bases. There is no PvZ-src counterpart for this file.

#include "NewUserDialog.h"

#include "Dialog.h"
#include "EditWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "ReanimationWidget.h"
#include "WidgetManager.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// The libc ctype table the binary reaches as `ctype_`: one byte per entry,
// indexed by the character value plus one (the body spells the +1 itself).
// ReconData.h declares it and ReconData.cpp defines it, so it must not be
// redeclared here: a file-local `ctype_` is a different type and is C2373.
// TODO(recon): no platform table is wired up, so every class bit reads as 0.

// ==========================================================================
// NewUserDialog::EditWidgetText(int,Sexy::String const&)
// @0014B498  24 bytes
// ==========================================================================
// The receiver is `this`, not a1: the dump's prototype lost the implicit
// parameter, and both EditListener arguments (theId, theText) are overwritten
// before the call, which is why only one shows up. What is left is mApp's
// ButtonDepress -- vtable slot 4 -- called with `mId + 2000`, the framework's
// edit-widget id convention (the dialog's own id plus 2000). The same call
// appears in CheatCodeDialog, CheatDialog and UserDialog.
void NewUserDialog::EditWidgetText(int theId, const Sexy::String &theText)
{
    mApp->ButtonDepress(mId + 2000);
}

// ==========================================================================
// NewUserDialog::AllowChar(int,char)
// @0014B4B8  32 bytes
// ==========================================================================
// `ctype_[c + 1] << 29` keeps only bit 2 of the class byte, which is libc's
// _CTYPE_PUNCT: a user name may contain punctuation but not control characters
// or digits, and a space is allowed explicitly. EditListener declares the slot
// `int`, so this returns one.
int NewUserDialog::AllowChar(int theId, char theChar)
{
    return ((ctype_[theChar + 1] & 4) != 0 || theChar == ' ') ? 1 : 0;
}

// ==========================================================================
// NewUserDialog::~NewUserDialog()
// @0014B4E8  56 bytes
// ==========================================================================
NewUserDialog::~NewUserDialog()
{
    // The edit widget belongs to this dialog; the dump deletes it through its
    // virtual deleting destructor (`(*(vptr + 4))(widget)`), which is what
    // `delete` emits. The three vptr stores are compiler-generated now that the
    // bases are real.
    delete mEditWidget;
}

// ==========================================================================
// NewUserDialog::NewUserDialog(LawnApp *,bool)
// @0014B558  438 bytes
// ==========================================================================
NewUserDialog::NewUserDialog(LawnApp *theApp, bool theIsRename)
    // The dump's placement-new of LawnDialog over `this` and the three string
    // temporaries it was handed (StringCtor is StringCtor) are this initialiser
    // list; the id and the header both depend on `isRename`. The three vptr
    // stores are compiler-generated.
    :
    LawnDialog(theApp, nullptr, theIsRename ? 33 : 31, true, theIsRename ? "[RENAME_USER]" : "[NEW_USER]",
        "[PLEASE_ENTER_NAME]", "", 2)
{
    mVerticalCenterText = false;
    mApp = theApp;

    // CreateEditWidget(int, Sexy::EditListener *, Sexy::Dialog *). The listener
    // argument is the +0x2FC EditListener sub-object, so the pointer adjustment
    // has to be explicit: the declaration is variadic and would otherwise pass
    // `this` unchanged.
    mEditWidget = (Sexy::EditWidget *)CreateEditWidget(0, static_cast<Sexy::EditListener *>(this), this);
    mEditWidget->mMaxChars = 12;
    mEditWidget->AddWidthCheckFont(FONT_BRIANNETOD16, 220);

    // vtable slot 129 (EditWidget::SetText): seed the widget with the empty
    // string, so the caret and the selection both start at zero.
    mEditWidget->SetText(Sexy::String(""), true, true, true);

    CalcSize(110, 40, true);
}

// ==========================================================================
// NewUserDialog::GetName()
// @0014B774  250 bytes
// ==========================================================================
Sexy::String NewUserDialog::GetName()
{
    // The dump's `this` is the result buffer -- the binary returns the String
    // through r0 -- and its `a2` is the real receiver, so the signature returns by
    // value (see the header). The body copies the edit widget's text one character
    // at a time, collapsing every run of spaces into a single one and dropping a
    // run at the front (the loop starts as if the previous character had been a
    // space), then drops one trailing space. The reserve/refcount bookkeeping in
    // the dump (StringReserve, StringDisposeLeaked, StringResize) is the inlined COW append, the
    // LeakBuffer call and the erase, all of which Sexy::String does itself.
    Sexy::String aName;
    bool aLastWasSpace = true;
    for (unsigned int i = 0; i < mEditWidget->mString.size(); i++)
    {
        char aChar = mEditWidget->mString[i];
        if (aChar == ' ')
        {
            if (aLastWasSpace)
                continue;
            aLastWasSpace = true;
        }
        else
        {
            aLastWasSpace = false;
        }
        aName += aChar;
    }

    if (!aName.empty() && aName[aName.size() - 1] == ' ')
        aName = aName.substr(0, aName.size() - 1);

    return aName;
}

// ==========================================================================
// NewUserDialog::SetName(Sexy::String const&)
// @0014B884  52 bytes
// ==========================================================================
int NewUserDialog::SetName(Sexy::String const &theName)
{
    // vtable offset 516 is slot 129, EditWidget::SetText(String const&, bool,
    // bool, bool); the three flags are all set.
    mEditWidget->SetText(theName, true, true, true);
    // The widget's selection follows the text that was just installed: the caret
    // sits at the end of it and nothing is selected.
    mEditWidget->mCursorPos = (int)theName.size();
    mEditWidget->mSelectionPos = 0;
    // The binary returns SetText's result. This tree's EditWidget::SetText is
    // declared void (Sexy\EditWidget.h), so there is nothing to return.
    return 0;
}

// ==========================================================================
// NewUserDialog::GetPreferredHeight(int)
// @0014B8B8  10 bytes
// ==========================================================================
int NewUserDialog::GetPreferredHeight(int theWidth)
{
    // The edit widget needs forty more pixels than the base asks for.
    return Sexy::Dialog::GetPreferredHeight(theWidth) + 40;
}

// ==========================================================================
// NewUserDialog::AddedToManager(Sexy::WidgetManager *)
// @0014B8C4  38 bytes
// ==========================================================================
void NewUserDialog::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::AddedToManager(theWidgetManager);
    AddWidget(mEditWidget);
    SetFocus(mEditWidget, true);
}

// ==========================================================================
// NewUserDialog::RemovedFromManager(Sexy::WidgetManager *)
// @0014B8EC  22 bytes
// ==========================================================================
void NewUserDialog::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
    RemoveWidget(mEditWidget);
}

// ==========================================================================
// NewUserDialog::Resize(int,int,int,int)
// @0014B904  64 bytes
// ==========================================================================
void NewUserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    LawnDialog::Resize(theX, theY, theWidth, theHeight);

    // The edit widget fills the dialog's content area, inset twelve pixels at the
    // top and leaving room for the two buttons at the bottom.
    mEditWidget->Resize(
        mContentInsets.mTop + 12, mHeight - 155, mWidth - mContentInsets.mTop - mContentInsets.mBottom - 24, 28);
}

// ==========================================================================
// NewUserDialog::Draw(Sexy::Graphics *)
// @0014B944  24 bytes
// ==========================================================================
void NewUserDialog::Draw(Sexy::Graphics *theGraphics)
{
    LawnDialog::Draw(theGraphics);
    DrawEditBox(theGraphics, mEditWidget);
}

// ==========================================================================
// NewUserDialog::GotFocus()
// @0014B95C  48 bytes
// ==========================================================================
void NewUserDialog::GotFocus()
{
    LawnDialog::GotFocus();
    // The edit widget takes the focus unless something else already holds it, in
    // which case the base's choice stands. The two paths the ARM takes are
    // "nothing focused yet" and "the edit widget itself is focused".
    if (mFocusedChildWidget == nullptr || mFocusedChildWidget == mEditWidget)
        SetFocus(mEditWidget, true);
}

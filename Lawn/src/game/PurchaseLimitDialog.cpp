// PurchaseLimitDialog -- reconstructed from libGameMain.so.
//
// Stage 2: the bodies are the IDA decompilation rewritten as ordinary C++
// against the reconstructed headers (see build/phase2-g5-brief.md).

#include "PurchaseLimitDialog.h"

#include "Dialog.h"
#include "EditWidget.h"
#include "Font.h"
#include "GameButton.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "ReanimationWidget.h"
#include "SexyCommonApp.h"
#include "WidgetManager.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ==========================================================================
// PurchaseLimitDialog::~PurchaseLimitDialog()
// @001ECA8C  152 bytes
// ==========================================================================
PurchaseLimitDialog::~PurchaseLimitDialog()
{
    // The dump is three vptr stores (this, +0x100 and +0x2FC), the release of
    // the two Strings at +0x310/+0x314 and LawnDialog::~LawnDialog -- all of
    // them compiler-generated now that both bases and both members are real.
    // The only work left is the edit widget, which the dump deletes through its
    // virtual deleting destructor (`(*(void (**)(int))(*(_DWORD *)v2 + 4))(v2)`);
    // that is what `delete` emits.
    //
    // NOTE: the transcription had no definition for this destructor at all --
    // the header declared it and no translation unit defined it, which left the
    // class's vtable with an unresolved entry.
    delete mEditWidget;
}

// ==========================================================================
// PurchaseLimitDialog::PurchaseLimitDialog(LawnApp *,int)
// @001ECB74  626 bytes
// ==========================================================================
PurchaseLimitDialog::PurchaseLimitDialog(LawnApp * theApp, int thePurchaseId)
    : LawnDialog(theApp, nullptr, 83, true, "[PurchaseSecretKey]", "", "", 2)
{
    // The base is a real base now, so the dump's placement-new of LawnDialog
    // over `this` and the three string temporaries it was handed (StringCtor is
    // StringCtor) are the initialiser list above. The vptr stores at +0x00,
    // +0x100 (Dialog's ButtonListener) and +0x2FC (the EditListener base) are
    // compiler-generated too.
    mPurchaseId = thePurchaseId;
    mApp = theApp;
    mKeyFailed = false;

    // +0x310 and +0x314. For each one the dump builds a String with a by-value
    // call whose result buffer is the first argument (ARM sret): the key goes
    // through TodStringTranslate, then through SexyUtf8ToWString, which is what
    // StringToWString is (Sexy\ReconHelpers.h). This tree's TodStringTranslate is
    // the variadic stand-in in Sexy\ReconFreeFunctions.h, so the destination is
    // passed first, as every other call site in the tree does.
    Sexy::String aText;
    TodStringTranslate(&aText, "[PurchaseSecretKey]");
    StringToWString(&mPrompt, aText.c_str());

    TodStringTranslate(&aText, "[PurchaseSecretKey_Failed]");
    StringToWString(&mFailedText, aText.c_str());

    // CreateEditWidget(int, Sexy::EditListener *, Sexy::Dialog *). The listener
    // argument is the +0x2FC sub-object, so the pointer adjustment has to be
    // explicit: the declaration is variadic and would otherwise pass `this`
    // unchanged.
    mEditWidget = (Sexy::EditWidget *)CreateEditWidget(0, static_cast<Sexy::EditListener *>(this), this);

    // The two fields the dialog programs on the widget are the named members
    // Sexy\EditWidget.h recovered at those offsets:
    //     +0x158  mMaxChars, the length cap, 12 characters
    //     +0x160  mPasswordChar, the character echoed instead of the text, '*'
    mEditWidget->mMaxChars = 12;
    mEditWidget->AddWidthCheckFont(FONT_BRIANNETOD12, 220);

    CalcSize(110, 40, true);

    mEditWidget->mPasswordChar = '*';

    // Three focus links, each spelled (self, up, down, left, right): the widget
    // links to the two buttons, and each button links to the widget and the
    // other button. vtable slot 59 is Widget::SetFocusLinks.
    mEditWidget->SetFocusLinks(nullptr, mLawnYesButton, mLawnYesButton, mLawnNoButton);
    mLawnYesButton->SetFocusLinks(mEditWidget, nullptr, nullptr, mLawnNoButton);
    mLawnNoButton->SetFocusLinks(mEditWidget, nullptr, mLawnYesButton, nullptr);

    // vtable slot 50 (the dump calls it through the table; nothing overrides it).
    Resize(0, 0, 506, 362);
}

// ==========================================================================
// PurchaseLimitDialog::isalnum(char)
// @001ECE70  36 bytes
// ==========================================================================
bool PurchaseLimitDialog::isalnum(char theChar)
{
    return (std::uint8_t)(theChar - 48) <= 9u || (std::uint8_t)((theChar & 0xDF) - 65) <= 0x19u || theChar > 0xBF;
}

// ==========================================================================
// PurchaseLimitDialog::AllowChar(int,char)
// @001ECE94  44 bytes
// ==========================================================================
int PurchaseLimitDialog::AllowChar(int theId, char theChar)
{
    // Typing anything clears the "wrong key" flag the dialog draws in red.
    if (mKeyFailed)
        mKeyFailed = false;

    if (theChar == '^')
        return false;

    if (!isalnum(theChar))
        return theChar == ' ';

    return true;
}

// ==========================================================================
// PurchaseLimitDialog::GetName()
// @001ECEC8  250 bytes
// ==========================================================================
Sexy::String PurchaseLimitDialog::GetName()
{
    // The dump's `this` is the result buffer -- the binary returns the String
    // through r0 -- and its `theId` is the real receiver, so the signature returns
    // by value (see the header).
    //
    // The body copies the edit widget's text one character at a time, collapsing
    // every run of spaces into a single one and dropping a run at the front (the
    // loop starts as if the previous character had been a space), then drops one
    // trailing space. The reserve/refcount bookkeeping in the dump (StringReserve,
    // StringDisposeLeaked, StringResize) is the inlined COW append, the LeakBuffer call and
    // the erase, all of which Sexy::String does itself.
    //
    // The text is EditWidget's mString at +0x108, which Sexy\EditWidget.h names.
    const Sexy::String & aText = mEditWidget->mString;

    Sexy::String aName;
    bool aLastWasSpace = true;
    for (unsigned int i = 0; i < aText.size(); i++)
    {
        char aChar = aText[i];
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
// PurchaseLimitDialog::SetKey(Sexy::String const&)
// @001ECFD8  52 bytes
// ==========================================================================
int PurchaseLimitDialog::SetKey(Sexy::String const& theKey)
{
    // vtable offset 516 is slot 129, EditWidget::SetText(String const&, bool,
    // bool, bool); the three flags are all set.
    mEditWidget->SetText(theKey, true, true, true);

    // +0x13C and +0x140 of the widget, the selection it keeps beside the text:
    // EditWidget's mCursorPos and mSelectionPos. The dump stores the text's
    // length in the first and zero in the second.
    mEditWidget->mSelectionPos = 0;
    mEditWidget->mCursorPos = (int)theKey.size();

    // The binary returns SetText's result. This tree's EditWidget::SetText is
    // declared void (Sexy\EditWidget.h), so there is nothing to return.
    return 0;
}

// ==========================================================================
// PurchaseLimitDialog::AddedToManager(Sexy::WidgetManager *)
// @001ED00C  72 bytes
// ==========================================================================
void PurchaseLimitDialog::AddedToManager(Sexy::WidgetManager * theWidgetManager)
{
    LawnDialog::AddedToManager(theWidgetManager);

    // Three virtual calls: slot 50 Widget::Resize, slot 6
    // WidgetContainer::AddWidget, slot 57 Widget::SetFocus(Widget *, bool).
    mEditWidget->Resize((mWidth - 200) / 2, 120, 200, 40);
    AddWidget(mEditWidget);
    SetFocus(mEditWidget, true);
}

// ==========================================================================
// PurchaseLimitDialog::RemovedFromManager(Sexy::WidgetManager *)
// @001ED054  22 bytes
// ==========================================================================
void PurchaseLimitDialog::RemovedFromManager(Sexy::WidgetManager * theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);

    // vtable slot 7 of WidgetContainer is RemoveWidget(Widget *).
    RemoveWidget(mEditWidget);
}

// ==========================================================================
// PurchaseLimitDialog::Draw(Sexy::Graphics *)
// @001ED06C  126 bytes
// ==========================================================================
void PurchaseLimitDialog::Draw(Sexy::Graphics * theGraphics)
{
    LawnDialog::Draw(theGraphics);

    theGraphics->SetColor(Sexy::Color(255, 238, 51));
    if (mKeyFailed)
    {
        theGraphics->SetColor(Sexy::Color(255, 0, 0));
        theGraphics->SetFont(FONT_DWARVENTODCRAFT24);

        // The width comes from Font slot 9, StringWidthUnicode(std::wstring
        // const&). The binary's wide strings are pvzstl::wstring, the one-word
        // wide string of this 32-bit libstdc++ ABI, which is the type mFailedText
        // holds here (see the note in Sexy\Font.h); only the font reads it.
        int aWidth = mWidth;
        int aTextWidth = (FONT_DWARVENTODCRAFT24)->StringWidthUnicode(*(const std::wstring *)&mFailedText);
        theGraphics->DrawString(mFailedText, (aWidth - aTextWidth) / 2, 200);
    }

    DrawEditBox(theGraphics, mEditWidget);
}

// ==========================================================================
// PurchaseLimitDialog::KeyDown(Sexy::KeyCode)
// @001ED0F4  12 bytes
// ==========================================================================
int PurchaseLimitDialog::KeyDown(Sexy::KeyCode theKey)
{
    // Escape is swallowed here -- the dump returns 1 -- instead of reaching the
    // base.
    if (theKey == Sexy::KEYCODE_ESCAPE)
        return 1;

    return LawnDialog::KeyDown(theKey);
}

// ==========================================================================
// PurchaseLimitDialog::KeyUp(Sexy::KeyCode)
// @001ED100  72 bytes
// ==========================================================================
int PurchaseLimitDialog::KeyUp(Sexy::KeyCode theKey)
{
    if (theKey == Sexy::KEYCODE_RETURN)
    {
        // The dump tests the edit widget's focus flag (Widget::mHasFocus at
        // +0x77): Return only submits once the box has the focus.
        if (!mEditWidget->mHasFocus)
            return Sexy::Dialog::KeyUp(theKey);

        // vtable offset 516 is slot 129, this class's own ButtonDepress, and the
        // argument is the yes button's id. The dump returns the focus byte, 1.
        ButtonDepress(mLawnYesButton->mWidgetId);
        return 1;
    }

    if (theKey == Sexy::KEYCODE_ESCAPE)
    {
        // vtable slot 107 of LawnApp is KillDialog(int); 83 is the id the base
        // constructor was given.
        mApp->KillDialog(83);
        return 1;
    }

    return Sexy::Dialog::KeyUp(theKey);
}

// ==========================================================================
// PurchaseLimitDialog::ButtonDepress(int)
// @001ED148  338 bytes
// ==========================================================================
void PurchaseLimitDialog::ButtonDepress(int theId)
{
    if (theId != mLawnYesButton->mWidgetId)
    {
        LawnDialog::ButtonDepress(theId);
        return;
    }

    Sexy::String aName = GetName();

    Sexy::String aSecretKey = mApp->getSecretKey();

    if (aName == aSecretKey)
    {
        mApp->BuyMoney(mPurchaseId);
        mApp->KillDialog(83);
    }
    else
    {
        mKeyFailed = true;
        SetKey("");
    }
}

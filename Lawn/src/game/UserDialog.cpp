// UserDialog -- transcribed from libGameMain.so
// Bodies restored against the class header; the mechanical transcription has
// been replaced by real member access and real virtual calls.

#include "UserDialog.h"

#include "Dialog.h"
#include "DefaultProfileMgr.h"
#include "DialogButton.h"
#include "Font.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "ListWidget.h"
#include "ReanimationWidget.h"
#include "Widget.h"
#include "WidgetManager.h"
#include <list>
// ==========================================================================
// UserDialog::EditWidgetText(int,Sexy::String const&)
// @00176354  24 bytes
// ==========================================================================
void UserDialog::EditWidgetText(int theId, const Sexy::String &theText)
{
    (void)theId;
    (void)theText;
    // The binary ignores both listener parameters and forwards the dialog's own
    // id, offset by the widget range the framework reserves.
    mApp->ButtonDepress(Sexy::Dialog::mId + 2000);
}

// ==========================================================================
// UserDialog::AllowChar(int,char)
// @00176374  24 bytes
// ==========================================================================
int UserDialog::AllowChar(int theId, char theChar)
{
    (void)theId;
    // Letters, digits and punctuation. Control characters and space are rejected.
    unsigned char aClass = ctype_[theChar + 1];
    return (aClass & 7) != 0 ? 1 : 0;
}

// ==========================================================================
// UserDialog::~UserDialog()
// @0017639C  88 bytes
// ==========================================================================
UserDialog::~UserDialog()
{
    // The binary writes this class' primary vtable and the two listener
    // sub-object vtables into the object by hand, then releases the list widget
    // and the two buttons through their deleting destructors. The vptr stores are
    // the compiler's job; slot 1 of each object's table, which the binary calls,
    // is what `delete` reaches.
    delete mUserList;
    delete mRenameButton;
    delete mDeleteButton;
}

// ==========================================================================
// UserDialog::ListClicked(int,int,int)
// @0017642C  60 bytes
// ==========================================================================
void UserDialog::ListClicked(int theId, int theIndex, int theClickCount)
{
    (void)theId;
    if (mNumUsers == theIndex)
    {
        // The row just past the last user is the "create a new user" line.
        mApp->DoCreateUserDialog();
        return;
    }
    mUserList->SetSelect(theIndex);
    if (theClickCount == 2)
        mApp->FinishUserDialog(true);
}

// ==========================================================================
// UserDialog::UserDialog(LawnApp *)
// @00176470  1242 bytes
// ==========================================================================
UserDialog::UserDialog(LawnApp *theApp) :
    LawnDialog(theApp, nullptr, 30, 1, Sexy::String("[WHO_ARE_YOU]"), Sexy::String(""), Sexy::String(""), 2)
{
    mVerticalCenterText = false;
    mUserList = new Sexy::ListWidget(0, FONT_BRIANNETOD16, this);
    mUserList->SetColors(gUserListWidgetColors, 5);
    mUserList->mItemHeight = 24;
    mUserList->mDrawOutline = true;
    mUserList->mJustify = 1;

    Sexy::String aButtonText;
    TodStringTranslate(&aButtonText, "[RENAME_BUTTON]");
    mRenameButton = MakeButton(0, (Sexy::ButtonListener *)this, this, aButtonText);
    TodStringTranslate(&aButtonText, "[DELETE_BUTTON]");
    mDeleteButton = MakeButton(1, (Sexy::ButtonListener *)this, this, aButtonText);

    mNumUsers = theApp->mProfileMgr->GetNumProfiles();
    LawnPlayerInfo *aCurrent = theApp->mPlayerInfo;
    if (aCurrent != nullptr)
    {
        // Select the line that was just added for the player in use.
        int aSelected = mUserList->AddLine(aCurrent->GetName(), false);
        mUserList->SetSelect(aSelected);
    }
    // Every other saved profile gets its own line. The current player is already
    // the selected row, so a matching name is skipped.
    std::list<Sexy::PlayerInfo *> aProfiles = theApp->mProfileMgr->GetProfileList();
    for (std::list<Sexy::PlayerInfo *>::iterator aProfile = aProfiles.begin(); aProfile != aProfiles.end(); ++aProfile)
    {
        if (*aProfile == nullptr)
            continue;
        Sexy::String aName = (*aProfile)->GetName();
        if (aCurrent != nullptr && aName == aCurrent->GetName())
            continue;
        mUserList->AddLine(aName, false);
    }
    if (mNumUsers <= 7)
    {
        // Room for one more user: append the "create a new user" line.
        Sexy::String aCreateNewUser;
        TodStringTranslate(&aCreateNewUser, "[CREATE_NEW_USER]");
        mUserList->AddLine(aCreateNewUser, false);
    }
    mTallBottom = true;
    CalcSize(260, 320, true);
}

// ==========================================================================
// UserDialog::GetSelName(void)
// @00176978  60 bytes
// ==========================================================================
Sexy::String UserDialog::GetSelName()
{
    int aSelected = mUserList->mSelectIdx;

    if (aSelected >= 0 && aSelected < mNumUsers)
        return mUserList->GetStringAt(aSelected);
    return Sexy::String();
}

// ==========================================================================
// UserDialog::FinishDeleteUser(void)
// @001769B8  162 bytes
// ==========================================================================
void UserDialog::FinishDeleteUser()
{
    int aSelected = mUserList->mSelectIdx;

    mUserList->RemoveLine(aSelected);
    if (mUserList->GetLineCount() > 1)
    {
        int aNext = aSelected - 1;
        if (aNext < 0)
            aNext = 0;
        mUserList->SetSelect(aNext);
    }
    mNumUsers = mNumUsers - 1;
    if (mNumUsers == 7)
    {
        // One user fewer means room for the "create a new user" line again.
        Sexy::String aCreateNewUser;
        TodStringTranslate(&aCreateNewUser, "[CREATE_NEW_USER]");
        mUserList->AddLine(aCreateNewUser, false);
    }
}

// ==========================================================================
// UserDialog::FinishRenameUser(Sexy::String const&)
// @00176A74  32 bytes
// ==========================================================================
void UserDialog::FinishRenameUser(const Sexy::String &theNewName)
{
    int aSelected = mUserList->mSelectIdx;

    if (aSelected < mNumUsers)
        mUserList->SetLine(aSelected, theNewName);
}

// ==========================================================================
// UserDialog::Resize(int,int,int,int)
// @00176A94  126 bytes
// ==========================================================================
void UserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    LawnDialog::Resize(theX, theY, theWidth, theHeight);
    int aLeft = GetLeft();
    int aTop = GetTop();
    int aWidth = GetWidth();
    mUserList->Resize(aLeft + 30, aTop + 4, aWidth - 60, 200);
    mRenameButton->Layout(4355, mLawnYesButton, 0, 0, 0, 0);
    mDeleteButton->Layout(4355, mLawnNoButton, 0, 0, 0, 0);
}

// ==========================================================================
// UserDialog::GetPreferredHeight(int)
// @00176B14  10 bytes
// ==========================================================================
int UserDialog::GetPreferredHeight(int theWidth)
{
    return Sexy::Dialog::GetPreferredHeight(theWidth) + 190;
}

// ==========================================================================
// UserDialog::AddedToManager(Sexy::WidgetManager *)
// @00176B20  46 bytes
// ==========================================================================
void UserDialog::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::AddedToManager(theWidgetManager);
    AddWidget(mUserList);
    AddWidget(mDeleteButton);
    AddWidget(mRenameButton);
}

// ==========================================================================
// UserDialog::RemovedFromManager(Sexy::WidgetManager *)
// @00176B50  46 bytes
// ==========================================================================
void UserDialog::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
    RemoveWidget(mUserList);
    RemoveWidget(mDeleteButton);
    RemoveWidget(mRenameButton);
}

// ==========================================================================
// UserDialog::Draw(Sexy::Graphics *)
// @00176B80  4 bytes
// ==========================================================================
void UserDialog::Draw(Sexy::Graphics *theGraphics)
{
    LawnDialog::Draw(theGraphics);
}

// ==========================================================================
// UserDialog::ButtonDepress(int)
// @00176B84  150 bytes
// ==========================================================================
void UserDialog::ButtonDepress(int theId)
{
    LawnDialog::ButtonDepress(theId);
    // Both buttons hand the selected user's name to the app; with no user
    // selected (an empty name) the press is ignored.
    Sexy::String aName = GetSelName();
    if (aName.size() == 0)
        return;
    if (theId == 0)
        mApp->DoRenameUserDialog(aName);
    else if (theId == 1)
        mApp->DoConfirmDeleteUserDialog(aName);
}

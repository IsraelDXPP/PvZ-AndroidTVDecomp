// RevivalGameDialog -- transcribed from libGameMain.so

#include "RevivalGameDialog.h"

#include "game/Board.h"
#include "Dialog.h"
#include "GameButton.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "ReconHelpers.h"
#include "WidgetManager.h"
#include "Zombie.h"

// ==========================================================================
// RevivalGameDialog::~RevivalGameDialog()
// @0013E620  34 bytes
//
// The binary stores the two vptrs and calls LawnDialog::~LawnDialog, which is
// exactly what the compiler emits for this empty body.
// ==========================================================================
RevivalGameDialog::~RevivalGameDialog()
{
}

// ==========================================================================
// RevivalGameDialog::GotFocus()
// @0013EA20  34 bytes
// ==========================================================================
void RevivalGameDialog::GotFocus()
{
  LawnDialog::GotFocus();
  // +0xDC is Widget::mFocusedChildWidget; the dialog only steals the focus when
  // nothing inside it has it yet.
  if ( mFocusedChildWidget == nullptr )
    SetFocus(mLawnYesButton, true);
}

// ==========================================================================
// RevivalGameDialog::MouseDrag(int,int)
// @0013EAB8  4 bytes
//
// A thunk: the override exists only to keep Sexy::Dialog's behaviour, and the
// binary forwards straight to it.
// ==========================================================================
void RevivalGameDialog::MouseDrag(int theX, int theY)
{
  Sexy::Dialog::MouseDrag(theX, theY);
}

// ==========================================================================
// RevivalGameDialog::ButtonDepress(int)
// @0013EC5C  104 bytes
//
// 1001 is the "no" button, 1000 the "yes" one: buying the revival packet
// restores the lawn mowers and takes the zombie away, and both paths close the
// dialog.
// ==========================================================================
void RevivalGameDialog::ButtonDepress(int theId)
{
  if ( theId == 1001 )
  {
    mApp->KillDialogById(17);
    mApp->mBoard->ZombiesWon(mZombie);
  }
  else if ( theId == 1000 )
  {
    // 6 is E_SHOP_PACKET's revival entry, which this tree has no enumerator for.
    if ( mApp->GotoBuyTool((E_SHOP_PACKET)6) != 0 )
    {
      mApp->mBoard->ResetLawnMowers();
      mZombie->DieNoLoot();
      mApp->KillDialogById(17);
    }
  }
}

// ==========================================================================
// RevivalGameDialog::RevivalGameDialog(Zombie *,int)
// @001408A8  598 bytes
//
// The binary builds the header, line and footer strings from the three literals
// on the stack before the call, which is what passing them straight to the base
// constructor does here.
// ==========================================================================
RevivalGameDialog::RevivalGameDialog(Zombie *theZombie, int theZombieType)
  : LawnDialog(gLawnApp, nullptr, 17, true, "[REVIVAL_GAME_TITLE]", "[REVIVAL_GAME_MSG]", "", 1)
  , mZombie(theZombie)
{
  Sexy::String aFormatted;
  StrFormat(&aFormatted, mDialogLines.c_str(), theZombieType);
  mDialogLines = aFormatted;
  mLawnYesButton->SetLabel("[REVIVAL_GAME_YES_BUTTON]");
  mLawnNoButton->SetLabel("[REVIVAL_GAME_NO_BUTTON]");
  CalcSize(300, 30, true);
  Resize((800 - mWidth) / 2, (600 - mHeight) / 2, mWidth, mHeight);
  mClip = false;
  if ( mApp->mBoard != nullptr )
  {
    mApp->mBoard->mShowShovel = false;
    mApp->mBoard->mShowButter = false;
    mApp->mBoard->mShowHammer = false;
  }
}

// ==========================================================================
// RevivalGameDialog::AddedToManager(Sexy::WidgetManager *)
// @00141154  4 bytes
//
// A thunk to LawnDialog's own slot.
// ==========================================================================
void RevivalGameDialog::AddedToManager(Sexy::WidgetManager *theManager)
{
  LawnDialog::AddedToManager(theManager);
}

// ==========================================================================
// RevivalGameDialog::RemovedFromManager(Sexy::WidgetManager *)
// @001412AC  4 bytes
//
// A thunk to LawnDialog's own slot.
// ==========================================================================
void RevivalGameDialog::RemovedFromManager(Sexy::WidgetManager *theManager)
{
  LawnDialog::RemovedFromManager(theManager);
}

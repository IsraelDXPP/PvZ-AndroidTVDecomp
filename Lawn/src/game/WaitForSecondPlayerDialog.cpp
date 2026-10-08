// WaitForSecondPlayerDialog -- reconstructed from libGameMain.so
//
// The 2-player join prompt. It is a LawnDialog with one extra byte of state:
// whether the second player has been accepted yet. KeyDown hands the pending
// input event to the app's gamepad layer and, if it maps to a button, forwards
// it to this class's own GameButtonDown; GameButtonDown (button A) binds the
// gamepad the second player pressed it on.
//
// The only state past LawnDialog is m2PJoined, set when the second pad joins.

#include "WaitForSecondPlayerDialog.h"

#include "game/Board.h"
#include "Dialog.h"
#include "GamepadApp.h"
#include "GamepadControls.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "WidgetManager.h"

// ==========================================================================
// WaitForSecondPlayerDialog::~WaitForSecondPlayerDialog()
// @0013E4DC  34 bytes
// ==========================================================================
WaitForSecondPlayerDialog::~WaitForSecondPlayerDialog()
{
  // The binary's body re-points both vptrs and calls the base destructor; with
  // real inheritance the compiler emits all three.
}

void WaitForSecondPlayerDialog::MouseDown(int x, int y, int theClickCount)
{
  if (theClickCount <= 0 || theClickCount == 3 || mApp->mBoard == nullptr)
    return;
  const int firstPlayer = mApp->mPlayerInfo->GetId();
  const int secondPlayer = firstPlayer == 0 ? 1 : 0;
  GameButtonDown(GAMEPAD_BUTTON_A, secondPlayer, 0);
}

// ==========================================================================
// WaitForSecondPlayerDialog::KeyDown(Sexy::KeyCode)
// @0013EDF4  100 bytes
// ==========================================================================
int WaitForSecondPlayerDialog::KeyDown(Sexy::KeyCode theKey)
{
  int aButton = 0;
  int aButtonId = 0;
  unsigned int aFlags = 0;

  // A pending event goes through the app's gamepad layer first (LawnApp IS a
  // Sexy::GamepadApp); when it maps to a button, this dialog consumes it.
  const Sexy::Event *anEvent = mEvent;
  if ( anEvent != nullptr && mApp->MapToButtonEvent(anEvent, aButton, aButtonId, aFlags) != 0 )
  {
    // Button B is swallowed; anything else is the dialog's own business.
    if ( aButton == GAMEPAD_BUTTON_B )
      return 0;
    GameButtonDown((Sexy::GamepadButton)aButton, aButtonId, aFlags);
    return 1;
  }

  if ( theKey == Sexy::KEYCODE_ESCAPE )
  {
    Sexy::Dialog::ButtonDepress(1001);
    return 1;
  }
  return LawnDialog::KeyDown(theKey);
}

// ==========================================================================
// WaitForSecondPlayerDialog::GameButtonDown(Sexy::GamepadButton,int,unsigned int)
// @0013F2A8  226 bytes
// ==========================================================================
void WaitForSecondPlayerDialog::GameButtonDown(Sexy::GamepadButton theButton, int theGamepadIndex, unsigned int theFlags)
{
  (void)theFlags;
  // Only button A joins a player.
  if ( theButton != GAMEPAD_BUTTON_A )
    return;

  if ( theGamepadIndex == mApp->mPlayerInfo->GetId() || mApp->mBoard == nullptr )
  {
    // The pad already belongs to player one (or there is no board to join): this
    // press is the "back out" path, and only does something once joined.
    if ( !m2PJoined )
      return;
    ButtonDepress(1000);
    return;
  }

  if ( m2PJoined )
  {
    // Already joined, so the same press means "back out".
    ButtonDepress(1000);
    return;
  }

  mApp->SetSecondPlayer(theGamepadIndex);
  // Make sure the keyboard/gamepad focus is on this dialog before it takes the
  // new pad.
  if ( mApp->mWidgetManager->mFocusWidget != this )
    mApp->mWidgetManager->SetFocus(this);
  mApp->mBoard->mGamepadControls[1]->mGamepadIndex = theGamepadIndex;

  // Announce the join in the dialog's line text. The dump builds the string,
  // assigns it and releases the temporary by hand; Sexy::String does that.
  Sexy::String aJoined;
  TodStringTranslate(&aJoined, "[2P_JOINED]");
  mDialogLines = aJoined;

  m2PJoined = true;
}

// ==========================================================================
// WaitForSecondPlayerDialog::WaitForSecondPlayerDialog(LawnApp *)
// @001400F4  276 bytes
// ==========================================================================
WaitForSecondPlayerDialog::WaitForSecondPlayerDialog(LawnApp *theApp)
  : LawnDialog(theApp, nullptr, 73, true,
               Sexy::String("[2P_JOIN]"), Sexy::String("[2P_JOIN_TEXT]"), Sexy::String(""), 0)
{
  // The dump's placement-new onto `this` is the base constructor, which the
  // mem-initializer list above performs.
  mApp->ClearSecondPlayer();
  m2PJoined = false;
}

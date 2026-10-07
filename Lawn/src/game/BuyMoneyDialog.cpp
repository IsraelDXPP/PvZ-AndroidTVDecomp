// BuyMoneyDialog -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from BuyMoneyDialog.h and its
// bases. There is no PvZ-src counterpart for this file.
//
// Two things the transcription did not express:
//   * the three strings the constructor handed the base are StringCtor
//     (StringCtor) calls, which the base's initialiser list replaces, and the
//     three release blocks after it were those temporaries' destructors;
//   * the loop over the money tiers is LawnUser::getCurUser(), whose
//     function-local static guard, its initialiser and its __cxa_atexit
//     registration were all inlined into this body by the compiler.
// The dialog's own member table is not in the authoritative tree, so the
// members below +0x2FC keep the header's placeholder names; the seven of them
// the constructor actually stores are pinned by the ARM at 0x2c3c62 and
// 0x2c3c86..0x2c3cb8.

#include "BuyMoneyDialog.h"
#include "Dialog.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "LawnStoneButton.h"
#include "LawnUser.h"
#include "Platform/LocalGameCenter.h"
#include "ReanimationWidget.h"
#include "WidgetManager.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ==========================================================================
// BuyMoneyDialog::GotGameFocus()
// @002C3A3C  2 bytes
// ==========================================================================
void BuyMoneyDialog::GotGameFocus()
{
}

// ==========================================================================
// BuyMoneyDialog::GetPreferredHeight(int)
// @002C3BA4  4 bytes
// ==========================================================================
int BuyMoneyDialog::GetPreferredHeight(int theWidth)
{
  // A four-byte thunk: the ARM is a single tail branch to the base, so this
  // override exists only to keep the slot and changes nothing the base returns.
  return Sexy::Dialog::GetPreferredHeight(theWidth);
}

// ==========================================================================
// BuyMoneyDialog::Resize(int,int,int,int)
// @002C3BA8  4 bytes
// ==========================================================================
void BuyMoneyDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
  LawnDialog::Resize(theX, theY, theWidth, theHeight);
}

// ==========================================================================
// BuyMoneyDialog::BuyMoneyDialog(LawnApp *,Sexy::String const&)
// @002C3BAC  1604 bytes
// ==========================================================================
BuyMoneyDialog::BuyMoneyDialog(LawnApp *theApp, Sexy::String const& theTierName)
    // The dump's placement-new of LawnDialog over `this` and the three string
    // temporaries it was handed (StringCtor is StringCtor) are this initialiser
    // list; the three release blocks that follow it were those temporaries'
    // destructors. The two vptr stores are compiler-generated.
    : LawnDialog(theApp, nullptr, 82, true, "[MONEY_NOT_ENOUGH_HEADER]", "", "", 3)
{
  bool aLocalRecharge = dynamic_cast<Sexy::LocalGameCenter*>(theApp->mGameCenter) != nullptr;
  // The tier vector at +0x2FC is cleared by its own constructor, and +0x308
  // with it; the three layout words are the dialog's own constants.
  mBoughtTier = 0;
  if ( !theTierName.empty() )
  {
    // 0x2C3FD4 assigns the translated title to Dialog's header at +0x118.
    Sexy::String aText;
    TodStringTranslate(&aText, theTierName);
    mDialogHeader = aText;
  }
  if ( aLocalRecharge )
    mDialogHeader = u8"本地测试充值";
  // 506 is the width the dialog resizes itself to, 35 what one tier row
  // contributes to the height, and 120 the first row's Y.
  mDialogWidth = 506;
  mRowPitch = 35;
  mFirstRowY = 120;

  // The money tiers are the current user's coin list. Each element is two
  // words: the coin amount, and the price the format reads as a float.
  LawnUser* aUser = LawnUser::getCurUser();
  const SCoinItem* aTiers = aUser->mCoinItems.data();
  int aTierCount = aUser->CoinTierCount();

  int aY = 0;
  for ( int i = 0; i < aTierCount; i++ )
  {
    // MakeButton(int, Sexy::ButtonListener *, Sexy::Widget *, const Sexy::String &)
    // returns the GameButton it allocated; this dialog keeps them in its own
    // LawnStoneButton vector.
    Sexy::String aLabel;
    if ( aLocalRecharge )
      StrFormat(&aLabel, u8"增加 %d V币", aTiers[i].mGetCoin);
    else
    {
      Sexy::String aFormat = TodStringTranslate("[BUY_MONEY_FORMAT]");
      StrFormat(&aLabel, aFormat.c_str(), aTiers[i].mGetCoin, (float)aTiers[i].mValue);
    }
    LawnStoneButton *aButton = MakeButton(
        i, static_cast<Sexy::ButtonListener *>(this), this, aLabel);

    aButton->Resize(130, mFirstRowY + aY, 200, 25);
    mTierButtons.push_back(aButton);
    aY += 45;
  }

  // The tiers are chained front to back, and every link is the target's
  // `mFocusLinks[0]` (vtable slot 59, Widget::SetFocusLinks). Each chain ends
  // at the dialog's own yes button (the ARM reads +0x2F0 at 0x2c3ec4/0x2c3ee4/
  // 0x2c3f3a), and the last tier's backward link is the word before it in the
  // vector -- for two tiers that is tier 0.
  if ( mTierButtons.size() != 0 )
  {
    if ( mTierButtons.size() == 1 )
    {
      mTierButtons[0]->SetFocusLinks(nullptr, mLawnYesButton, nullptr, nullptr);
    }
    else
    {
      mTierButtons[0]->SetFocusLinks(nullptr, mTierButtons[1], nullptr, nullptr);
      mTierButtons[mTierButtons.size() - 1]->SetFocusLinks(
          mTierButtons[mTierButtons.size() - 2], mLawnYesButton, nullptr, nullptr);
      for ( unsigned int i = 1; i + 1 < mTierButtons.size(); i++ )
        mTierButtons[i]->SetFocusLinks(mTierButtons[i - 1], mTierButtons[i + 1],
                                       nullptr, nullptr);
    }
    mLawnYesButton->SetFocusLinks(mTierButtons[mTierButtons.size() - 1],
                                  nullptr, nullptr, nullptr);
  }

  mLawnYesButton->SetLabel("[DIALOG_BUTTON_CANCEL]");

  // The dialog is sized to the rows it ended up with: six fit per screenful of
  // mRowPitch pixels, plus 254 for the frame.
  unsigned int aRows = mRowPitch * mTierButtons.size() / 0x36;
  if ( (mRowPitch * mTierButtons.size()) % 0x36 != 0 )
    aRows++;
  Resize(0, 0, mDialogWidth, 54 * aRows + 254);
}

// ==========================================================================
// BuyMoneyDialog::AddedToManager(Sexy::WidgetManager *)
// @002C424C  60 bytes
// ==========================================================================
void BuyMoneyDialog::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
  LawnDialog::AddedToManager(theWidgetManager);
  // The tier buttons this dialog created in its constructor, in order. The
  // dump walks the vector's three words by hand because the loop's count is
  // re-read after every call.
  for ( unsigned int i = 0; i < mTierButtons.size(); i++ )
    AddWidget(mTierButtons[i]);
}

// ==========================================================================
// BuyMoneyDialog::RemovedFromManager(Sexy::WidgetManager *)
// @002C4288  60 bytes
// ==========================================================================
void BuyMoneyDialog::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
  LawnDialog::RemovedFromManager(theWidgetManager);
  for ( unsigned int i = 0; i < mTierButtons.size(); i++ )
    RemoveWidget(mTierButtons[i]);
}

// ==========================================================================
// BuyMoneyDialog::Draw(Sexy::Graphics *)
// @002C42C4  4 bytes
// ==========================================================================
void BuyMoneyDialog::Draw(Sexy::Graphics *theGraphics)
{
  LawnDialog::Draw(theGraphics);
}

// ==========================================================================
// BuyMoneyDialog::ButtonDepress(int)
// @002C42C8  58 bytes
// ==========================================================================
void BuyMoneyDialog::ButtonDepress(int theId)
{
  // Only a tier button buys money: 1000 and 1001 are the base dialog's own
  // buttons, and anything outside the tier vector is ignored.
  if ( (unsigned int)(theId - 1000) > 1 && theId < (int)mTierButtons.size() && theId >= 0 )
  {
    // The tier the player picked, which FinishBuyMoneyDialog reads back.
    mBoughtTier = theId;
    mApp->BuyMoney(theId);
    return;
  }
  LawnDialog::ButtonDepress(theId);
}

// ==========================================================================
// BuyMoneyDialog::KeyDown(Sexy::KeyCode)
// @002C430C  4 bytes
// ==========================================================================
int BuyMoneyDialog::KeyDown(Sexy::KeyCode theKey)
{
  return LawnDialog::KeyDown(theKey);
}

// ==========================================================================
// BuyMoneyDialog::KeyUp(Sexy::KeyCode)
// @002C4310  30 bytes
// ==========================================================================
int BuyMoneyDialog::KeyUp(Sexy::KeyCode theKey)
{
  if ( theKey != Sexy::KEYCODE_ESCAPE )
    return Sexy::Dialog::KeyUp(theKey);
  // Escape is the dialog's own no button (id 1001), so the base's ButtonDepress
  // (slot 129) is called directly and this class's override is not re-entered.
  Sexy::Dialog::ButtonDepress(1001);
  return 1;
}

// ==========================================================================
// BuyMoneyDialog::GotFocus()
// @002C4330  64 bytes
// ==========================================================================
void BuyMoneyDialog::GotFocus()
{
  // Read before the base runs, because the base is what may set it.
  Sexy::Widget *aFocusedChild = mFocusedChildWidget;
  LawnDialog::GotFocus();
  if ( aFocusedChild != nullptr )
    return;
  // Nothing in this container holds the focus yet: hand it to the first tier,
  // or to the dialog's own yes button when there are no tiers (Widget::SetFocus,
  // slot 57).
  if ( mTierButtons.size() != 0 )
    SetFocus(mTierButtons[0], true);
  else
    SetFocus(mLawnYesButton, true);
}

// ==========================================================================
// BuyMoneyDialog::~BuyMoneyDialog()
// @002C3A40  116 bytes
// ==========================================================================
BuyMoneyDialog::~BuyMoneyDialog()
{
  // The dump's two opening vptr stores are the compiler's. Its tail -- the
  // `_M_finish = _M_start` store and the `operator delete(_M_start)` that
  // follows -- is libstdcxx::vector<LawnStoneButton *>'s own destructor, which
  // the compiler inlines here and which runs on its own, so neither is written.
  //
  // What is left is the walk over [mTierButtons._M_start, _M_finish): each
  // non-null element is dispatched through vtable byte 4, which is what
  // `delete` compiles to.
  for ( size_t i = 0; i < mTierButtons.size(); ++i )
    delete mTierButtons[i];
}

// SavingProfileDialog -- reconstructed from libGameMain.so
//
// The "saving profile" dingus: a SavingDingusDialog whose Draw optionally lays
// the app's dialog fade over the screen and whose GetStatus reports the dialog
// as finished immediately (>= 1000 is what SavingDingusDialog::Update waits
// for). It adds no data members of its own; the two words the bodies touch,
// +0x2E4 and +0x2E8, are SavingDingusDialog::mApp and ::mDrawFade.
//
// Both AddedToManager and RemovedFromManager are pass-through overrides: the
// binary tail-branches straight into Sexy::WidgetContainer's implementation, so
// they are spelled as qualified base calls rather than virtual ones (a plain
// call would dispatch back into this class).

#include "SavingProfileDialog.h"

#include "Dialog.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "SavingDingusDialog.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"

// ==========================================================================
// SavingProfileDialog::GetStatus()
// @0013E3E4  6 bytes
// ==========================================================================
int SavingProfileDialog::GetStatus()
{
  // 1000 is the "finished" threshold SavingDingusDialog::Update tests.
  return 1000;
}

// ==========================================================================
// SavingProfileDialog::~SavingProfileDialog()
// @0013E764  34 bytes
// ==========================================================================
SavingProfileDialog::~SavingProfileDialog()
{
  // The binary's body re-points the two vptrs and calls the base destructor;
  // with real inheritance the compiler emits all three.
}

// ==========================================================================
// SavingProfileDialog::Draw(Sexy::Graphics *)
// @0013E998  16 bytes
// ==========================================================================
void SavingProfileDialog::Draw(Sexy::Graphics *theGraphics)
{
  if ( mDrawFade )
    mApp->DrawDialogFade(theGraphics);
}

// ==========================================================================
// SavingProfileDialog::AddedToManager(Sexy::WidgetManager *)
// @0013EB34  4 bytes
// ==========================================================================
void SavingProfileDialog::AddedToManager(Sexy::WidgetManager *theManager)
{
  Sexy::WidgetContainer::AddedToManager(theManager);
}

// ==========================================================================
// SavingProfileDialog::RemovedFromManager(Sexy::WidgetManager *)
// @0013EB38  4 bytes
// ==========================================================================
void SavingProfileDialog::RemovedFromManager(Sexy::WidgetManager *theManager)
{
  Sexy::WidgetContainer::RemovedFromManager(theManager);
}

// ==========================================================================
// SavingProfileDialog::SavingProfileDialog(LawnApp *,bool)
// @0013FA58  132 bytes
// ==========================================================================
SavingProfileDialog::SavingProfileDialog(LawnApp *theApp, bool theDrawFade)
  : SavingDingusDialog(theApp, true, 79, Sexy::String(""), 0.0f, true)
{
  mDrawFade = theDrawFade;
}

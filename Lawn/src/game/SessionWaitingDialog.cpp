// SessionWaitingDialog -- transcribed from libGameMain.so
// Rewritten by hand in stage 2; see build/phase2-kickoff.md.

#include "SessionWaitingDialog.h"

#include "Dialog.h"
#include "Graphics.h"
#include "LawnApp.h"

// ==========================================================================
// SessionWaitingDialog::MouseDrag(int,int)
// @001EADDC  2 bytes
// ==========================================================================
void SessionWaitingDialog::MouseDrag(int theX, int theY)
{
    (void)theX;
    (void)theY;
}

// SessionWaitingDialog::~SessionWaitingDialog()
// @001EAE48  110 bytes
// --------------------------------------------------------------------------
// The dump deletes the widget, releases the string rep, restores both vptrs and
// then calls Sexy::Dialog::~Dialog explicitly. With a real base class and a real
// String member the compiler emits all of that, so only the delete is left.
SessionWaitingDialog::~SessionWaitingDialog()
{
  delete mMessageWidget;
}

// ==========================================================================
// SessionWaitingDialog::Update()
// @001EB018  42 bytes
// ==========================================================================
void SessionWaitingDialog::Update()
{
  mMessageWidget->Update();
  // The dump calls vtable slot 25 (byte offset 100), which is
  // WidgetContainer::MarkDirty.
  MarkDirty();

  if (mDelay > 0)
  {
    mDelay--;
    if (mDelay == 0)
      mResult = 0;
  }
}

// ==========================================================================
// SessionWaitingDialog::Draw(Sexy::Graphics *)
// @001EBA88  26 bytes
// ==========================================================================
void SessionWaitingDialog::Draw(Sexy::Graphics *theGraphics)
{
  LawnApp *theApp = gLawnApp;
  if (theApp->mLoaded)
    mMessageWidget->Draw(theGraphics);
}

// ==========================================================================
// SessionWaitingDialog::SessionWaitingDialog(LawnApp *,int)
// @001EBAB8  396 bytes
// ==========================================================================
// The dump builds three empty Sexy::String temporaries (StringCtor over the empty
// C string at 0x57D7EA) for the header/lines/footer arguments and releases them
// again at the end; the default-constructed String spells the same thing.
SessionWaitingDialog::SessionWaitingDialog(LawnApp *theApp, int theDialogId)
  : Dialog(nullptr, nullptr, theDialogId, true, Sexy::String(), Sexy::String(), Sexy::String(), 0)
{
  mMessageWidget = new SimpleMessageWidget(theApp);
  mMessageWidget->SetLabel(Sexy::String("[CONNECTION_SERVER]"), (SimpleMessageStyle)7);
  Resize(0, 0, theApp->mWidth, theApp->mHeight);
  mDelay = 0;
}

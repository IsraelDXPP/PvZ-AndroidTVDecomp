// LawnMenuWidget -- transcribed from libGameMain.so
// Bodies are the IDA decompilation with syntactic changes only;
// see build\phase2-kickoff.md for the refactor rules.

#include "LawnMenuWidget.h"

#include "LawnApp.h"
#include "LawnMenuParser.h"
// ==========================================================================
// LawnMenuWidget::CreateMenuParser()
// @00169140  96 bytes
// ==========================================================================
int LawnMenuWidget::CreateMenuParser()
{
    // A parser is reference counted (LawnMenuParser.h): +0x4C is
    // RefCount::mRefCount. When the count reaches zero the binary destroys the
    // object through the RefCount sub-object at +0x48 --
    // `LDR R3,[R0,#0x48] ; LDR R3,[R3,#4] ; BLX R3`, which is the secondary
    // vtable's deleting-destructor slot, its thunk subtracting the same 0x48.
    // `delete` on the LawnMenuParser pointer emits the identical call.
    if ( mMenuParser != nullptr )
    {
        if ( --mMenuParser->mRefCount <= 0 )
            delete mMenuParser;
        mMenuParser = nullptr;
    }

    LawnMenuParser* aParser = new LawnMenuParser(this);
    // 0x169168 re-reads mMenuParser here, but the release above has already
    // cleared it, so the old handle is always null and only the adopt branch
    // below can actually run.
    if ( mMenuParser != aParser )
    {
        if ( aParser != nullptr )
            ++aParser->mRefCount;
        mMenuParser = aParser;
    }
    // The dump's `result` is that same re-read handle, i.e. always zero.
    return 0;
}

// ==========================================================================
// LawnMenuWidget::~LawnMenuWidget()
// @0016930C  220 bytes
// ==========================================================================
LawnMenuWidget::~LawnMenuWidget()
{
  // The dump opens with two vptr stores (this and +0x100, the ButtonListener
  // base MenuWidget carries) and closes with a third that restores Widget's
  // table; the compiler emits all three, so none is written here.
  //
  // The one thing this class does beyond its base is hand back the reanimation
  // it started for the menu's backdrop. LawnMenuWidget's own member area is
  // `int data118[3]` (+0x118..+0x123), and the binary reads word 70 (+0x118),
  // clears it and passes it to LawnApp::RemoveReanimation, which is how the
  // authoritative header's `ReanimationID mBackdropReanimID` is reached.
  // TODO(recon): name data118[0] as that ReanimationID once a header pass owns
  // the member table; only word 70 is ever touched, and words 71/72 are not.
  ((LawnApp *)gLawnApp)->RemoveReanimation((ReanimationID)data118[0]);
  data118[0] = 0;
  // The trailing indirect calls are MenuWidget's own teardown -- the parser's
  // reference-counted release and the mSourceFileName rep -- and C++ runs the
  // base destructor for them.
}

// HelpTextImageWidget -- restored from libGameMain.so.

#include "HelpTextImageWidget.h"

#include "Graphics.h"
#include "Image.h"
#include "SharedImageRef.h"
#include "Widget.h"
// ==========================================================================
// HelpTextImageWidget::~HelpTextImageWidget()
// @0017C5D8  34 bytes
// ==========================================================================
HelpTextImageWidget::~HelpTextImageWidget()
{
  // The binary's body is the vtable store, ~SharedImageRef(mImage) and
  // ~Widget(); the first is the compiler's job and the other two are exactly
  // what an implicit destructor does, so nothing is left to write.
}

// ==========================================================================
// HelpTextImageWidget::Draw(Sexy::Graphics *)
// @0017C620  40 bytes
// ==========================================================================
void HelpTextImageWidget::Draw(Sexy::Graphics *theGraphics)
{
  theGraphics->DrawImage(mImage, mX, mY, mWidth, mHeight);
}

// ==========================================================================
// HelpTextImageWidget::HelpTextImageWidget()
// @0017C648  40 bytes
// ==========================================================================
HelpTextImageWidget::HelpTextImageWidget()
{
  // mImage is the only own member and default-constructs itself; the binary's
  // explicit call is that construction, and its remaining store clears mClip.
  mClip = false;
}

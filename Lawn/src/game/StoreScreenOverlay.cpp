// StoreScreenOverlay -- transcribed from libGameMain.so

#include "StoreScreenOverlay.h"

#include "Graphics.h"
#include "Widget.h"

// ==========================================================================
// StoreScreenOverlay::Draw(Sexy::Graphics *)
// @0018D7A8  16 bytes
//
// The binary loads the widget at +0x100 and calls its vtable slot 54, i.e.
// Widget::DrawOverlay: the overlay draws on the store screen's behalf.
// ==========================================================================
void StoreScreenOverlay::Draw(Sexy::Graphics *g)
{
  mStoreScreen->DrawOverlay(g);
}

// ==========================================================================
// StoreScreenOverlay::~StoreScreenOverlay()
// @0018D7BC  24 bytes
//
// The binary stores the vptr and calls Sexy::Widget::~Widget, which is what the
// compiler emits for this empty body.
// ==========================================================================
StoreScreenOverlay::~StoreScreenOverlay()
{
}

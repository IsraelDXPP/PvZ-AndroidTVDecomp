// TestMenuWidget -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class TestMenuWidget : public Sexy::MenuWidget`
// (RTTI "14TestMenuWidget", primary base "N4Sexy10MenuWidgetE" at offset 0,
// vtable 0x683558).
//
// It adds NOTHING of its own -- no member, no override. All 128 slots of its
// vtable still hold MenuWidget's or Widget's addresses, so the class declares no
// virtual function at all and the compiler's own destructor and copy of the base
// vtable do the whole job:
//   [  0] 0x19d848  TestMenuWidget::~TestMenuWidget   (D1)
//   [  1] 0x19d8f4  TestMenuWidget::~TestMenuWidget   (D0)
//   [  2] .. [127]  MenuWidget / Widget implementations, unchanged
// MSVC x86 gives a virtual destructor ONE slot where Itanium gives two (D1+D0).
//
// The destructor confirms there is no own data: it restores the two inherited
// vptrs -- the primary one at +0x00 and the ButtonListener one at +0x100 (which
// MenuWidget's multiple inheritance puts there) -- and then tears down the two
// string members that MenuWidget itself owns, at +0x108 and +0x10C. Both of the
// offsets the transcription listed for this class (words 66 and 67) are those
// inherited members, not new ones.
//
// Object size = sizeof(MenuWidget) = 0x118. The transcribed class body was an
// opaque 0x120-byte block, eight bytes too big.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_TESTMENUWIDGET_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_TESTMENUWIDGET_H

#include "ReconTypes.h"
// The base must be complete: the object is exactly a MenuWidget plus nothing.
#include "MenuWidget.h"
#include "Widget.h"

class TestMenuWidget : public Sexy::MenuWidget
{
public:
    TestMenuWidget();
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_TESTMENUWIDGET_H

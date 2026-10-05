// BottomRotatorMenu -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class BottomRotatorMenu : public Sexy::Widget`.
// The binary's RTTI base for _ZTV17BottomRotatorMenu is N4Sexy6WidgetE, and every
// WidgetContainer slot in the table is unchanged.
//
// Object size 0x110. Sexy::Widget is 0x100, and the constructor plus Update reach
// +0x100, +0x104, +0x108 and +0x10C (BottomRotatorMenu.cpp:19-23, :53-58, :133-139),
// so the object ends at 0x110. The transcribed object was 0x120.
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x176c38  BottomRotatorMenu::~BottomRotatorMenu
//   [  1] 0x176c74  BottomRotatorMenu::~BottomRotatorMenu
//   [ 31] 0x176d44  BottomRotatorMenu::Update
//   [ 36] 0x176d48  BottomRotatorMenu::Draw
//   [ 61] 0x176d40  BottomRotatorMenu::GotFocus
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Graphics.h"
#include "Widget.h"
#include <list>

namespace Sexy { class ButtonWidget; }

class BottomRotatorMenu : public Sexy::Widget
{
public:
    virtual ~BottomRotatorMenu();
    virtual void Update();
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void GotFocus();

    // The buttons on the rotator row. Draw paints the last one added.
    std::list<Sexy::ButtonWidget *> mRotatorWidgets;
    // Subtracted from the row origin at x = 300, so it scrolls the row left.
    float mScrollOffset;

    BottomRotatorMenu();
    void AddRotatorWidget(Sexy::ButtonWidget *theWidget);
    void RemoveRotatorWidget(Sexy::ButtonWidget *theWidget);
};

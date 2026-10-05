// HelpTextWidget -- reconstructed from libGameMain.so.
//
// Polymorphic class derived from Sexy::Widget (Sexy\Widget.h): the constructor
// calls Sexy::Widget::Widget(this) and then stores its own vtable.
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x18d6d8  HelpTextWidget::~HelpTextWidget
//   [  1] 0x18d738  HelpTextWidget::~HelpTextWidget
//   [ 36] 0x18d684  HelpTextWidget::Draw
//   [ 50] 0x18d7a4  HelpTextWidget::Resize
// Slots 2..35 and 37..49 are the inherited Widget / WidgetContainer slots and
// must NOT be re-declared here.
// Itanium/ARM gives the destructor two slots; MSVC x86 gives it one, so the
// generated absolute slot numbers are one lower than the comments above.
//
// Layout after the base (sizeof(Sexy::Widget) == 0x100 in the binary):
//     +0x100  Sexy::String mText   -- `StringCtor((char*)this + 256, gEmptyCString, ...)`,
//                                     the const-char* string constructor.
//     +0x104  Sexy::Font *mFont    -- `*((_DWORD*)this + 65) = Sexy::FONT_DWARVENTODCRAFT24`
//     +0x108  int
//     +0x10C .. +0x117  three trailing words kept so sizeof stays at 0x118.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "Widget.h"

namespace Sexy {
class Font;
} // namespace Sexy

class HelpTextWidget : public Sexy::Widget
{
public:
    // No out-of-line destructor exists in HelpTextWidget.cpp, so it is defaulted
    // here to give the vtable its destructor slots.
    virtual ~HelpTextWidget() = default;

    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);

    HelpTextWidget();

    Sexy::String mText;
    Sexy::Font *mFont;
    // Draw passes this to TodDrawStringWrappedHelper as the justification.
    int mJustification;
};

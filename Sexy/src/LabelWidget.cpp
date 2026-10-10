// LabelWidget -- reconstructed from libGameMain.so.
//
// The bodies below are the binary's own code, rewritten in ordinary C++: the
// `+0x100`/`+0x104`/`+0x108` word accesses are the named members of
// LabelWidget.h, the `VTSLOT(..., 184)` call is Widget's slot 46
// (`GetColor`), the vtable store is gone (a real `virtual` declaration makes
// the compiler write the vptr) and the hand-written base constructor call is
// left to the compiler.

#include "LabelWidget.h"

#include "Font.h"
#include "Graphics.h"
#include "Widget.h"

// ==========================================================================
// Sexy::LabelWidget::Draw(Sexy::Graphics *)
// @002AB0F8  110 bytes
// ==========================================================================
void Sexy::LabelWidget::Draw(Sexy::Graphics *g)
{
    // Nothing to draw while the label is empty.
    if (mString.empty())
        return;

    if (mFont != nullptr)
        g->SetFont(mFont);

    // JUSTIFY_CENTER is the one justification the binary special-cases: the
    // label is laid out around x = 0, so shift left by half the rendered width.
    // The width comes from the font the Graphics is currently carrying.
    int aX = 0;
    if (mJustification == 1 && g->mFont != nullptr)
        aX = -g->StringWidth(mString) / 2;

    // Widget::COLOR_LABEL is index 0 of Widget::mColors
    // (PvZ\SexyAppFramework\Widget\ButtonWidget.h spells the enum out).
    g->SetColor(GetColor(0));
    g->DrawString(mString, aX, 0);
}

// ==========================================================================
// Sexy::LabelWidget::LabelWidget()
// @002AB22C  56 bytes
// ==========================================================================
Sexy::LabelWidget::LabelWidget()
{
    mFont = nullptr;
    mString = "";
    // Widget +0x45. The binary clears the clip flag here, after Widget's own
    // constructor has run.
    mClip = false;
}

// ==========================================================================
// Sexy::LabelWidget::~LabelWidget()
// @002AB1CC  44 bytes
// ==========================================================================
Sexy::LabelWidget::~LabelWidget()
{
    // The vptr store is the compiler's and the trailing
    // `Sexy::Widget::~Widget(this)` is the base, which C++ runs.
    //
    // What is left is the release of the word at +0x104, mString: the dump
    // subtracts 12 to reach the COW rep, drops its refcount and frees the rep at
    // zero. mString is declared `Sexy::String`, whose own destructor is exactly
    // that, so the body is empty.
}

// GameSlider -- reconstructed from libGameMain.so.
//
// `class GameSlider : public Sexy::Slider`.
//   RTTI: `_ZTI10GameSlider` @0x680554 records `_ZTIN4Sexy6SliderE` @0x68B650 as
//   its base. The vtable agrees: slot 31 is `Sexy::SelectableWidget::Update`
//   (0x3c5940) and slots 36/71/74/75/76/78 are Slider's own
//   Draw/KeyDown/MouseLeave/MouseMove/MouseDown/MouseUp -- Slider derives from
//   SelectableWidget, so the whole chain comes in through the base.
//
// SIZE = sizeof(Sexy::Slider); this class adds NO data members.
//   GameSlider.cpp is only two functions (GameSlider.cpp is 1744 bytes total for
//   the constructor and FocusDirectionHint) and contains no `this`-relative member
//   access at all, so nothing is added. The transcriber's 0x148 was a `this`-extent
//   placeholder -- and it cannot be trusted anyway, because the Slider base it
//   fenced off is itself still carrying a `this`-extent extent (0x7E0).
//
// VTABLE -- only one own slot:
//   [  0] 0x1680f4  ~GameSlider (deleting)
//   [ 60] 0x1680a4  FocusDirectionHint(Sexy::WidgetFocusDir)
// Everything else is Slider's, SelectableWidget's, Widget's or WidgetContainer's,
// so it is inherited and not redeclared.
// The Itanium table has a D0/D1 destructor pair; MSVC x86 keeps one slot, so only
// one `virtual ~GameSlider()` is declared.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "SelectableWidget.h"
// Defines the base class Sexy::Slider. Widget.h and SelectableWidget.h are its
// ancestors, not its definition, so both were leaving the base incomplete (C2504).
#include "Slider.h"
#include "Widget.h"

class GameSlider : public Sexy::Slider
{
public:
    virtual ~GameSlider();

    // slot 60
    virtual void FocusDirectionHint(Sexy::WidgetFocusDir theDir);

    // ---- no own data members ----

    GameSlider(Sexy::Image *theTrackImage, Sexy::Image *theThumbImage, int theId,
               Sexy::SliderListener *theSliderListener, Sexy::ButtonListener *theButtonListener);
};

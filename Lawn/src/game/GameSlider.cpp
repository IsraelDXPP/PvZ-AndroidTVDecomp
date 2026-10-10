// GameSlider -- reconstructed from libGameMain.so.
//
// The bodies below are the transcribed IDA decompilation, rewritten by hand into
// real C++ against the reconstructed headers (see Game\GameSlider.h for the
// vtable and size evidence).

#include "GameSlider.h"

#include "ButtonListener.h"
#include "Image.h"
#include "LawnApp.h"
#include "SelectableWidget.h"
#include "Slider.h"
#include "SliderListener.h"
#include "Widget.h"

// ==========================================================================
// GameSlider::FocusDirectionHint(Sexy::WidgetFocusDir)
// @001680A4  34 bytes
// ==========================================================================
// Two statements in the binary: a direct (non-virtual) call to the base
// implementation and then LawnApp::PlaySample(SOUND_BLEEP, 1) through slot 169
// of the app's vtable -- `LDR.W R3,[R3,#0x2A4]` at 0x1680BE, R1 = SOUND_BLEEP,
// R2 = 1.
void GameSlider::FocusDirectionHint(Sexy::WidgetFocusDir theDir)
{
    Sexy::Widget::FocusDirectionHint(theDir);
    ((LawnApp *)gLawnApp)->PlaySample(SOUND_BLEEP, 1);
}

// ==========================================================================
// GameSlider::~GameSlider()
// @001680D4  24 bytes
// ==========================================================================
// The binary stores this class's vtable and then calls the SelectableWidget
// base destructor explicitly.  With a real base class chain the compiler emits
// both, so the body is empty.
GameSlider::~GameSlider() {}

// ==========================================================================
// GameSlider::GameSlider(Sexy::Image *,Sexy::Image *,int,Sexy::SliderListener *,Sexy::ButtonListener *)
// @00168320  32 bytes
// ==========================================================================
// The binary placement-constructs the Slider sub-object and then restores its
// own vtable, which is exactly base-class construction.  The incoming
// ButtonListener* is not used by this constructor.
GameSlider::GameSlider(Sexy::Image *theTrackImage, Sexy::Image *theThumbImage, int theId,
    Sexy::SliderListener *theSliderListener, Sexy::ButtonListener *theButtonListener) :
    Sexy::Slider(theTrackImage, theThumbImage, theId, theSliderListener)
{
    (void)theButtonListener;
}

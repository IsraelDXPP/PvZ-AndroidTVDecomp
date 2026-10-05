// CustomScrollbarWidget -- reconstructed from libGameMain.so.
//
// Polymorphic class derived from Sexy::ScrollbarWidget (Sexy\ScrollbarWidget.h).
// The authoritative tree states it adds no member at all ("this class adds
// nothing, 6, no idea what it means") and its own comment says the size is 90
// ints, i.e. 0x168 -- one word past the base's 0x164 because of the doubles.
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x167e60  CustomScrollbarWidget::~CustomScrollbarWidget
//   [  1] 0x167e94  CustomScrollbarWidget::~CustomScrollbarWidget
// Slots 2..79 (and the 67 that follow) are the inherited ScrollbarWidget /
// Widget / WidgetContainer slots and must NOT be re-declared here.
// Itanium/ARM gives the destructor two slots; MSVC x86 gives it one, so the
// generated absolute slot number is one lower than the comment above.
//
// The methods below that also exist on the base are kept declared here on
// purpose: CustomScrollbarWidget.cpp defines them out of line, and removing the
// declarations would break that definition. They hide, rather than override, the
// ScrollbarWidget ones -- the map notes this and it is a stage-2 cleanup.
//
// Note: the class belongs in namespace Sexy (the binary's mangled name is
// _ZN4Sexy22CustomScrollbarWidget...), and the global spelling is kept alive by
// the `using` at the end of this header so the .cpp files keep compiling.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "ScrollbarWidget.h"

namespace Sexy {

class CustomScrollbarWidget : public Sexy::ScrollbarWidget
{
public:
    virtual ~CustomScrollbarWidget();

    virtual int ResizeScrollbar(int theX, int theY, int theWidth, int theHeight);
    virtual int ClampValue();
    virtual int GetThumbPosition();
    virtual int GetTrackSize();
    virtual int GetThumbSize();
    CustomScrollbarWidget(int theId, Sexy::ScrollListener *theListener);
    void RemoveScrollButtons();
};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::CustomScrollbarWidget;

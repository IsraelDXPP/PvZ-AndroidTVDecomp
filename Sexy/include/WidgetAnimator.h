// WidgetAnimator is a RefCount that drives one widget.
//
// Init stores whether completion should hide the widget and clears the tick
// counter. Update advances that counter. IsCompleted is pure; HideOnCompletion
// returns the flag Init wrote.

#pragma once

#include "ReconTypes.h"
#include "RefCount.h"
#include "Widget.h"

namespace Sexy {

class WidgetAnimator : public Sexy::RefCount
{
public:
    virtual ~WidgetAnimator();
    virtual void Init(Sexy::WidgetAnimationState theState);
    virtual void Update();
    virtual bool IsCompleted() = 0;
    virtual bool HideOnCompletion();

    int mBlendTicks;           // +0x08  fade-in length the curve animator reads
    Sexy::Widget *mWidget;     // +0x0C
    int mUpdateCounter;        // +0x10
    bool mHideOnCompletion;    // +0x14
};

} // namespace Sexy

using Sexy::WidgetAnimator;

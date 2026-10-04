// SliderListener is a one-method interface with an empty base body.
//
// The vtable holds SliderVal and nothing else, so there is no virtual destructor.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class SliderListener
{
public:
    virtual void SliderVal(int theId, double theValue);
};

} // namespace Sexy

using Sexy::SliderListener;

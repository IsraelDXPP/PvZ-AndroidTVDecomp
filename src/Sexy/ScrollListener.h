// ScrollListener is a one-method interface.
//
// AlmanacDialog inherits it as a second base. The vtable holds ScrollPosition
// and nothing else, so there is no virtual destructor.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class ScrollListener
{
public:
    virtual void ScrollPosition(int theId, double thePosition);
};

} // namespace Sexy

using Sexy::ScrollListener;

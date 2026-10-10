// ScrollListener is a one-method interface.
//
// AlmanacDialog inherits it as a second base. The vtable holds ScrollPosition
// and nothing else, so there is no virtual destructor.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SCROLLLISTENER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SCROLLLISTENER_H

#include "ReconTypes.h"

namespace Sexy
{

class ScrollListener
{
public:
    virtual void ScrollPosition(int theId, double thePosition);
};

} // namespace Sexy

using Sexy::ScrollListener;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SCROLLLISTENER_H

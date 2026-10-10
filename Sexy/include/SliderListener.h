// SliderListener is a one-method interface with an empty base body.
//
// The vtable holds SliderVal and nothing else, so there is no virtual destructor.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SLIDERLISTENER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SLIDERLISTENER_H

#include "ReconTypes.h"

namespace Sexy
{

class SliderListener
{
public:
    virtual void SliderVal(int theId, double theValue);
};

} // namespace Sexy

using Sexy::SliderListener;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SLIDERLISTENER_H

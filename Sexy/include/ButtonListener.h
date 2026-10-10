// ButtonListener -- the callback a button reports presses through.
//
// A pure interface: one vptr and no data. It is a base of Dialog and of many
// widgets, so the slot order is load-bearing. The binary keeps two destructor
// slots; MSVC keeps one, and the methods below follow in the binary's order.
// ButtonPress has two overloads; the second is the click-count form.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_BUTTONLISTENER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_BUTTONLISTENER_H

#include "ReconTypes.h"

namespace Sexy
{

class ButtonListener
{
public:
    virtual ~ButtonListener() = default;

    virtual void ButtonPress(int theId);
    virtual void ButtonPress(int theId, int theClickCount);
    virtual void ButtonDepress(int theId);
    virtual void ButtonDownTick(int theId);
    virtual void ButtonMouseEnter(int theId);
    virtual void ButtonMouseLeave(int theId);
    virtual void ButtonMouseMove(int theId, int x, int y);
};

} // namespace Sexy

using Sexy::ButtonListener;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_BUTTONLISTENER_H

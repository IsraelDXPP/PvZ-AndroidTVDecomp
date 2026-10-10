// CheckboxListener -- the callback a checkbox reports a toggle through.
//
// Pure interface, one virtual and no data. There is deliberately no virtual
// destructor: the binary table holds only CheckboxChecked, and adding a
// destructor would shift every derived class's slots.
//
// Consumer: VSResultsMenu's VSResultsCheckboxController.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_CHECKBOXLISTENER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_CHECKBOXLISTENER_H

#include "ReconTypes.h"

namespace Sexy
{

class CheckboxListener
{
public:
    virtual void CheckboxChecked(int theId, bool theChecked);
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_CHECKBOXLISTENER_H

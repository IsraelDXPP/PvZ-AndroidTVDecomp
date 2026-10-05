// CheckboxListener -- the callback a checkbox reports a toggle through.
//
// Pure interface, one virtual and no data. There is deliberately no virtual
// destructor: the binary table holds only CheckboxChecked, and adding a
// destructor would shift every derived class's slots.
//
// Consumer: VSResultsMenu's VSResultsCheckboxController.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class CheckboxListener
{
public:
    virtual void CheckboxChecked(int theId, bool theChecked);
};

} // namespace Sexy

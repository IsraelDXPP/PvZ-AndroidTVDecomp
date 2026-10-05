// Sexy::Driver -- the base of the input and video driver family.
//
// No base class of its own. The table holds only the destructor; every derived
// driver adds its own virtuals. The four words are the vptr, the name, the
// priority and the disabled flag.
//
// GetName and GetPriority belong to PakLib::Driver, declared in ReconTypes.h,
// not to this class.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class Driver
{
public:
    virtual ~Driver();

    Sexy::String mName;
    int mPriority;
    bool mDisabled;

    Driver(Sexy::String theName, int thePriority);
    int Disable(bool theDisabled);
    int IsDisabled();
};

} // namespace Sexy

using Sexy::Driver;

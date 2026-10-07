#include "Driver.h"

Sexy::Driver::Driver(Sexy::String theName, int thePriority)
    : mName(theName)
{
    mPriority = thePriority;
    mDisabled = false;
}

// The dump returns the receiver; that is the leftover return register, not a
// value the method produces.
int Sexy::Driver::Disable(bool theDisabled)
{
    mDisabled = theDisabled;
    return 0;
}

int Sexy::Driver::IsDisabled()
{
    return mDisabled;
}

// @0x444290: the first ARM argument is the hidden string return buffer.
Sexy::String PakLib::Driver::GetName() const
{
    return mName;
}

int PakLib::Driver::GetPriority()
{
    return mPriority;
}

PakLib::Driver::Driver(Sexy::String theName, int thePriority)
    : mName(theName)
{
    mPriority = thePriority;
}

Sexy::Driver::~Driver()
{
    // mName's own destructor releases the string. Driver has no base.
}

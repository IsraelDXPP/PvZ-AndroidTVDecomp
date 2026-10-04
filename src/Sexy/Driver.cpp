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

// A by-value return. The published signature still names the source as an int;
// the name copied is this driver's own, which is the only string the method has.
PakLib::Driver* PakLib::Driver::GetName(int)
{
    StringCopyCtor(static_cast<Sexy::String*>(static_cast<void*>(this)), &mName);
    return this;
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

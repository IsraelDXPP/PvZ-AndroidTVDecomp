// Shared ownership count. Release drops it and deletes the object at zero.
// The destructor is the only virtual; the count sits in the word after the vptr.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class RefCount
{
public:
    virtual ~RefCount();

    void Release();

    int mRefCount;
};

} // namespace Sexy

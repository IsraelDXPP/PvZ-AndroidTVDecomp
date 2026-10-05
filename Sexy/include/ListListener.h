// ListListener is the callback a list widget notifies. The three methods
// have empty defaults, so a listener can override only the events it cares
// about. There is no destructor slot.
//
// The word after the vptr is unused. It stays so a listener embedded in a
// larger object keeps the size the binary recorded.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class ListListener
{
public:
    virtual void ListClicked(int theId, int theIndex, int theClickCount);
    virtual void ListClosed(int theId);
    virtual void ListHiliteChanged(int theId, int theIndex, int thePrevIndex);

    int mId;
};

} // namespace Sexy

using Sexy::ListListener;

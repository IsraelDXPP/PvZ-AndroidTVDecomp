// SingleDataElement is a DataElement that holds one string and a sibling.
//
// Duplicate is the pure-virtual override. The string is one Sexy::String word;
// the sibling is deleted through its own destructor.

#pragma once

#include "ReconTypes.h"
#include "DataElement.h"

namespace Sexy {

class SingleDataElement : public DataElement
{
public:
    virtual ~SingleDataElement();
    virtual DataElement *Duplicate();

    SingleDataElement();

    Sexy::String mData;    // +0x08
    DataElement *mNext;    // +0x0C
};

} // namespace Sexy

using Sexy::SingleDataElement;

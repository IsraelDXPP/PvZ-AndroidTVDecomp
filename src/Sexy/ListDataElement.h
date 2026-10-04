// ListDataElement is a DataElement that owns a list of child elements.
// Duplicate deep-copies that list.

#pragma once

#include "ReconTypes.h"
#include "DataElement.h"

#include <vector>

namespace Sexy {

class ListDataElement : public DataElement
{
public:
    virtual ~ListDataElement();
    virtual ListDataElement *Duplicate();

    std::vector<DataElement *> mDataElements;

    ListDataElement();
    ListDataElement(Sexy::ListDataElement const& theOther);
};

} // namespace Sexy

using Sexy::ListDataElement;

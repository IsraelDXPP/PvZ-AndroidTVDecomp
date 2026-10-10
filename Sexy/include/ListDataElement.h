// ListDataElement is a DataElement that owns a list of child elements.
// Duplicate deep-copies that list.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_LISTDATAELEMENT_H
#define LAWNPROJECTTV_SEXY_INCLUDE_LISTDATAELEMENT_H

#include "ReconTypes.h"
#include "DataElement.h"

#include <vector>

namespace Sexy
{

class ListDataElement : public DataElement
{
public:
    virtual ~ListDataElement();
    virtual ListDataElement *Duplicate();

    std::vector<DataElement *> mDataElements;

    ListDataElement();
    ListDataElement(Sexy::ListDataElement const &theOther);
};

} // namespace Sexy

using Sexy::ListDataElement;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_LISTDATAELEMENT_H

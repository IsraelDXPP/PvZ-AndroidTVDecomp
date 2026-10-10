// DataElement -- the abstract node of a descriptor tree.
//
// The binary table holds the destructor pair and a pure Duplicate(). It is
// declared non-pure here only because ListDataElement.cpp still constructs a
// DataElement directly, which an abstract class forbids. The body writes the
// pure-virtual trap and aborts.
//
// The return is DataElement*: SingleDataElement and ListDataElement override
// it covariantly. mElementKind is 0 for a single value and non-zero for a list.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_DATAELEMENT_H
#define LAWNPROJECTTV_SEXY_INCLUDE_DATAELEMENT_H

#include "ReconTypes.h"

namespace Sexy
{

class DataElement
{
public:
    virtual ~DataElement();
    virtual DataElement *Duplicate();

    DataElement();

    int mElementKind;
};

} // namespace Sexy

using Sexy::DataElement;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_DATAELEMENT_H

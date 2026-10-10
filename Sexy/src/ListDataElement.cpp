// ListDataElement owns its children. The destructor deletes each one, and
// the copy constructor duplicates each one through Duplicate.

#include "ListDataElement.h"

#include "DataElement.h"

Sexy::ListDataElement::~ListDataElement()
{
    for (unsigned int i = 0; i < mDataElements.size(); ++i)
        delete mDataElements[i];
}

Sexy::ListDataElement::ListDataElement()
{
    mElementKind = 1;
}

Sexy::ListDataElement::ListDataElement(Sexy::ListDataElement const &theOther)
{
    mElementKind = 1;
    for (unsigned int i = 0; i < theOther.mDataElements.size(); ++i)
        mDataElements.push_back(theOther.mDataElements[i]->Duplicate());
}

Sexy::ListDataElement *Sexy::ListDataElement::Duplicate()
{
    return new ListDataElement(*this);
}

#include "DataElement.h"
#include <cstdio>
#include <cstdlib>

Sexy::DataElement::~DataElement() {}

Sexy::DataElement::DataElement()
{
    mElementKind = 0;
}

// The slot is pure in the binary. This body is the trap: report it and stop.
// Declared non-pure only so ListDataElement.cpp can still construct the base.
DataElement *Sexy::DataElement::Duplicate()
{
    fprintf(stderr, "pure virtual method called\n");
    fflush(stderr);
    abort();
    return nullptr;
}

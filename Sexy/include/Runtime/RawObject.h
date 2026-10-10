#ifndef LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_RAWOBJECT_H
#define LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_RAWOBJECT_H

#include <new>

// First construction of a DataArray slot. The bytes come from operator new[]
// and have never run a constructor. This is not a second construction of a
// member the compiler already built.
template <class T> T *ConstructRaw(T *theMemory)
{
    return new (theMemory) T();
}

#endif // LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_RAWOBJECT_H

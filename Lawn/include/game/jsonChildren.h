// jsonChildren -- reconstructed from libGameMain.so.
//
// Plain class: no base class and no virtual function (the recon dump has only
// deleteAll/inc/reserve2/doerase and none of them writes a vptr).
//
// Layout, read off the recon bodies:
//     +0x00  JSONNode **mChildren -- the malloc/realloc'd array. inc() does
//                                   `realloc(*this, 8 * count)` for the first
//                                   growth path and `malloc(0x20)` for the empty
//                                   case, i.e. eight pointers.
//     +0x04  unsigned int mSize     -- `*(this + 1)`; doerase() maintains it and
//                                      deleteAll() walks `&(*this)[size]`.
//     +0x08  unsigned int mCapacity -- `*(this + 2)`; doubling growth.
//     +0x0C  three trailing words kept so sizeof stays at the 0x18 the
//            transcription allocated.
//
// TODO(recon): the original jsoncpp struct has exactly the three fields above
// (12 bytes); confirm the allocation size before dropping the padding.

#pragma once

#include "ReconTypes.h"
// JSONNode.h and internalJSONNode.h are deliberately NOT included, and these are
// the two edges that closed this cluster's cycles:
//   jsonChildren.h -> JSONNode.h -> internalJSONNode.h -> jsonChildren.h
//   jsonChildren.h -> internalJSONNode.h -> jsonChildren.h
//
// The comment that used to sit here said the includes were kept "so other files'
// transitive dependencies do not change". That is true in the other direction --
// Game\internalJSONNode.cpp and Game\JSONWorker.cpp both dereference a
// jsonChildren and neither includes this header, so internalJSONNode.h and
// JSONWorker.h still include it for them -- but it is not true of anything this
// header declares. `mChildren` is `JSONNode **` and `doerase` takes a
// `JSONNode **`, so pointers are the whole requirement and ReconTypes.h already
// forward-declares JSONNode (line 1384). The names are only forward declarations
// here; every complete type the transcribed bodies need is named in the .cpp.
class JSONNode;
class internalJSONNode;

class jsonChildren
{
public:
    JSONNode **mChildren;
    unsigned int mSize;
    unsigned int mCapacity;

    jsonChildren();
    void deleteAll();
    // Grow the array when it is full, or when theExtra more entries will not fit.
    // Both return the child array, which may have been reallocated.
    JSONNode **inc();
    JSONNode **inc(unsigned int theExtra);
    // Static helper. theChildren is the array it grows, not an implicit this.
    static JSONNode **reserve2(jsonChildren *&theChildren, unsigned int theCapacity);
    // Erase theCount entries starting at theStart, then shift the tail down.
    void doerase(JSONNode **theStart, unsigned int theCount);
};

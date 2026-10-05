// ReanimationHolder -- TV DataArray<Reanimation> expanded in place.
//
// Not polymorphic: no vtable. The seven words are the array header
// (block, used count, capacity, free-list head, live count, next key, name).
// Element stride is 220: the Reanimation plus a 4-byte slot id. Capacity is 1024.
// Nothing after the name is read or written, so the old padding is gone.

#pragma once

#include "ReconTypes.h"
#include "DataArray.h"

class Reanimation;

class ReanimationHolder : public DataArray<Reanimation, ReanimationSlot>
{
public:

    ReanimationHolder();
    ~ReanimationHolder();

    // Returns 1 after the block is allocated. Callers ignore the number.
    int InitializeHolder();
    void DisposeHolder();
    Reanimation* AllocReanimation(float theX, float theY, int theRenderOrder, ReanimationType theReanimationType);
};

// RefCount -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from RefCount.h.

#include "RefCount.h"
// ==========================================================================
// Sexy::RefCount::~RefCount()
// @00149050  14 bytes
// ==========================================================================
// The binary's body is the 14-byte vptr store alone, so with `virtual` declared
// on the header the body is empty and the compiler emits the store itself.
Sexy::RefCount::~RefCount() {}

// ==========================================================================
// Sexy::RefCount::Release()
// @002AF3C0  22 bytes
// ==========================================================================
// The dump's tail call reads the object's first word and then calls through its
// second entry (`LDR R3,[R3,#4]` / `BLX R3`), which is the vtable's deleting
// destructor -- exactly what `delete this` compiles to. The countdown runs on
// the word at +0x04 and releases the object at or below zero, so the body is the
// classic reference-count release.
void Sexy::RefCount::Release()
{
    if (--mRefCount <= 0)
        delete this;
}

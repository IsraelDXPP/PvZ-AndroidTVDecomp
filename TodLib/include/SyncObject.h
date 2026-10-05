// SyncObject -- the base class of every game object that takes part in
// cross-peer state synchronisation.
//
// The layout is proven by the binary:
//   ~SyncObject      @0x13C550  `v2 = *((void **)this + 1); ... operator delete(v2)`
//   GameObject::ctor @0x1B7BB4  `STR R6,[R4,#4] / [R4,#8] / [R4,#0xC]`, three
//                               zero stores in a row, then `ADDS R0, #4` as the
//                               receiver of the vector
// so:
//   +0x00  the vtable pointer (written by the compiler)
//   +0x04  mSyncBlocks    std::vector<SyncBlockInfo>, three pointers, 12 bytes
//   total 0x10
//
// The binary's table for this class holds 2 slots and both are destructors, so
// declaring a virtual destructor here reproduces it:
//   [  0] 0x13c550  SyncObject::~SyncObject   (D1)
//   [  1] 0x13c574  SyncObject::~SyncObject   (D0)
//
// The original transcription kept 0x8 bytes of opaque storage, which was 8 bytes
// short of the real size; that was then widened to a vtable-pointer word plus
// mSyncBlocks, and the class is now a real polymorphic base with the vtable
// pointer left to the compiler. That is why the statement in every derived
// constructor that stored a recon table address into the object's first word
// (`*(_DWORD *)this = &off_XXXX;`) has to go -- it would overwrite the pointer
// the compiler wrote.

#pragma once

#include "ReconTypes.h"

#include <vector>

class SyncObject
{
public:
    virtual ~SyncObject() = default;

    std::vector<SyncBlockInfo> mSyncBlocks;
};

// Layout check: the vtable pointer plus the vector's three pointers.

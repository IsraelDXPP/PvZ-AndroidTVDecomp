// TodParticle -- reconstructed from libGameMain.so.
//
// `class TodParticle : public SyncObject`.
//   RTTI: `_ZTI11TodParticle` @0x67F0F8 records `_ZTI10SyncObject` @0x67CDB8 as its
//   base. (The authoritative PvZ\TodLib\Effect\TodParticle.h does not declare a
//   `TodParticle` class at all -- it only has TodParticleHolder, TodParticleEmitter
//   and TodParticleSystem -- so RTTI is the only evidence, and it is unambiguous.)
//
// SIZE = 0xAC. SyncObject is 0x10, the emitter pointer is +0x10, and the 152-byte
// synced region is +0x14. There is no constructor symbol: SyncDataArray's read
// path inlines the init (vtable, one registered block, memset of that region).
//
// VTABLE -- two slots, both the destructor:
//   [  0] 0x142638  TodParticle::~TodParticle (deleting)
// SyncObject already declares `virtual ~SyncObject()`, so `virtual ~TodParticle()`
// is the only declaration needed. MSVC x86 gives a virtual destructor one slot while
// the Itanium table has a D0/D1 pair.

#pragma once

#include "ReconTypes.h"
// The base must be complete: SyncObject supplies the vtable TodParticle reuses.
#include "SyncObject.h"

class TodParticleEmitter;

class TodParticle : public SyncObject
{
public:
    virtual ~TodParticle();

    TodParticleEmitter *mParticleEmitter; // +0x10
    unsigned char mSyncState[152];        // +0x14, ends at +0xAC
};

struct ParticleSlot
{
    TodParticle mItem;
    unsigned int mID;
};


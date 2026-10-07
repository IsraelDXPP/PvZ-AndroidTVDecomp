// SaveGameContext -- reconstructed from libGameMain.so.
//
// Polymorphic base: the bodies call `Sexy::Buffer::WriteLong(this, ...)`,
// `Sexy::Buffer::ReadLong(this)` and `Sexy::Buffer::WriteBytes(this, ...)` with
// `this` as the receiver, i.e. `class SaveGameContext : public Sexy::Buffer` with
// Buffer at offset 0. The class has no vtable symbol of its own, so it adds no
// virtual function and simply inherits Buffer's.
//
// Layout, from _recon/ida/SaveGameContext.cpp:
//   ByteLeftToRead @0x142664  (+0x10 - +0x14 + 7) >> 3, both fields belonging to
//                             Buffer (the byte cursor)
//   SyncInt        @0x142678  reads +0x1D: zero means "write", non-zero "read"
//                             reads +0x1C: set when a read ran out of bytes
//   SyncBytes      @0x1426B0  same two flags
//
// Only the two flags are this class's own; everything below +0x10 is Buffer's.
// The transcribed class body, an opaque 0x28-byte block, covered both classes at
// once.

#pragma once

#include "ReconTypes.h"
// The base must be complete, and the definitions below are method parameters.
#include "Buffer.h"
#include "ReanimatorDefinition.h"
#include "TodParticleDefinition.h"
#include "TrailDefinition.h"

class SaveGameContext : public Sexy::Buffer
{
public:
    // TODO(recon): the authoritative tree has no header for this class, so the two
    // flags below carry names derived from their use rather than from a symbol.
    bool mReadError = false;  // +0x1C  set when a read found fewer bytes than it needed
    bool mReading = false;    // +0x1D  non-zero selects the read path, zero the write path

    int ByteLeftToRead();
    unsigned int SyncInt(int &theValue);
    signed int SyncBytes(void *theData, int theSize);
    unsigned int SyncUint(unsigned int &theValue);
    int SyncReanimationDef(ReanimatorDefinition *&theDef);
    unsigned int SyncParticleDef(TodParticleDefinition *&theDef);
    unsigned int SyncTrailDef(TrailDefinition *&theDef);
    Sexy::Image * SyncImage(Sexy::Image *&theImage);
};

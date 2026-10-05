// TodParticleDefinition -- reconstructed from libGameMain.so.
//
// A plain 8-byte data class: no vtable in the RTTI export, no base class. The
// member table is the authoritative header PvZ\TodLib\Effect\TodParticle.h, and
// the size is confirmed by `(TodParticleDefinition *)operator new of 8 bytes` at
// Board.cpp:22319. The old placeholder's 0x18 was an over-allocation.

#pragma once

#include "ReconTypes.h"
#include "Image.h"

// TodEmitterDefinition -- the per-emitter definition block a TodParticleDefinition
// owns. One definition is 0x164 bytes: TodParticleInitializeFromDef advances its
// cursor with `ADD.W R8, R8, #0x164` @0x20a594, and FindEmitterDefByName's
// `ADD.W R4, R4, #0x164` @0x2092f8 agrees. Only the fields any body in this tree
// actually reads are named:
//   +0x14  the flags word whose bit 0x80 means "create only while the holder
//          still has room" (TodParticleInitializeFromDef @0x20a5ae does
//          `LDR R3,[R7,#0x14] / LSLS R6,R3,#0x18`, i.e. a test of bit 7);
//   +0x1C  the emitter's name, the string every TodParticleSystem::OverrideXxx
//          and FindEmitterByName matches with strcasecmp (`LDR R0,[R4,#0x10]` then
//          `LDR R0,[R3,#0x1C]`);
//   +0x24, +0x2C, +0x94  three FloatParameterTracks: the two
//          TodParticleEmitter::TodEmitterInitialize @0x20a42c evaluates to size
//          its first burst, and the one TodParticleInitializeFromDef tests to
//          decide whether the definition may be created at all.
class TodEmitterDefinition
{
public:
    Sexy::Image        *mImage;         // +0x00
    int                 mImageCol;      // +0x04
    int                 mImageRow;      // +0x08
    int                 mImageFrames;   // +0x0C
    int                 mAnimated;      // +0x10
    int                 mFlags;         // +0x14
    unsigned char       mEmitterPad18[4]; // +0x18 .. +0x1B
    const char         *mName;          // +0x1C
    unsigned char       mEmitterPad20[4]; // +0x20 .. +0x23
    FloatParameterTrack mSystemDuration; // +0x24
    FloatParameterTrack mCrossFadeDuration; // +0x2C
    unsigned char       mEmitterTracks[0x60]; // +0x34 .. +0x93  tracks before the X offset
    FloatParameterTrack mEmitterOffsetX; // +0x94
    unsigned char       mEmitterTail[0xC8]; // +0x9C .. +0x163  tracks after the X offset
    // size 0x164
};


class TodParticleDefinition
{
public:
    TodEmitterDefinition *mEmitterDefs; // +0x00
    int mEmitterDefCount;              // +0x04
    // size 0x08

    TodParticleDefinition();
};


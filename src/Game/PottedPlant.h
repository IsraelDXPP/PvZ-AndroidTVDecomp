// PottedPlant -- reconstructed from libGameMain.so.
//
// Plain (non-polymorphic) value type: the binary never stores a vptr in it and
// never enters a method through a vtable. Size 0x14, from two independent
// pieces of evidence:
//   * LawnApp::GetPottedPlantByIndex @0x2B62B0 computes
//     `*(_DWORD*)(this + 587*4) + 20 * a2 + 792` -- the array stride is 20;
//   * InitializePottedPlant @0x17D9B0 itself does `memset(this, 0, 0x14)` and
//     writes to `this[19]`.
// It is stored BY VALUE (stride 20) inside LawnPlayerInfo's Zen-garden arrays, so
// the object size IS the stride here -- this is an embedded array, not a
// `DataArray<T>`-managed one, and the four bytes of `DataArrayItem::mID` must NOT
// be subtracted. The old opaque byte array of 8 bytes would have overrun 12.
//
// Member names, order and bit widths are from the authoritative header
// PvZ\Lawn\System\PlayerInfo.h:32, which restores this same binary: one
// bitfield word packs everything up to mFeedingsPerGrow, then four ints hold
// the timestamps.

#pragma once

#include "ReconTypes.h"

class PottedPlant
{
public:
    enum FacingDirection
    {
        FACING_RIGHT,
        FACING_LEFT
    };

    // The leading bitfield word, +0x00..+0x03.
    SeedType       mSeedType : 7;      // bits 0..6
    FacingDirection mFacing : 1;       // bit 7
    GardenType     mWhichZenGarden : 2; // bits 8..9
    int            mY : 2;             // bits 10..11
    // The authoritative header spells this field `DrawVariation mDrawVariation : 4`,
    // but ReconTypes.h models DrawVariation as a one-int struct wrapper, and C++ allows
    // only integral or fixed-underlying-type enum bit-fields, not class types. Using
    // `unsigned int` keeps the field 4 bits wide and in place at bits 12..15, so the
    // bitfield word and sizeof are unchanged. If DrawVariation is ever promoted to a
    // real enum in ReconTypes.h, this line reverts to `DrawVariation mDrawVariation : 4;`.
    unsigned int   mDrawVariation : 4; // bits 12..15 (DrawVariation, 4 bits)
    int            mX : 3;             // bits 16..18
    PottedPlantNeed mPlantNeed : 3;    // bits 19..21
    PottedPlantAge mPlantAge : 2;      // bits 22..23
    int            mTimesFed : 3;      // bits 24..26
    int            mFeedingsPerGrow : 3; // bits 27..29
    int            : 2;                // bits 30..31

    int mLastWateredTime;         // +0x04
    int mLastNeedFulfilledTime;   // +0x08
    int mLastFertilizedTime;      // +0x0C
    int mLastChocolateTime;       // +0x10
    // size 0x14

    void InitializePottedPlant(SeedType theSeedType);
};

// Layout check: one bitfield word plus four ints.

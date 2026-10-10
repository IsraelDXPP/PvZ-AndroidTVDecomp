// ReanimatorDefinition -- reconstructed from libGameMain.so.
//
// Plain (non-polymorphic) data type: no vptr, no vtable, no virtual methods.
// Size 0x10 (4 words), from two independent sources that agree:
//   * ReanimatorDefinition::ReanimatorDefinition @0x... writes exactly words
//     0..3 and puts 0x41400000 (12.0f) into word 2, which is the default FPS;
//   * the authoritative header PvZ\TodLib\Effect\Reanimator.h:67 lists the same
//     four members and closes with its own note that the size is four integers.
// The old opaque byte array of 0x20 bytes was a transcriber placeholder derived
// from the largest `this`-relative offset in the IDA dump; the constructor writes
// only words 0..3, so it overhung the object by 0x10.
//
// ReanimatorDefinition.cpp touches no byte past +0x0C.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATORDEFINITION_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATORDEFINITION_H

#include "ReanimatorTransform.h"
#include "ReconTypes.h"

class ReanimAtlas;

// The track record the definition owns. Reanimation.cpp and Zombie.cpp both
// walk it: 20 bytes, the name at +0x04 and the transform array at +0x08
// (PvZ\TodLib\Effect\Reanimator.h:53). ReanimatorTrackInstance stays in
// Reanimation.h, which already owns the 100-byte runtime record.
class ReanimatorTrack
{
public:
    const char *mTrackId; // +0x00 interned track-name id
    char *mName; // +0x04
    ReanimatorTransform *mTransforms; // +0x08
    int mTransformCount; // +0x0C
    bool mIsAttacher; // +0x10
};

class ReanimatorTrackInstance;

class ReanimatorDefinition
{
public:
    ReanimatorTrack *mTracks; // +0x00
    int mTrackCount; // +0x04
    float mFPS; // +0x08, default 12.0f
    ReanimAtlas *mReanimAtlas; // +0x0C
    // size 0x10

    ReanimatorDefinition();
};

// Layout check: four words.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATORDEFINITION_H

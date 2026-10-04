// TodFoley -- reconstructed from libGameMain.so.
//
// Plain data class: no virtual functions and no vtable. _recon/vtables.json has no
// entry for TodFoley, and TodFoley.cpp reaches every method through a direct
// address, so nothing here is virtual.
//
// The whole object is one array of FoleyTypeData records, one per FoleyType:
//   class FoleyTypeData { FoleyInstance mFoleyInstances[8]; int mLastVariationPlayed; };
//   class FoleyInstance { AudiereSoundInstance *mInstance; int mRefCount; bool _paused;
//                         int mStartTime; int mPauseOffset; };
// so a record is 8 * 20 + 4 = 164 bytes and the object is 110 * 164 = 0x4678.
//
// BOTH record types now come from Game\FoleyTypeData.h instead of being spelled out
// here. This file used to carry its own `TodFoleyInstance` and its own
// `FoleyTypeData`, which collided by name with two other definitions:
//   * Sexy\ReconTypes.h:408 owns the GLOBAL name `TodFoleyInstance` for a one-int
//     enum shim (a transcription catch-all, the same shape as `FoleyInstance` on
//     line 210). ReconTypes.h is included first by everything, so the local struct
//     was a C2011 redefinition -- and because the redefinition failed, the very
//     same struct was then "undefined" at the point FoleyTypeData used it (C2079).
//     MSVC kept the 4-byte shim, so all three static_asserts below collapsed.
//   * Game\FoleyTypeData.h:32 already defines the real `FoleyTypeData`, and it is
//     the file LawnApp.cpp and FoleyTypeData.cpp include for it, so a translation
//     unit that pulled in both headers would have had two different `FoleyTypeData`
//     classes.
// FoleyTypeData.h is the correct owner: it models the element as
// `FoleyTypeDataElement` (Sexy\ReconTypes.h has taken the authoritative name
// `FoleyInstance` for the shim above) and asserts both sizes itself.
//
// --- why 20 and 164 and 0x4678 are the right numbers --------------------------
//   * The element record is 20 bytes, from the authoritative header
//     PvZ\TodLib\Common\TodFoley.h (`class FoleyInstance`): an ARM32 pointer, an
//     int, a bool padded out to the next int boundary, then two ints --
//     0/4/8/(12)/12/16, so 20. The binary agrees twice in TodFoley.cpp:
//     CancelPausedFoley walks instances with `v5 += 20` (:130, :135) and
//     RehookupSoundWithMusicVolume does the same at :331.
//   * A FoleyTypeData record is 164 bytes = 0xA4: FoleyTypeData's constructor
//     walks `this` in steps of 20 up to `this + 160` (8 instances) and then stores
//     -1 at +160, which is exactly the shape Game\FoleyTypeData.h declares. The
//     strides in TodFoley.cpp are 164 (GamePause `i += 164` at :56,
//     CancelPausedFoley `v4 += 164` at :143, RehookupSoundWithMusicVolume
//     `v3 += 164` at :343).
//   * The object is 0x4678 = 18040 = 110 * 164. This is the allocation point:
//     Game\LawnApp.cpp:12357 allocates 0x4678 bytes for it, then loops
//     constructing one record per 164-byte step and stops at
//     `v5 + 18040` (:12363-12365). 110 is EXTENDED_NUM_FOLEY.
//
// The transcribed object used to be 0xB8, only one record and a bit.

#pragma once

#include "ReconTypes.h"
// The record types: `FoleyTypeData` (0xA4) and its 20-byte element
// `FoleyTypeDataElement`. They used to be duplicated here; see the note above.
#include "FoleyTypeData.h"

// EXTENDED_NUM_FOLEY in the authoritative PvZ\Lawn\Common\ConstEnums.h, which is
// the enum this array is sized by. The Community restoration and the old symbol
// dump both give 110.
inline constexpr int TODFOLEY_NUM_TYPES = 110;

class TodFoley
{
public:
    FoleyTypeData mTypeData[TODFOLEY_NUM_TYPES]; // +0x0000 .. +0x4677
    // size 0x4678

    // The element type is spelled FoleyTypeDataElement and not `FoleyInstance`
    // because Sexy\ReconTypes.h owns the global name `FoleyInstance` for a
    // four-byte enum shim; see Game\FoleyTypeData.h.
    FoleyTypeDataElement * StopFoley(FoleyType theFoleyType);
    int GamePause(bool thePause);
    int CancelPausedFoley();
    void ApplyMusicVolume(FoleyTypeDataElement *theInstance);
    FoleyTypeDataElement * PlayFoleyPitch(FoleyType theFoleyType, float thePitch);
    FoleyTypeDataElement * PlayFoley(FoleyType theFoleyType);
    int RehookupSoundWithMusicVolume();
    bool IsFoleyPlaying(FoleyType theFoleyType);
};

// Layout checks, all three from TodFoley.cpp's own strides and from the
// 0x4678-byte allocation site in LawnApp.cpp:12357. The element and record sizes
// are also asserted in Game\FoleyTypeData.h; they are repeated here because 164 is
// the array's stride and 110 * 164 is this class's whole size.

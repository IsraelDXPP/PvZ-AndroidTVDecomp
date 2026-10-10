// FoleyTypeData -- reconstructed from libGameMain.so.
//
// No base class and no virtual function. The authoritative header
// PvZ\TodLib\Common\TodFoley.h declares it as
//     struct FoleyTypeData { FoleyInstance mFoleyInstances[MAX_FOLEY_INSTANCES]; int mLastVariationPlayed; };
// and the constructor at 0x1FF754 confirms both the element size and the count:
// it walks `this` in steps of 20 bytes up to `this + 160` (8 elements) calling
// `FoleyInstance::FoleyInstance()` on each, then stores -1 at +160.
//
// SIZE 0xA4 = 8 * 20 + 4. The transcriber's 0xB0 was 12 bytes of over-count.
//
// NAMING NOTE: Sexy\ReconTypes.h already owns the global name `FoleyInstance` for
// a one-int enum shim, so the element type here cannot use it.
// TODO(recon): once that shim is gone, rename FoleyTypeDataElement to FoleyInstance.

#ifndef LAWNPROJECTTV_TODLIB_INCLUDE_FOLEYTYPEDATA_H
#define LAWNPROJECTTV_TODLIB_INCLUDE_FOLEYTYPEDATA_H

#include "ReconTypes.h"
#include "SoundInstance.h"

// ---------------------------------------------------------------------------
// FoleyParams -- the per-FoleyType sound definition.
//
// It lives in Sexy\ReconFreeFunctions.h next to gLawnFoleyParamArray /
// gFoleyParamArray / LookupFoley, which is where the placeholder struct used to
// be; that placeholder has been replaced by the real layout, taken from the
// authoritative header PvZ\TodLib\Common\TodFoley.h. See that file for the field
// comments.
// ---------------------------------------------------------------------------

// The sound-instance record. mInstance is a Sexy::SoundInstance; the Windows
// override is SDLSoundInstance, reached through the virtual call.
struct FoleyTypeDataElement
{
    Sexy::SoundInstance *mInstance; // +0x00
    int mRefCount; // +0x04
    bool mPaused; // +0x08
    int mStartTime; // +0x0C
    int mPauseOffset; // +0x10
};

class FoleyTypeData
{
public:
    static const int MAX_FOLEY_INSTANCES = 8;

    FoleyTypeDataElement mFoleyInstances[MAX_FOLEY_INSTANCES]; // +0x00 .. +0x9F
    int mLastVariationPlayed; // +0xA0
    // size 0xA4

    FoleyTypeData();
};

// Layout check: 8 records of 20 bytes plus the variation counter.

#endif // LAWNPROJECTTV_TODLIB_INCLUDE_FOLEYTYPEDATA_H

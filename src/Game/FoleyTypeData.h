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

#pragma once

#include "ReconTypes.h"

// ---------------------------------------------------------------------------
// Sexy::AudiereSoundInstance -- the sound object a foley instance points at.
//
// Taken word for word from the authoritative header PvZ\TodLib\Common\TodFoley.h
// (class AudiereSoundInstance), which is also where the slot table comes from.
// Only the vptr exists in this reconstruction; every call site goes through
// GetVTable(), exactly as the ground-truth bodies in
// PvZ-src\TodLib\Common\TodFoley.cpp do.
// ---------------------------------------------------------------------------
namespace Sexy
{

class AudiereSoundInstance
{
public:
    struct AudiereSoundInstanceVTable
    {
        void (*completeDestructor)(Sexy::AudiereSoundInstance *self);                   // 0x00
        void (*deletingDestructor)(Sexy::AudiereSoundInstance *self);                   // 0x04
        void (*Release)(Sexy::AudiereSoundInstance *self);                              // 0x08
        void (*SetBaseVolume)(Sexy::AudiereSoundInstance *self, double volume);         // 0x0C
        void (*SetBasePan)(Sexy::AudiereSoundInstance *self, int pan);                  // 0x10
        void (*SetBaseRate)(Sexy::AudiereSoundInstance *self, double rate);             // 0x14
        void (*AdjustPitch)(Sexy::AudiereSoundInstance *self, double pitch);            // 0x18
        void (*SetVolume)(Sexy::AudiereSoundInstance *self, double volume);             // 0x1C
        void (*SetMasterVolumeIdx)(Sexy::AudiereSoundInstance *self, int volumeIndex);  // 0x20
        void (*SetPan)(Sexy::AudiereSoundInstance *self, int pan);                      // 0x24
        bool (*Play)(Sexy::AudiereSoundInstance *self, bool looping, bool autoRelease); // 0x28
        void (*Stop)(Sexy::AudiereSoundInstance *self);                                 // 0x2C
        bool (*IsPlaying)(Sexy::AudiereSoundInstance *self);                            // 0x30
        bool (*IsReleased)(Sexy::AudiereSoundInstance *self);                           // 0x34
        double (*GetVolume)(Sexy::AudiereSoundInstance *self);                          // 0x38
    };

    const AudiereSoundInstanceVTable *GetVTable() const
    {
        return mInstanceVTable;
    }

public:
    const AudiereSoundInstanceVTable *mInstanceVTable;
    // size 0x04
};

} // namespace Sexy

// ---------------------------------------------------------------------------
// FoleyParams -- the per-FoleyType sound definition.
//
// It lives in Sexy\ReconFreeFunctions.h next to gLawnFoleyParamArray /
// gFoleyParamArray / LookupFoley, which is where the placeholder struct used to
// be; that placeholder has been replaced by the real layout, taken from the
// authoritative header PvZ\TodLib\Common\TodFoley.h. See that file for the field
// comments.
// ---------------------------------------------------------------------------

// The 20-byte sound-instance record (`class FoleyInstance` in the authoritative
// header). mInstance is the Sexy::AudiereSoundInstance above.
struct FoleyTypeDataElement
{
    Sexy::AudiereSoundInstance *mInstance; // +0x00
    int mRefCount;                         // +0x04
    bool mPaused;                          // +0x08
    int mStartTime;                        // +0x0C
    int mPauseOffset;                      // +0x10
};

class FoleyTypeData
{
public:
    static const int MAX_FOLEY_INSTANCES = 8;

    FoleyTypeDataElement mFoleyInstances[MAX_FOLEY_INSTANCES]; // +0x00 .. +0x9F
    int mLastVariationPlayed;                                  // +0xA0
    // size 0xA4

    FoleyTypeData();
};

// Layout check: 8 records of 20 bytes plus the variation counter.

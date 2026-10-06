// SoundInstance.h -- the handle a SoundManager hands out for one playing sound.
//
// There was no header for this type: Sexy\SexyAppBase.cpp carried a private copy
// of a few forwarders, so no other translation unit could call a SoundInstance
// at all -- Game\Board.cpp reaches `AdjustPitch` and `Play` through it.
//
// Slot list -- _ZTVN4Sexy13SoundInstanceE @0x68D290 is the abstract base (every
// slot `__cxa_pure_virtual`); the names come from the concrete
// _ZTVN4Sexy18DummySoundInstanceE @0x68D2E8, whose entries are real symbols.
//
// The right-hand numbers are the BINARY's byte offsets. Itanium spends two slots
// on a virtual destructor and MSVC spends one, so the declared order is one slot
// shorter than those numbers -- the same note Image.h and NativeDisplay.h carry.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class SoundInstance
{
public:
    virtual ~SoundInstance() {}                                //   0

    virtual void Release() = 0;                                //   8
    virtual void SetBaseVolume(double theVolume) = 0;          //  12
    virtual void SetBasePan(int thePan) = 0;                   //  16
    virtual void SetBaseRate(double theRate) = 0;              //  20
    virtual void AdjustPitch(double thePitch) = 0;             //  24
    virtual void SetVolume(double theVolume) = 0;              //  28
    virtual void SetMasterVolumeIdx(int theIndex) = 0;         //  32
    virtual void SetPan(int thePan) = 0;                       //  36
    virtual void Play(bool theLoop, bool theAutoRelease) = 0;  //  40
    virtual void Stop() = 0;                                   //  44
    virtual bool IsPlaying() = 0;                              //  48
    virtual bool IsReleased() = 0;                             //  52
};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::SoundInstance;

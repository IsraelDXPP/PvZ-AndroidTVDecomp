// SoundManager.h -- the sound engine interface.
//
// There was no header for this type: Sexy\SexyAppBase.cpp carried a private copy
// of the few forwarders it needed, so no other translation unit could call it --
// Game\Board.cpp reaches `GetSoundInstance` from the level-end sequence.
//
// Slot list -- _ZTVN4Sexy12SoundManagerE @0x68D190 is the abstract base (every
// slot `__cxa_pure_virtual` except Update at 88); the names come from the
// concrete _ZTVN4Sexy17DummySoundManagerE @0x68D220, whose entries are real
// symbols. The table is 21 slots long, 0..88.
//
// The right-hand numbers are the BINARY's byte offsets. Itanium spends two slots
// on a virtual destructor and MSVC spends one, so the declared order is one slot
// shorter than those numbers -- the same note Image.h and NativeDisplay.h carry.
//
// This is an interface: the tree never constructs a SoundManager, it only calls
// through a pointer, so the slots are declared and not defined.

#pragma once

#include "ReconTypes.h"
#include "SexyString.h"
#include "SoundInstance.h"

namespace Sexy {

class SoundManager
{
public:
    virtual ~SoundManager();                                                   //   0

    virtual bool Initialized() = 0;                                            //   8
    virtual bool LoadSound(unsigned int theSoundId, const String & theFileName) = 0; //  12
    virtual int LoadSound(const String & theFileName) = 0;                     //  16
    virtual void ReleaseSound(unsigned int theSoundId) = 0;                    //  20
    virtual void SetVolume(double theVolume) = 0;                              //  24
    virtual void SetVolume(int theSoundId, double theVolume) = 0;              //  28
    virtual void SetBaseVolume(unsigned int theSoundId, double theVolume) = 0; //  32
    virtual void SetBasePan(unsigned int theSoundId, int thePan) = 0;          //  36
    virtual bool IsPlaying(unsigned int theSoundId) = 0;                       //  40
    virtual SoundInstance * GetPlayingSoundInstance(unsigned int theSoundId) = 0; //  44
    virtual SoundInstance * GetSoundInstance(unsigned int theSoundId) = 0;     //  48
    virtual void ReleaseSounds() = 0;                                          //  52
    virtual void ReleaseChannels() = 0;                                        //  56
    virtual double GetMasterVolume() = 0;                                      //  60
    virtual void SetMasterVolume(double theVolume) = 0;                        //  64
    virtual void Flush() = 0;                                                  //  68
    virtual void SetCooperativeWindow(void * theWindow, bool theActive) = 0;   //  72
    virtual void StopAllSounds() = 0;                                          //  76
    virtual int GetFreeSoundId() = 0;                                          //  80
    virtual int GetNumSounds() = 0;                                            //  84
    virtual void Update() = 0;                                                 //  88
};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::SoundManager;

// MusicInterface -- reconstructed from libGameMain.so.
//
// The framework's audio backend interface. `_ZTVN4Sexy14MusicInterfaceE` is at
// 0x68D640 and holds 26 slots; `Sexy::AudiereMusicInterface`
// (`_ZTVN4Sexy21AudiereMusicInterfaceE` @0x68ECD8) is the same 26 slots with real
// bodies, and only the destructor pair and the platform-specific bodies differ.
//
// Until now this class had NO header: `Sexy\ReconTypes.h` only forward-declares
// it, so `SexyAppBase::mMusicInterface` (+0x304) and every call through it were
// incomplete-type errors. Three files had grown private copies of it --
// `Game\Music.cpp` (the full declaration), `Sexy\SexyAppBase.cpp` and
// `Sexy\ReconFuncs.cpp` (a placeholder with a constructor only) -- which is
// exactly the shape the SoundManager problem has. This header is the home the
// copies want; each of those files should drop its local copy and include this
// one instead.
//
// SLOT ORDER. Taken from the two `_ZTV` groups above, slot k at `symbol + 8 + 4k`
// with the Thumb bit cleared. The Itanium table carries a D0/D1 destructor pair
// where MSVC x86 keeps a single slot, so the numbers below are the Itanium ones
// (2..25) and MSVC's own run one lower; only the relative order matters and it is
// unchanged.
//
//     byte  [slot] method                       byte  [slot] method
//     0x08  [ 2]  IsLoaded                       0x38  [14]  FadeOut
//     0x0C  [ 3]  LoadMusic                      0x3C  [15]  FadeOutAll
//     0x10  [ 4]  PlayMusic                      0x40  [16]  SetSongVolume
//     0x14  [ 5]  StopMusic                      0x44  [17]  SetSongMaxVolume
//     0x18  [ 6]  PauseMusic                     0x48  [18]  IsPlaying
//     0x1C  [ 7]  ResumeMusic                    0x4C  [19]  SetChannelVolume
//     0x20  [ 8]  StopAllMusic                   0x50  [20]  GetOrder
//     0x24  [ 9]  UnloadMusic                    0x54  [21]  GetTempo
//     0x28  [10]  UnloadAllMusic                 0x58  [22]  SetTempo
//     0x2C  [11]  PauseAllMusic                  0x5C  [23]  SetVolume
//     0x30  [12]  ResumeAllMusic                 0x60  [24]  SetMusicAmplify
//     0x34  [13]  FadeIn                         0x64  [25]  Update
//
// The song ids the interface takes are `MusicTune` values, not `MusicFile`
// values: `Music2::MusicInit` loads "music/day.ogg" as id 1 and
// `Music2::MusicLoadCreditsSong` loads "sounds/ZombiesOnYourLawn.ogg" as id 13,
// which are MUSIC_TUNE_DAY_GRASSWALK and MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_MUSICINTERFACE_H
#define LAWNPROJECTTV_SEXY_INCLUDE_MUSICINTERFACE_H

#include "ReconTypes.h"

namespace Sexy
{

class MusicInterface
{
public:
    virtual ~MusicInterface();

    virtual bool IsLoaded(int theSongId); // [ 2] +0x08
    virtual bool LoadMusic(int theSongId, Sexy::String const &theFileName); // [ 3] +0x0C
    virtual void PlayMusic(int theSongId, int theOffset, bool noLoop); // [ 4] +0x10
    virtual void StopMusic(int theSongId); // [ 5] +0x14
    virtual void PauseMusic(int theSongId); // [ 6]
    virtual void ResumeMusic(int theSongId); // [ 7]
    virtual void StopAllMusic(); // [ 8]
    virtual void UnloadMusic(int theSongId); // [ 9]
    virtual void UnloadAllMusic(); // [10]
    virtual void PauseAllMusic(); // [11]
    virtual void ResumeAllMusic(); // [12]
    virtual void FadeIn(int theSongId, int theOffset, double theVolume, bool noLoop); // [13]
    virtual void FadeOut(int theSongId, bool noLoop, double theVolume); // [14]
    virtual void FadeOutAll(bool noLoop, double theVolume); // [15]
    virtual void SetSongVolume(int theSongId, double theVolume); // [16] +0x40
    virtual void SetSongMaxVolume(int theSongId, double theVolume); // [17]
    virtual bool IsPlaying(int theSongId); // [18]
    virtual void SetChannelVolume(int theSongId, int theChannel, float theVolume); // [19] +0x4C
    virtual int GetOrder(int theSongId); // [20] +0x50
    virtual int GetTempo(int theSongId); // [21]
    virtual void SetTempo(int theSongId, int theTempo); // [22]
    virtual void SetVolume(double theVolume); // [23]
    virtual void SetMusicAmplify(int theSongId, double theVolume); // [24]
    virtual void Update(); // [25]
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_MUSICINTERFACE_H

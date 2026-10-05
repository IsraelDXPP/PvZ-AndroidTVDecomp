// MusicInterface -- reconstructed from libGameMain.so.
//
// The framework's audio-backend interface. `_ZTVN4Sexy14MusicInterfaceE` is at
// 0x68D640; slot k is at `0x68D640 + 8 + 4k` (offset-to-top at +0, typeinfo at
// +4, and the ARM Thumb pointers have bit 0 set, so it is cleared before
// resolving a name). The 26 slots decode to exactly the declarations in
// Sexy\MusicInterface.h, in that order:
//
//   slot  byte  address     method                       body in the ARM?
//   [ 0]     0  0x4062a8    ~MusicInterface              yes -- vptr store only
//   [ 1]     4  0x406320    ~MusicInterface (deleting)   shared with [0]
//   [ 2]  0x08  0x4062c0    IsLoaded(int)                yes -- `return 0`
//   [ 3]  0x0C  0x4062c4    LoadMusic(int,String const&) yes -- `return 0`
//   [ 4]  0x10  0x4062c8    PlayMusic(int,int,bool)      yes -- empty
//   [ 5]  0x14  0x4062cc    StopMusic(int)               yes -- empty
//   [ 6]  0x18  0x4062d4    PauseMusic(int)              yes -- empty
//   [ 7]  0x1C  0x4062d8    ResumeMusic(int)             yes -- empty
//   [ 8]  0x20  0x4062dc    StopAllMusic()               yes -- empty
//   [ 9]  0x24  0x4062e0    UnloadMusic(int)             yes -- empty
//   [10]  0x28  0x4062e4    UnloadAllMusic()             yes -- empty
//   [11]  0x2C  0x4062e8    PauseAllMusic()              yes -- empty
//   [12]  0x30  0x4062ec    ResumeAllMusic()             yes -- empty
//   [13]  0x34  0x4062f0    FadeIn(int,int,double,bool)  yes -- empty
//   [14]  0x38  0x4062f4    FadeOut(int,bool,double)     yes -- empty
//   [15]  0x3C  0x4062f8    FadeOutAll(bool,double)      yes -- empty
//   [16]  0x40  0x4062fc    SetSongVolume(int,double)    yes -- empty
//   [17]  0x44  0x406300    SetSongMaxVolume(int,double) yes -- empty
//   [18]  0x48  0x406304    IsPlaying(int)               yes -- `return 0`
//   [19]  0x4C  0x4062d0    SetChannelVolume(int,int,float) yes -- empty
//   [20]  0x50  0x406308    GetOrder(int)                yes -- `return 0`
//   [21]  0x54  0x40630c    GetTempo(int)                yes -- `return 0`
//   [22]  0x58  0x406310    SetTempo(int,int)            yes -- empty
//   [23]  0x5C  0x406314    SetVolume(double)            yes -- empty
//   [24]  0x60  0x406318    SetMusicAmplify(int,double)  yes -- empty
//   [25]  0x64  0x40631c    Update()                     yes -- empty
//   [26]       0x688428    (end of table -- the next group's typeinfo)
//
// EVERY body is one of two two/three-instruction shapes. The compiler emitted
// the abstract base's method table in full, so all 24 slots have real (if
// trivial) bodies rather than `__cxa_pure_virtual`:
//
//     `70 47`        = `BX LR`               -- an empty body, 2 bytes
//     `00 20 70 47`  = `MOVS R0, #0; BX LR`  -- `return 0`, 4 bytes
//
// That is the whole platform story, and it is not an artifact of a stripped
// build. This class is the Android/TV audio backend's INTERFACE: the real
// playback lives in the concrete `Sexy::AudiereMusicInterface`
// (`_ZTVN4Sexy21AudiereMusicInterfaceE` @0x68ECD8), which overrides 22 of these
// 24 slots with substantial bodies and inherits only `SetSongMaxVolume`
// (still 0x406300 here) and `SetMusicAmplify` (still 0x406318). The framework
// constructs a bare `MusicInterface` in `SexyAppBase::CreateMusicInterface`
// (SexyAppBase.cpp:1459) only when no platform backend is registered, and in
// that configuration every call is a deliberate no-op.
//
// CONSEQUENCE FOR THIS PORT, stated because it is observable: `LoadMusic`
// returns false, and `Music::LoadSong` (Game\Music.cpp:189) reads that as "the
// song could not be loaded" and sets `mMusicDisabled = true`, so music stays off
// for the session. That is the faithful behaviour of a build with no audio
// backend, not a bug introduced here. A Windows backend would subclass this
// class and override these slots -- the virtuals are declared non-pure in the
// header precisely so that it can.
//
// The destructor is NOT defined here. `Sexy::MusicInterface::~MusicInterface()`
// already has its definition in Sexy\ReconFuncs.cpp:906, and because the
// destructor is the class's key function that is also the translation unit
// which emits `??_7MusicInterface@Sexy@@6B@`. Defining it a second time here
// would be an LNK2005 duplicate.

#include "MusicInterface.h"

// ==========================================================================
// Sexy::MusicInterface::IsLoaded(int)
// @004062C0  4 bytes
// ==========================================================================
bool Sexy::MusicInterface::IsLoaded(int theSongId)
{
  // `MOVS R0, #0; BX LR` -- the ARM body is a constant `return 0`, so nothing
  // is ever reported as loaded. Music::MusicLoadCreditsSong (Game\Music.cpp:662)
  // reads this and goes on to LoadSong, which fails for the same reason.
  (void)theSongId;
  return false;
}

// ==========================================================================
// Sexy::MusicInterface::LoadMusic(int,Sexy::String const&)
// @004062C4  4 bytes
// ==========================================================================
bool Sexy::MusicInterface::LoadMusic(int theSongId, Sexy::String const & theFileName)
{
  // `MOVS R0, #0; BX LR` -- a constant `return 0`: no backend, so no song is
  // ever loaded. The caller in Game\Music.cpp:189 turns this into
  // `mMusicDisabled = true`.
  (void)theSongId;
  (void)theFileName;
  return false;
}

// ==========================================================================
// Sexy::MusicInterface::PlayMusic(int,int,bool)
// @004062C8  2 bytes
// ==========================================================================
void Sexy::MusicInterface::PlayMusic(int theSongId, int theOffset, bool noLoop)
{
  // `BX LR` -- the ARM body is empty.
  (void)theSongId;
  (void)theOffset;
  (void)noLoop;
}

// ==========================================================================
// Sexy::MusicInterface::StopMusic(int)
// @004062CC  2 bytes
// ==========================================================================
void Sexy::MusicInterface::StopMusic(int theSongId)
{
  // `BX LR` -- the ARM body is empty.
  (void)theSongId;
}

// ==========================================================================
// Sexy::MusicInterface::SetChannelVolume(int,int,float)
// @004062D0  2 bytes
// ==========================================================================
// The channel is an xm module channel, not a song: Music::SetupMusicFileForTune
// (Game\Music.cpp:178) mutes and unmutes individual channels of a loaded module
// through this slot to strip the drums and hihats into their own tracks.
void Sexy::MusicInterface::SetChannelVolume(int theSongId, int theChannel, float theVolume)
{
  // `BX LR` -- the ARM body is empty. Nothing to mute when nothing loaded.
  (void)theSongId;
  (void)theChannel;
  (void)theVolume;
}

// ==========================================================================
// Sexy::MusicInterface::PauseMusic(int)
// @004062D4  2 bytes
// ==========================================================================
void Sexy::MusicInterface::PauseMusic(int theSongId)
{
  // `BX LR` -- the ARM body is empty.
  (void)theSongId;
}

// ==========================================================================
// Sexy::MusicInterface::ResumeMusic(int)
// @004062D8  2 bytes
// ==========================================================================
void Sexy::MusicInterface::ResumeMusic(int theSongId)
{
  // `BX LR` -- the ARM body is empty.
  (void)theSongId;
}

// ==========================================================================
// Sexy::MusicInterface::StopAllMusic()
// @004062DC  2 bytes
// ==========================================================================
void Sexy::MusicInterface::StopAllMusic()
{
  // `BX LR` -- the ARM body is empty.
}

// ==========================================================================
// Sexy::MusicInterface::UnloadMusic(int)
// @004062E0  2 bytes
// ==========================================================================
void Sexy::MusicInterface::UnloadMusic(int theSongId)
{
  // `BX LR` -- the ARM body is empty.
  (void)theSongId;
}

// ==========================================================================
// Sexy::MusicInterface::UnloadAllMusic()
// @004062E4  2 bytes
// ==========================================================================
void Sexy::MusicInterface::UnloadAllMusic()
{
  // `BX LR` -- the ARM body is empty.
}

// ==========================================================================
// Sexy::MusicInterface::PauseAllMusic()
// @004062E8  2 bytes
// ==========================================================================
void Sexy::MusicInterface::PauseAllMusic()
{
  // `BX LR` -- the ARM body is empty.
}

// ==========================================================================
// Sexy::MusicInterface::ResumeAllMusic()
// @004062EC  2 bytes
// ==========================================================================
void Sexy::MusicInterface::ResumeAllMusic()
{
  // `BX LR` -- the ARM body is empty.
}

// ==========================================================================
// Sexy::MusicInterface::FadeIn(int,int,double,bool)
// @004062F0  2 bytes
// ==========================================================================
void Sexy::MusicInterface::FadeIn(int theSongId, int theOffset, double theVolume, bool noLoop)
{
  // `BX LR` -- the ARM body is empty. The fade is driven from the game side
  // instead: Music::MusicUpdate (Game\Music.cpp:557) ramps the volume itself
  // with TodAnimateCurveFloat and pushes each step through SetSongVolume.
  (void)theSongId;
  (void)theOffset;
  (void)theVolume;
  (void)noLoop;
}

// ==========================================================================
// Sexy::MusicInterface::FadeOut(int,bool,double)
// @004062F4  2 bytes
// ==========================================================================
void Sexy::MusicInterface::FadeOut(int theSongId, bool noLoop, double theVolume)
{
  // `BX LR` -- the ARM body is empty, for the same reason as FadeIn above.
  (void)theSongId;
  (void)noLoop;
  (void)theVolume;
}

// ==========================================================================
// Sexy::MusicInterface::FadeOutAll(bool,double)
// @004062F8  2 bytes
// ==========================================================================
void Sexy::MusicInterface::FadeOutAll(bool noLoop, double theVolume)
{
  // `BX LR` -- the ARM body is empty.
  (void)noLoop;
  (void)theVolume;
}

// ==========================================================================
// Sexy::MusicInterface::SetSongVolume(int,double)
// @004062FC  2 bytes
// ==========================================================================
// A song id here is a `MusicTune`, not a `MusicFile` (see the note at the top of
// Sexy\MusicInterface.h), which is why Music::UpdateMusicBurst passes the tune
// ids it tracks rather than the file ids it loaded.
void Sexy::MusicInterface::SetSongVolume(int theSongId, double theVolume)
{
  // `BX LR` -- the ARM body is empty.
  (void)theSongId;
  (void)theVolume;
}

// ==========================================================================
// Sexy::MusicInterface::SetSongMaxVolume(int,double)
// @00406300  2 bytes
// ==========================================================================
void Sexy::MusicInterface::SetSongMaxVolume(int theSongId, double theVolume)
{
  // `BX LR` -- the ARM body is empty. This is one of the two slots the concrete
  // AudiereMusicInterface does NOT override (its table @0x68ECD8 points slot 17
  // straight back at this address), so the empty body is the real
  // implementation for every backend in this build.
  (void)theSongId;
  (void)theVolume;
}

// ==========================================================================
// Sexy::MusicInterface::IsPlaying(int)
// @00406304  4 bytes
// ==========================================================================
bool Sexy::MusicInterface::IsPlaying(int theSongId)
{
  // `MOVS R0, #0; BX LR` -- a constant `return 0`: nothing ever plays.
  (void)theSongId;
  return false;
}

// ==========================================================================
// Sexy::MusicInterface::GetOrder(int)
// @00406308  4 bytes
// ==========================================================================
int Sexy::MusicInterface::GetOrder(int theSongId)
{
  // `MOVS R0, #0; BX LR` -- a constant `return 0`. The order is the module
  // position the track has reached; Music::UpdateMusicBurst (Game\Music.cpp:359)
  // uses it to keep the drums layer in step with the main track, and reads 0 as
  // "at the very start", so the burst logic stays consistent rather than
  // faulting.
  (void)theSongId;
  return 0;
}

// ==========================================================================
// Sexy::MusicInterface::GetTempo(int)
// @0040630C  4 bytes
// ==========================================================================
int Sexy::MusicInterface::GetTempo(int theSongId)
{
  // `MOVS R0, #0; BX LR` -- a constant `return 0`.
  (void)theSongId;
  return 0;
}

// ==========================================================================
// Sexy::MusicInterface::SetTempo(int,int)
// @00406310  2 bytes
// ==========================================================================
void Sexy::MusicInterface::SetTempo(int theSongId, int theTempo)
{
  // `BX LR` -- the ARM body is empty.
  (void)theSongId;
  (void)theTempo;
}

// ==========================================================================
// Sexy::MusicInterface::SetVolume(double)
// @00406314  2 bytes
// ==========================================================================
void Sexy::MusicInterface::SetVolume(double theVolume)
{
  // `BX LR` -- the ARM body is empty. SexyAppBase::SetMusicVolume
  // (SexyAppBase.cpp:1167) guards its call with a null test on mMusicInterface
  // but not on a live backend, so this is the slot that swallows the master
  // volume when there is none.
  (void)theVolume;
}

// ==========================================================================
// Sexy::MusicInterface::SetMusicAmplify(int,double)
// @00406318  2 bytes
// ==========================================================================
void Sexy::MusicInterface::SetMusicAmplify(int theSongId, double theVolume)
{
  // `BX LR` -- the ARM body is empty. Like SetSongMaxVolume above, this is one
  // of the two slots the concrete AudiereMusicInterface inherits rather than
  // overrides (its table @0x68ECD8 points slot 24 back at this address).
  (void)theSongId;
  (void)theVolume;
}

// ==========================================================================
// Sexy::MusicInterface::Update()
// @0040631C  2 bytes
// ==========================================================================
void Sexy::MusicInterface::Update()
{
  // `BX LR` -- the ARM body is empty. SexyAppBase::UpdateFrames calls this once
  // per frame (SexyAppBase.cpp:6132); with no backend there is no streaming
  // state to advance.
}

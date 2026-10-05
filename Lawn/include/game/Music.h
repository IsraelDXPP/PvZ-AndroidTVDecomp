// Music -- reconstructed from libGameMain.so.
//
// `class Music : public SyncObject`.
//   RTTI: `_ZTI5Music` @0x6847B0 records `_ZTI10SyncObject` @0x67CDB8 as its base,
//   and the authoritative PvZ\Lawn\System\Music.h says
//   `class Music : public SyncObject` as well.
//
// SIZE 0x68, NOT 0x130.
//   SyncObject owns +0x00..+0x0F (vtable pointer plus the 12-byte sync-block
//   vector) and Music adds 22 words, so 4 + 22 = 26 words = 0x68 -- exactly the
//   "size = 26 ints" the authoritative header records. The 0x130 the transcriber
//   wrote came from misreading vtable-slot reads as member accesses: Music.cpp
//   loads the word at byte offset 144 and calls it as a function pointer, so 144 is
//   a vtable byte offset, not a member (documented in build\map-lawn.md section
//   "System\Music.h").
//   Every member offset below is confirmed by a body: +0x10 mApp, +0x14
//   mMusicInterface, +0x18/+0x1C the two tunes, +0x30 mBaseBPM = 155, +0x34
//   mBaseModSpeed = 3, +0x38 mMusicBurstState, +0x3C mBurstStateCounter = 400,
//   +0x40 mMusicDrumsState, +0x4C/+0x50 the pause offsets, +0x54/+0x55 the two
//   bools, +0x58/+0x5C the fade pair, +0x60/+0x64 the two volumes = 1.0f.
//
// VTABLE -- 21 slots @0x6847C0. Slot 0/1 is the destructor (SyncObject's), slots
// 2..20 are Music's own 19 new virtuals, declared below in this order:
//   [  2] 0x1b8624 MusicInit            [ 12] 0x1b7ebc MusicResyncChannel
//   [  3] 0x1b7f68 MusicDispose         [ 13] 0x1b852c MusicTitleScreenInit
//   [  4] 0x1b8348 MusicUpdate          [ 14] 0x1b893c MakeSureMusicIsPlaying
//   [  5] 0x1b7e24 StopAllMusic         [ 15] 0x1b7efc FadeOut
//   [  6] 0x1b83f0 StartGameMusic       [ 16] 0x1b7c54 SetupMusicFileForTune
//   [  7] 0x1b7e00 LoadSong             [ 17] 0x1b7f08 GetMusicOrder
//   [  8] 0x1b7ec0 MusicResync          [ 18] 0x1b7e1c GetNumLoadingTasks
//   [  9] 0x1b7fa0 UpdateMusicBurst     [ 19] 0x1b85a0 MusicLoadCreditsSong
//   [ 10] 0x1b7ee8 StartBurst           [ 20] 0x1b7e84 PlayFromOffset
//   [ 11] 0x1b7ef8 GameMusicPause
// The Itanium table has a D0/D1 destructor pair; MSVC x86 keeps one slot, so only
// one `virtual ~Music()` is declared and the relative order is unchanged.
// NOTE: `Music2 : public Music` lives in Game\Music2.h and adds no data members, so
// sizeof(Music2) is also 0x68.

#pragma once

#include "ReconTypes.h"
// The bodies call methods on these complete types.
#include "SyncObject.h"

// Music stores only `LawnApp *mApp`, and it never names Board at all. Both
// includes were therefore pure cycle: Music2.h derives from Music, and when this
// header was entered first its transitive LawnApp.h reached Music2.h while Music
// was still mid-parse, so `#pragma once` skipped Music's definition and
// `class Music2 : public Music` reported C2504. Keep the declaration instead --
// including LawnApp.h also pulled all 75 of its includes in for one pointer.
// Sexy\ReconTypes.h already declares LawnApp.
class LawnApp;

// MusicTune and MusicFile already exist in Sexy\ReconTypes.h, so they are only
// spelled here. The two state enums have no other home in this tree and are
// defined from the authoritative PvZ\Lawn\System\Music.h (values identical to
// PvZ\Lawn\Common\ConstEnums.h).
enum MusicBurstState
{
    MUSIC_BURST_OFF = 0,
    MUSIC_BURST_STARTING = 1,
    MUSIC_BURST_ON = 2,
    MUSIC_BURST_FINISHING = 3,
};

enum MusicDrumsState
{
    MUSIC_DRUMS_OFF = 0,
    MUSIC_DRUMS_ON_QUEUED = 1,
    MUSIC_DRUMS_ON = 2,
    MUSIC_DRUMS_OFF_QUEUED = 3,
    MUSIC_DRUMS_FADING = 4,
};

class Music : public SyncObject
{
public:
    virtual ~Music();

    // slots 2..20, in vtable order
    virtual void MusicInit();
    virtual void MusicDispose();
    virtual void MusicUpdate();
    virtual void StopAllMusic();
    virtual void StartGameMusic(bool theStartPaused);
    virtual void LoadSong(MusicFile theMusicFile, Sexy::String const& theFileName);
    virtual void MusicResync();
    virtual void UpdateMusicBurst();
    virtual void StartBurst();
    virtual void GameMusicPause(bool thePaused);
    virtual void MusicResyncChannel(MusicFile theMusicFile1, MusicFile theMusicFile2);
    virtual void MusicTitleScreenInit();
    virtual void MakeSureMusicIsPlaying(MusicTune theMusicTune);
    virtual void FadeOut(int theFadeOutDuration);
    virtual void SetupMusicFileForTune(MusicFile theMusicFile, MusicTune theMusicTune);
    virtual int GetMusicOrder(MusicFile theMusicFile);
    virtual int GetNumLoadingTasks();
    virtual void MusicLoadCreditsSong();
    virtual void PlayFromOffset(MusicFile theMusicFile, int theOffset, double theVolume);
    virtual void PlayMusic(MusicTune theTune, int theOffset, int theDrumsOffset);

    // ---- members, absolute offsets (base SyncObject = 0x10) ----
    LawnApp * mApp;                          // +0x10
    Sexy::MusicInterface * mMusicInterface;  // +0x14
    MusicTune mCurMusicTune;                 // +0x18
    MusicTune mLastMusicTune;                // +0x1C
    MusicFile mCurMusicFileMain;             // +0x20
    MusicFile mCurMusicFileDrums;            // +0x24
    MusicFile mCurMusicFileHihats;           // +0x28
    int mBurstOverride;                      // +0x2C
    int mBaseBPM;                            // +0x30  the constructor writes 155
    int mBaseModSpeed;                       // +0x34  the constructor writes 3
    MusicBurstState mMusicBurstState;        // +0x38
    int mBurstStateCounter;                  // +0x3C
    MusicDrumsState mMusicDrumsState;        // +0x40
    int mQueuedDrumTrackPackedOrder;         // +0x44
    int mDrumsStateCounter;                  // +0x48
    int mPauseOffset;                        // +0x4C
    int mPauseOffsetDrums;                   // +0x50
    bool mPaused;                            // +0x54
    bool mMusicDisabled;                     // +0x55
    // TODO(recon): two bytes of padding at +0x56..+0x57 hold the word alignment
    // for mFadeOutCounter; nothing writes them.
    int mFadeOutCounter;                     // +0x58
    int mFadeOutDuration;                    // +0x5C
    float mPauseVolume;                      // +0x60  initialised to 1.0f
    float mNormalVolume;                     // +0x64  initialised to 1.0f
    Music();
};
